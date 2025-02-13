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
	// ֻ��Ҫ�Զ���ӵ���߸��Ƽ���,�����Ĳ���Ҫ����,��ʡ����
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
			// fieldofviewΪ��ǰ�������Ұ
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if (Character->HasAuthority())
		{
			// ��ʼ����ǰ�ĵ�ҩ��
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

		// ֻ���ڱ��ؽ�ɫ�Ż��������ʮ��
		SetHUDCrosshairs(DeltaTime);
		// ���ؽ�ɫ�Ż�ı��Ӿ�
		InterpFOV(DeltaTime);
	}
}


void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	// �����ǰĿ���Ƿ�����,����Ҫ����ServerSetAiming_Implementation
	// ���ɿͻ��˸��Ƶ�������

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
		// ��ȡ�ӿڴ�С
		GEngine->GameViewport->GetViewportSize(viewportSize);
	}
	// ����׼��λ�ã���׼���趨����Ļ����
	FVector2D CrosshairLocation(viewportSize.X / 2.f, viewportSize.Y / 2.f);

	// ����׼������λ�úͷ���
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// ����Ļ����ת������������
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	if (bScreenToWorld)
	{
		// �������ߵ���ʼ�ͽ�����
		FVector Start = CrosshairWorldPosition;

		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			// CrosshairWorldDirection�Ǽ�������ĵ�λ����
			Start += CrosshairWorldDirection * (DistanceToCharacter + 50.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		// ����һ�����ߣ��� Start �� End����������������������ײ
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
		// cast����c#�е�as,������ת��Ϊ����
		// ֮�������������еĺ���
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			// ��װ������ʱ����,û��ʱ�Ͳ�����
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
			// ����crosshairspread
			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				// �ڿ���ʱ,��Ҳ��ɢ������
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor,
					2.25f, DeltaTime, 2.25f);
			}
			else
			{
				// ����ڵ���,�����������0
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
			// ĳ�ֻ���base value + �ƶ���Ӱ������ + ��Ծ��Ӱ��Ӱ�� - ��׼��Ӱ������ + ����ʱ��Ӱ������
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
	// �ڵ��õ���������ʱ��֮����Ҫ�ÿɿ���Ϊtrue
	// ��Ϊ�е�������Ҫ����ʱ֮����ܼ�������
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;

	// ����ǿ����Զ�������Ҫ�������п���
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
	// ���ڸ�������Ϊownerʱ�Żᱻ����
	// �ڿͻ����Զ�������
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::Fire()
{
	// �����Ƿ����ڷ�������,hitresult.impact�������䵽������
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
	// empalceֱ����ԭ�ع���,��add�Ǹ���
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
}

void UCombatComponent::OnRep_CombatState()
{
	// ����������ʱ�����仯ʱ,�Զ����Ƶ�ÿһ���ͻ���
	// �൱�ڶಥ
	// �ĵ� : 1.ʹ�öಥ����ÿһ���ͻ���
	// 2.ͨ����������ʹһ���������㷢���仯,��ͨ���仯����õ�onrep��������ÿһ���ͻ���
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
	// ��Ҫ����ĵ�ҩ
	int32 ReloadAmount = AmountToReload();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		// ����ʱcarriedammo�����仯ʱ,�Զ�����onrep_carriedammo
		// ���Զ�����carried��HUD
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	// �������ʱ,ammo����ᱻ����,��weapon�е�onrep����������HUD
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

// �ӿͻ��˵���,�ڷ�����ִ��
void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{

	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	// �㲥��ÿһ���ͻ���
	if (EquippedWeapon == nullptr) return;

	if (Character && bFireButtonPressed && CombatState == ECombatState::ECS_Unoccupied)
	{
		// ��ɫ����
		Character->PlayFireMontage(bAiming);
		// ��������
		// ����ʹ��ԭ�е�hittarget,��Ҫʹ�þ������紫����vector
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	// ���װ��һ������,��ô����ԭ�е�����
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

	// ���¹ؼ����ݺ�״̬ʱ�����ڷ�����������
	// ��ͨ��replicatedusing���Ƶ��ͻ���
	// �ڷ����������е��߼�������Ҫ�ڿͻ�����ʹ��OnRep
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
	// �������Я����������ҩ����0,�ͷ�����������
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
		// ֻ��Ҫ�ڷ�������ִ�еĴ���
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
	// ��������ִ�еĴ���
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
	// һ������������� - ��ǰ�ӵ�����
	// Ӧ�ò�������ӵ�
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		// �����Ի���ʣ�����е��е��ӵ�����
		int32 AmmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		// Ҫ��������Ҫ��ȡ����ʣ����ӵ�
		int32 Least = FMath::Min(RoomInMag, AmmountCarried);
		// ����������������ǰ�ӵ����������������,�����Ǹ�ֵ����RoomInMag������ >= 0
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
		// �Ƿ�����ƶ����������ת
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