// Fill out your copyright notice in the Description page of Project Settings.


#include "Casting.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
// Sets default values
ACasting::ACasting()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	// 禁用相机碰撞
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// 相当于启用了刚体组件
	CasingMesh->SetSimulatePhysics(true);
	// 设置重力
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	// 为其设置默认值
	ShellEjectionImpulse = 10.f;
}

void ACasting::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasting::OnHit);
	// 增加一个弹壳向外飞出的力
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	
}

void ACasting::Destroyed()
{
	// 使其在服务器上和客户端都播放
	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ShellSound,
			GetActorLocation()
		);
	}
}

void ACasting::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	Destroy();
}

