// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/WeaponType.h"
// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// 生成碰撞处理方法
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// 创建弹簧臂子对象
	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	// 设置父对象
	cameraBoom->SetupAttachment(GetMesh());

	// 设置弹簧臂长度
	cameraBoom->TargetArmLength = 600.f;

	// 弹簧臂设置旋转
	cameraBoom->bUsePawnControlRotation = true;

	// 创建跟随相机
	followCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	followCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	followCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	// 通过旋转自动调整朝向
	GetCharacterMovement()->bOrientRotationToMovement = true;

	overheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	overheadWidget->SetupAttachment(RootComponent);

	combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 禁用相机的碰撞
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	// 设置服务器更新频率
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	// 当移动时复制后调用的函数
	Super::OnRep_ReplicatedMovement();
	if (GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		SimProxiesTurn();
	}
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Elim()
{
	// 丢掉武器
	if (combat && combat->EquippedWeapon) 
	{
		combat->EquippedWeapon->Dropped();
	}

	MulticastElim();
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->SetHUDCarriedAmmo(0);
	}
	// 将需要在服务器同步到客户端的代码多播出来
	bElimmed = true;
	PlayElimMontage();

	// 开始溶解效果
	if (DissolveMaterialInstance)
	{
		// 创建出这个动态实例
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// 禁用角色移动
	GetCharacterMovement()->DisableMovement();
	// 立即停止运动,包括速度以及物理影响
	GetCharacterMovement()->StopMovementImmediately();

	bDisableGameplay = true;
	if (combat)
	{
		combat->FireButtonPressed(false);
	}
	// 禁用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 粒子系统
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		// 生成粒子系统需要在这个库里
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	// 一旦角色被销毁,就在destroyed中销毁elim组件
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));

	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (combat && combat->EquippedWeapon && bMatchNotInProgress)
	{
		combat->EquippedWeapon->Destroy();
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	// 将得分复制
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			//如果太久不动,也要复制
			OnRep_ReplicatedMovement();
		}
		CalculateAOPitch();
	}
}


void ABlasterCharacter::MoveForward(float _value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && _value != 0.f)
	{
		const FRotator yawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		// 可以理解为前向向量,通过鼠标旋转的朝向获取前进的X向量
		const FVector direction(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(direction, _value);

	}
}

void ABlasterCharacter::MoveRight(float _value)
{
	if (bDisableGameplay) return;
	if (Controller != nullptr && _value != 0.f)
	{
		const FRotator yawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		// 可以理解为前向向量
		const FVector direction(FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(direction, _value);

	}
}

void ABlasterCharacter::Turn(float _value)
{
	AddControllerYawInput(_value);
}

void ABlasterCharacter::LookUp(float _value)
{
	AddControllerPitchInput(_value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	if (combat)
	{
		if (HasAuthority())
		{
			combat->EquipWeapon(overlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}


void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}

}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (combat)
	{
		combat->Reload();
	}
}

void ABlasterCharacter::DropButtonPressed()
{
	if (combat)
	{
		combat->Drop();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	// 要有武器
	if (combat && combat->EquippedWeapon)
	{
		combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (combat && combat->EquippedWeapon)
	{
		combat->SetAiming(false);
	}
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (combat == nullptr || combat->EquippedWeapon == nullptr)
	{
		return;
	}

	bool bIsInAir = GetCharacterMovement()->IsFalling();

	float speed = CalculateSpeed();
	if (speed == 0.f && !bIsInAir) // 静止并且没有跳跃
	{
		bRotateRootBone = true;
		FRotator currentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(currentAimRotation, startingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (speed > 0.f || bIsInAir) // 跑步或者跳跃
	{
		bRotateRootBone = false;
		startingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAOPitch();

}

void ABlasterCharacter::CalculateAOPitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// 将270 - 360角度的值映射到-90 - 0
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	// 防止代理模型的动画蓝图在转身时抖动
	if (combat == nullptr || combat->EquippedWeapon == nullptr)
	{
		return;
	}
	bRotateRootBone = false;
	float speed = CalculateSpeed();
	if (speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	UE_LOG(LogTemp, Warning, TEXT("ProxyYaw : %f"), ProxyYaw);
	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			// 说明要向右转
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}

		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (combat && combat->EquippedWeapon)
	{
		FireWeapon(); //执行射击操作
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (combat && combat->EquippedWeapon)
	{
		combat->FireButtonPressed(false);
	}
}
void ABlasterCharacter::FireWeapon()
{
	// 执行射击操作
	if (combat)
	{
		combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	// UE_LOG(LogTemp, Warning, TEXT("AO_Yaw : %f"), AO_Yaw);

	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}

	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 10.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			// 重置开始时的startaimrotation
			startingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}

}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if((followCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (combat && combat->EquippedWeapon && combat->EquippedWeapon->GetWeaponMesh())
		{
			// 设置武器对拥有者不可见
			combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}

	else
	{
		GetMesh()->SetVisibility(true);
		if (combat && combat->EquippedWeapon && combat->EquippedWeapon->GetWeaponMesh())
		{
			combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (combat)
	{
		combat->EquipWeapon(overlappingWeapon);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (overlappingWeapon)
	{
		overlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector velocity = GetVelocity();
	velocity.Z = 0.f;
	return velocity.Size();
}

void ABlasterCharacter::OnRep_Health()
{
	// 更新客户端上的数据
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	// 绑定委托函数
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		// 创建了一个时间轨道
		// 将时间曲线和委托添加到时间线上
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (!Weapon && overlappingWeapon)
	{
		overlappingWeapon->ShowPickupWidget(false);
	}
	overlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (overlappingWeapon)
		{
			overlappingWeapon->ShowPickupWidget(true);
		}

	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (combat && combat->EquippedWeapon);
}

bool ABlasterCharacter::isAiming()
{
	return (combat && combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (combat == nullptr) return nullptr;
	return combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if(combat == nullptr) return FVector();
	return combat->HitTarget;
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this,&ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Released, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Drop", IE_Released, this, &ABlasterCharacter::DropButtonPressed);
	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 只有当该玩家是该对象的拥有者时,才会复制到客户端
	DOREPLIFETIME_CONDITION(ABlasterCharacter, overlappingWeapon,COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (combat)
	{
		combat->Character = this;
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (combat == nullptr || combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);

		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (combat == nullptr || combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);

		FName SectionName;
		switch (combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:

			SectionName = FName("Rifle");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (combat == nullptr || combat->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);

		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	// 将当前血量界定在0 - max
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	// 更新服务器机上的血条
	UpdateHUDHealth();
	// 更新服务器机上的受击动画
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		// 返回当前模板游戏模式
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::PollInit()
{
	// 一旦获取了玩家状态,就会立即初始化
	// 即使玩家被销毁,这个玩家状态也不会被销毁
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
			BlasterPlayerState->AddToScoreSquare(0);
		}
	}
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (combat == nullptr) return ECombatState::ECS_MAX;
	return combat->CombatState;
}

