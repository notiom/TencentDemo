// Fill out your copyright notice in the Description page of Project Settings.


#include "AICharacter.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "AINpc/Controller/AINpcAIController.h"
#include "AINpc/AINpcCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AINpc/HUD/OverheadWidget.h"

// Sets default values
AAICharacter::AAICharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	overheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	overheadWidget->SetupAttachment(RootComponent);
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->InitSphereRadius(300.0f); // 设定检测范围
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AAICharacter::OnOverlapBegin);

}

// Called when the game starts or when spawned
void AAICharacter::BeginPlay()
{
	Super::BeginPlay();
    Overhead = Cast<UOverheadWidget>(overheadWidget->GetUserWidgetObject());
    AIController = Cast<AAINpcAIController>(GetController());
}

// Called every frame
void AAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (Overhead == nullptr || AIController == nullptr) return;
    FString ir;
    if (AIController->GetIsHello())
    {
        ir.Append("hello!");
    }
    else if (AIController->GetIsResting())
    {
        ir.Append("Resting!");
    }
    else if (AIController->GetIsNeedRest())
    {
        ir.Append("Need Rest!");
    }
    else if (AIController->GetIsNavgation())
    {
        ir.Append("Navgation!");
    }
    Overhead->ShowInformation(ir);
}

// Called to bind functionality to input
void AAICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AAICharacter::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->IsA(AAINpcCharacter::StaticClass())) // 检查是否是角色
    {
        bPlayerDetected = true;
        if (AIController)
        {
            AIController->StopAIMovement(); // 停止 AI 移动
        }
    }
}

