// Fill out your copyright notice in the Description page of Project Settings.


#include "AINpcAIController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "AINpc/PatrolPoint.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AINpc/RestPoint.h"
#include "Navigation/PathFollowingComponent.h"
#include "AINpc/AICharacter.h"

void AAINpcAIController::BeginPlay()
{
    Super::BeginPlay();

    // 获取关卡中所有 PatrolPoint
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APatrolPoint::StaticClass(), PatrolPoints);
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARestPoint::StaticClass(), RestPoints);
    MoveToNextPatrolPoint();
}

void AAINpcAIController::MoveToNextPatrolPoint()
{
    if (PatrolPoints.Num() == 0) return;

    // 获取 AI 控制的角色
    AAICharacter* MyCharacter = Cast<AAICharacter>(GetPawn());
    if (!MyCharacter) return;
    if (MyCharacter->bPlayerDetected) return;

    // 设置 AI 角色的移动速度
    MyCharacter->GetCharacterMovement()->MaxWalkSpeed = 300.0f; // 设置速度（可调整）

    // 随机选择一个巡逻点
     // 获取当前目标巡逻点
    AActor* TargetPoint = PatrolPoints[CurrentPatrolIndex];

    // 让 AI 角色移动到目标点
    MoveToActor(TargetPoint, 5.0f);

}

void AAINpcAIController::StopAIMovement()
{
    // UE_LOG(LogTemp, Warning, TEXT("AI 控制器正在停止移动..."));
    bIsHello = true;
    StopMovement();
    // ClearFocus(EAIFocusPriority::Gameplay);

    // 强制 AI "导航" 到当前位置，立即停止导航
    // MoveToLocation(GetPawn()->GetActorLocation());

    /*
    if (GetPathFollowingComponent())
    {
        GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished);
    }
    */
}

void AAINpcAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);
    // 获取 AI 角色
    ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
    if (MyCharacter)
    {
        // 停止 AI 移动并将速度置为 0
        MyCharacter->GetCharacterMovement()->StopMovementImmediately();
        MyCharacter->GetCharacterMovement()->MaxWalkSpeed = 0.0f;
    }

    if (bIsLookfor)
    {
        bIsResting = true;
        return;
    }
    if (bIsNeedRest)
    {

        // 是否需要休息,如果要休息,需要寻路到restpoint
        if (RestPoints.Num() == 0) return;

        // 获取 AI 控制的角色
        if (!MyCharacter) return;

        // 设置 AI 角色的移动速度
        MyCharacter->GetCharacterMovement()->MaxWalkSpeed = 600.0f; // 设置速度（可调整）

        // 随机选择一个巡逻点
         // 获取当前目标巡逻点
        int32 RandomIndex = FMath::RandRange(0, RestPoints.Num() - 1);
        AActor* TargetPoint = RestPoints[RandomIndex];
        bIsLookfor = true;
        // 让 AI 角色移动到目标点
        MoveToActor(TargetPoint, 5.0f);
        return;
    }

    // 让 AI 过一会儿再移动（模拟休息）
    GetWorld()->GetTimerManager().SetTimer(RestartPatrolTimer, this, &AAINpcAIController::MoveToNextPatrolPoint, 0.5f, false);
    // 更新索引，确保循环巡逻
    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
}

void AAINpcAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsHello)
    {
        // 打招呼时间也算在工作时间里
        // 如果在休息时打招呼也算休息
        HelloTimer -= DeltaTime;
        if (HelloTimer < 0.f)
        {
            HelloTimer = HelloTime;
            bIsHello = false;
            AAICharacter* MyCharacter = Cast<AAICharacter>(GetPawn());
            MyCharacter->bPlayerDetected = false;
            if (!bIsResting)
            {
                MoveToNextPatrolPoint();
            }
            return;
        }
    }

    if (bIsResting)
    {
        // 在休息的话就只需要计算剩余的休息时间
        RestTimer -= DeltaTime;
        if (RestTimer < 0.f)
        {
            RestTimer = RestTime;
            bIsResting = false;
            bIsNeedRest = false;
            bIsLookfor = false;
            MoveToNextPatrolPoint();
        }
        return;
    }

    if (!bIsNeedRest)
    {
        // 不需要休息的话才使得工作时间延长
        SplitTimer -= DeltaTime;
    }

    if (SplitTimer < 0.f)
    {
        /// 又到休息时间了
        SplitTimer = SplitRestTime;
        bIsNeedRest = true;
    }
}
