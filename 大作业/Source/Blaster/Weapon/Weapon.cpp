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
	bReplicates = true; // 在服务器上复制一份

	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	weaponMesh->SetupAttachment(RootComponent);

	SetRootComponent(weaponMesh);

	// 设置武器对所有对象的默认碰撞为block,即五i且无法穿透墙壁物体等
	weaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	// 忽略武器与玩家的碰撞
	weaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	// 初始化时不需要设置武器的碰撞,节省性能开销
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
			// 物理效果,要实实在在的碰撞到地面
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
		// 物理效果,要实实在在的碰撞到地面
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

// 复制武器到服务器上时复制弹药
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
			// 子弹壳不需要设置所有者
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
	// 消耗一颗子弹
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	weaponMesh->DetachFromComponent(DetachRules);

	// 禁止旋转：通过设置一个高的旋转阻尼来减少旋转

	weaponMesh->SetAngularDamping(10.f); // 你可以调整这个值来控制旋转
	// 给武器一个前向速度：可以通过角色的朝向来加速武器
	FVector ForwardDirection = BlasterOwnerCharacter->GetActorForwardVector();  // 获取武器当前的朝向
	FVector Impulse = ForwardDirection * 100.f;  // 给武器添加一个力，调整数值来控制速度

	// 将速度应用到武器
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

