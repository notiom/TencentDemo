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
	// ������ײ������
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// �������ɱ��Ӷ���
	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	// ���ø�����
	cameraBoom->SetupAttachment(GetMesh());

	// ���õ��ɱ۳���
	cameraBoom->TargetArmLength = 600.f;

	// ���ɱ�������ת
	cameraBoom->bUsePawnControlRotation = true;

	// �����������
	followCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	followCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	followCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	// ͨ����ת�Զ���������
	GetCharacterMovement()->bOrientRotationToMovement = true;

	overheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	overheadWidget->SetupAttachment(RootComponent);

	combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	combat->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// �����������ײ
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	// ���÷���������Ƶ��
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	// ���ƶ�ʱ���ƺ���õĺ���
	Super::OnRep_ReplicatedMovement();
	if (GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		SimProxiesTurn();
	}
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Elim()
{
	// ��������
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
	// ����Ҫ�ڷ�����ͬ�����ͻ��˵Ĵ���ಥ����
	bElimmed = true;
	PlayElimMontage();

	// ��ʼ�ܽ�Ч��
	if (DissolveMaterialInstance)
	{
		// �����������̬ʵ��
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// ���ý�ɫ�ƶ�
	GetCharacterMovement()->DisableMovement();
	// ����ֹͣ�˶�,�����ٶ��Լ�����Ӱ��
	GetCharacterMovement()->StopMovementImmediately();

	bDisableGameplay = true;
	if (combat)
	{
		combat->FireButtonPressed(false);
	}
	// ������ײ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// ����ϵͳ
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		// ��������ϵͳ��Ҫ���������
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

	// һ����ɫ������,����destroyed������elim���
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
	// ���÷ָ���
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
			//���̫�ò���,ҲҪ����
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
		// �������Ϊǰ������,ͨ�������ת�ĳ����ȡǰ����X����
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
		// �������Ϊǰ������
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
	// Ҫ������
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
	if (speed == 0.f && !bIsInAir) // ��ֹ����û����Ծ
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

	if (speed > 0.f || bIsInAir) // �ܲ�������Ծ
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
		// ��270 - 360�Ƕȵ�ֵӳ�䵽-90 - 0
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	// ��ֹ����ģ�͵Ķ�����ͼ��ת��ʱ����
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
			// ˵��Ҫ����ת
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
		FireWeapon(); //ִ���������
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
	// ִ���������
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
			// ���ÿ�ʼʱ��startaimrotation
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
			// ����������ӵ���߲��ɼ�
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
	// ���¿ͻ����ϵ�����
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
	// ��ί�к���
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		// ������һ��ʱ����
		// ��ʱ�����ߺ�ί����ӵ�ʱ������
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

	// ֻ�е�������Ǹö����ӵ����ʱ,�ŻḴ�Ƶ��ͻ���
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
	// ����ǰѪ���綨��0 - max
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	// ���·��������ϵ�Ѫ��
	UpdateHUDHealth();
	// ���·��������ϵ��ܻ�����
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		// ���ص�ǰģ����Ϸģʽ
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
	// һ����ȡ�����״̬,�ͻ�������ʼ��
	// ��ʹ��ұ�����,������״̬Ҳ���ᱻ����
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

