// Fill out your copyright notice in the Description page of Project Settings.


#include "SquareActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Sound/SoundCue.h"
#include "Blaster/Weapon/ProjectileBullet.h"
#include "GameFramework/Character.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Blaster.h"


// Sets default values
ASquareActor::ASquareActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SquareMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SquareMesh"));

    SetRootComponent(SquareMesh);

	// 碰撞类型为与动态物体进行碰撞
    SquareMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    SquareMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// 先取消对所有的碰撞
    SquareMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	// 开启碰撞
    SquareMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,ECollisionResponse::ECR_Block);
	/**/
    SquareMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	/**/
    SquareMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
    SquareMesh->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECollisionResponse::ECR_Block);
}

// Called when the game starts or when spawned
void ASquareActor::BeginPlay()
{
	Super::BeginPlay();
	ABlasterGameState* BlasterGameState = GetWorld()->GetGameState<ABlasterGameState>();
	if (BlasterGameState)
	{
		if (BlasterGameState->SquareOfAmount % 5 == 0)
		{
			SquareColor = FColor::Red;
			SquareMesh->SetStaticMesh(BlasterGameState->SquareRedMesh);
		}
		else
		{
			SquareColor = FColor::White;
			SquareMesh->SetStaticMesh(BlasterGameState->SquareWhiteMesh);
		}
	}
    if (HasAuthority())
    {
        SquareMesh->OnComponentHit.AddDynamic(this, &ASquareActor::OnHit);
    }

}

void ASquareActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASquareActor, bIsFirstCollision);
	DOREPLIFETIME(ASquareActor, ReplicatedScale);
}

void ASquareActor::Destroyed()
{
	// 播放销毁声音
	// 使其在服务器上和客户端都播放
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			GetActorLocation()
		);
	}
}

void ASquareActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (!HasAuthority())
    {
        ServerHandleHit(OtherActor);
        return;
    }

    if (OtherActor)
    {
        
        AProjectileBullet* Projectile = Cast<AProjectileBullet>(OtherActor);
        if (Projectile)
        {
            ACharacter* OwnerCharacter = Cast<ACharacter>(Projectile->GetInstigator());
            if (OwnerCharacter)
            {
                ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);
                if (BlasterPlayerController)
                {
                    ABlasterPlayerState* BlasterPlayerState = BlasterPlayerController->GetPlayerState<ABlasterPlayerState>();
                    if (BlasterPlayerState)
                    {
                        if (bIsFirstCollision)
                        {
                            if (SquareColor == FColor::Red)
                            {
                                BlasterPlayerState->AddToScoreSquare(FirstScore * 2);
                            }
                            else
                            {
                                BlasterPlayerState->AddToScoreSquare(FirstScore);
                            }
                            bIsFirstCollision = false;

                            // 服务器修改缩放，并同步到客户端
                            if (HasAuthority())
                            {
                                ReplicatedScale = GetActorScale3D() * 0.5f;
                                OnRep_Scale();
                            }
                        }
                        else
                        {
                            if (SquareColor == FColor::Red)
                            {
                                BlasterPlayerState->AddToScoreSquare(SecondScore * 2);
                            }
                            else
                            {
                                BlasterPlayerState->AddToScoreSquare(SecondScore);
                            }

                            if (HasAuthority())
                            {
                                Destroy();
                            }
                        }
                    }
                }
            }
        }
    }
}



       
/*
*/
void ASquareActor::ServerHandleHit_Implementation(AActor* OtherActor)
{
    OnHit(SquareMesh, OtherActor, nullptr, FVector::ZeroVector, FHitResult());
}

void ASquareActor::OnRep_Scale()
{
	SetActorScale3D(ReplicatedScale);
}

// Called every frame
void ASquareActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

