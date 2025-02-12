// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Casting.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // �ڷ������ϸ���һ��

	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	weaponMesh->SetupAttachment(RootComponent);

	SetRootComponent(weaponMesh);

	// �������������ж����Ĭ����ײΪblock,����i���޷���͸ǽ�������
	weaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	// ������������ҵ���ײ
	weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	// ��ʼ��ʱ����Ҫ������������ײ,��ʡ���ܿ���
	weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	areaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	areaSphere->SetupAttachment(RootComponent);
	areaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	areaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	areaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	pickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	pickupWidget->SetupAttachment(RootComponent);
}


void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		areaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		areaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		areaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		areaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndlap);
	}


	if (pickupWidget)
	{
		pickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* blasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (blasterCharacter)
	{
		blasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* blasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (blasterCharacter)
	{
		blasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (weaponState)
	{
		case EWeaponState::EWS_Equipped:
			ShowPickupWidget(false);
			weaponMesh->SetSimulatePhysics(false);
			weaponMesh->SetEnableGravity(false);
			weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			break;
		case EWeaponState::EWS_Dropped:
			weaponMesh->SetSimulatePhysics(true);
			weaponMesh->SetEnableGravity(true);
			// ����Ч��,Ҫʵʵ���ڵ���ײ������
			weaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			break;
	}
}

void AWeapon::OnRep_Ammo()
{
	SetHUDAmmo();
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	weaponState = State;
	switch (weaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		areaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		weaponMesh->SetSimulatePhysics(false);
		weaponMesh->SetEnableGravity(false);
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EWeaponState::EWS_Dropped:
		if (HasAuthority())
		{
			areaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		weaponMesh->SetSimulatePhysics(true);
		weaponMesh->SetEnableGravity(true);
		// ����Ч��,Ҫʵʵ���ڵ���ײ������
		weaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}

}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, weaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

// ������������������ʱ���Ƶ�ҩ
void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		SetHUDAmmo();
	}
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (pickupWidget)
	{
		pickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		weaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = weaponMesh->GetSocketByName(FName("AmmoEject"));

		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(weaponMesh);
			// �ӵ��ǲ���Ҫ����������
			UWorld* world = GetWorld();
			if (world)
			{
				world->SpawnActor<ACasting>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}

		}
	}
	// ����һ���ӵ�
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	weaponMesh->DetachFromComponent(DetachRules);

	// ��ֹ��ת��ͨ������һ���ߵ���ת������������ת

	weaponMesh->SetAngularDamping(10.f); // ����Ե������ֵ��������ת
	// ������һ��ǰ���ٶȣ�����ͨ����ɫ�ĳ�������������
	FVector ForwardDirection = BlasterOwnerCharacter->GetActorForwardVector();  // ��ȡ������ǰ�ĳ���
	FVector Impulse = ForwardDirection * 100.f;  // ���������һ������������ֵ�������ٶ�

	// ���ٶ�Ӧ�õ�����
	weaponMesh->AddImpulse(Impulse, NAME_None, true);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo - AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}	 

