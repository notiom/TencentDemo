// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;

}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, bFireButtonPressed);
	// 只需要对对象拥有者复制即可,其他的不需要看到,节省带宽
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}

// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			// fieldofview为当前相机的视野
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			// 初始化当前的弹药库
			InitializeCarriedAmmo();
		}
	}

}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character && Character->IsLocallyControlled())
	{

		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		// 只有在本地角色才会调用设置十字
		SetHUDCrosshairs(DeltaTime);
		// 本地角色才会改变视距
		InterpFOV(DeltaTime);
	}
}


void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	// 如果当前目标是服务器,就需要调用ServerSetAiming_Implementation
	// 即由客户端复制到服务器

	ServerSetAiming(bAiming);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetFirePressed_Implementation(bool bIsPressed)
{
	bFireButtonPressed = bIsPressed;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	ServerSetFirePressed(bFireButtonPressed);
	if (bFireButtonPressed)
	{
		Fire();
	}

}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D viewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		// 获取视口大小
		GEngine->GameViewport->GetViewportSize(viewportSize);
	}
	// 计算准星位置：将准星设定在屏幕中央
	FVector2D CrosshairLocation(viewportSize.X / 2.f, viewportSize.Y / 2.f);

	// 声明准星世界位置和方向
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// 将屏幕坐标转换到世界坐标
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		// 设置射线的起始和结束点
		FVector Start = CrosshairWorldPosition;

		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			// CrosshairWorldDirection是计算出来的单位向量
			Start += CrosshairWorldDirection * (DistanceToCharacter + 50.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		// 发射一条射线，从 Start 到 End，并检查与世界中物体的碰撞
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		// cast像在c#中的as,将父类转化为子类
		// 之后便可以用子类中的函数
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			// 有装备武器时绘制,没有时就不绘制
			if (EquippedWeapon)
			{

				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// 计算crosshairspread
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				// 在空中时,他也会散步开来
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor,
					2.25f, DeltaTime, 2.25f);
			}
			else
			{
				// 如果在地面,会快速收缩至0
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor,
					0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}

			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);
			// 某种基础base value + 移动的影响因子 + 跳跃的影响影子 - 瞄准的影响因子 + 开火时的影响因子
			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}

}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomedInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr) return;
	Character->GetWorld()->GetTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FinishFireTimer,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FinishFireTimer()
{
	// 在调用到这个开火计时器之后需要置可开火为true
	// 因为有的武器需要有延时之后才能继续开火
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;

	// 如果是可以自动连击才要继续运行开火
	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	// 仅在复制条件为owner时才会被调用
	// 在客户端自动被调用
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::Fire()
{
	// 无论是否是在服务器上,hitresult.impact都被传输到服务器
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			if (CrosshairShootingFactor >= 1.f) return;
			CrosshairShootingFactor += 0.2f;
		}
		StartFireTimer();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	// empalce直接在原地构造,而add是复制
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::OnRep_CombatState()
{
	// 当变量复制时发生变化时,自动复制到每一个客户端
	// 相当于多播
	// 心得 : 1.使用多播播到每一个客户端
	// 2.通过服务器端使一个便来给你发生变化,在通过变化后调用的onrep函数播到每一个客户端
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::UpdateAmmoValue()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	// 需要补充的弹药
	int32 ReloadAmount = AmountToReload();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		// 当此时carriedammo发生变化时,自动调用onrep_carriedammo
		// 会自动设置carried的HUD
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// 运行这个时,ammo本身会被复制,而weapon中的onrep函数设置了HUD
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

// 从客户端调用,在服务器执行
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{

	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// 广播到每一个客户端
	if (EquippedWeapon == nullptr) return;

	if (Character && bFireButtonPressed && CombatState == ECombatState::ECS_Unoccupied)
	{
		// 角色开火
		Character->PlayFireMontage(bAiming);
		// 武器开火
		// 不能使用原有的hittarget,需要使用经过网络传输后的vector
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	// 如果装有一把武器,那么丢弃原有的武器
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();

	// 更新关键数据和状态时必须在服务器上运行
	// 再通过replicatedusing复制到客户端
	// 在服务器上运行的逻辑代码需要在客户端上使用OnRep
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::Reload()
{
	// 如果本地携带的武器弹药大于0,就发送至服务器
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::Drop()
{
	if (EquippedWeapon == nullptr) return;
	ServerDrop();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		// 只需要在服务器端执行的代码
		UpdateAmmoValue();
	}
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	// 服务器上执行的代码
	HandleReload();
}

void UCombatComponent::ServerDrop_Implementation()
{
	MulticastDrop();
}

void UCombatComponent::MulticastDrop_Implementation()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
		EquippedWeapon = nullptr;
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDWeaponAmmo(0);
			Controller->SetHUDCarriedAmmo(0);
		}
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	// 一个弹夹最大容量 - 当前子弹数量
	// 应该补充多少子弹
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// 还可以换的剩余所有弹夹的子弹数量
		int32 AmmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		// 要不填满，要不取所有剩余的子弹
		int32 Least = FMath::Min(RoomInMag, AmmountCarried);
		// 这里的意义是如果当前子弹数量大于最大容量,可能是负值，将RoomInMag限制在 >= 0
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}

		// UE_LOG(LogTemp, Warning, TEXT("EquippedWeapon changed!"));
		// 是否根据移动方向调整旋转
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;

		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller && EquippedWeapon)
		{
			EquippedWeapon->SetHUDAmmo();
			Controller->SetHUDCarriedAmmo(CarriedAmmo);
		}

		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquipSound,
				Character->GetActorLocation()
			);
		}
	}
}