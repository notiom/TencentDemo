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
	// ���������ײ
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	// �൱�������˸������
	CasingMesh->SetSimulatePhysics(true);
	// ��������
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	// Ϊ������Ĭ��ֵ
	ShellEjectionImpulse = 10.f;
}

void ACasting::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->OnComponentHit.AddDynamic(this, &ACasting::OnHit);
	// ����һ����������ɳ�����
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);

	
}

void ACasting::Destroyed()
{
	// ʹ���ڷ������ϺͿͻ��˶�����
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

