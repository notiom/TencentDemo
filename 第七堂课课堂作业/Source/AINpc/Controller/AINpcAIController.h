// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AINpcAIController.generated.h"

/**
 * 
 */
UCLASS()
class AINPC_API AAINpcAIController : public AAIController
{
	GENERATED_BODY()
public:
    virtual void BeginPlay() override;

    void MoveToNextPatrolPoint();
    void StopAIMovement();

private:
    TArray<AActor*> PatrolPoints;
    TArray<AActor*> RestPoints;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
    FTimerHandle RestartPatrolTimer;
    virtual void Tick(float DeltaTime) override;
    int32 CurrentPatrolIndex;

    float RestTime = 5.f;
    float RestTimer = 5.f;
    float SplitRestTime = 20.f; //每隔20s需要去指定地点休息5s
    float SplitTimer = 20.f;
    float HelloTime = 2.0f;
    float HelloTimer = 2.0f;

    bool bIsNeedRest = false; //是否需要休息(需要从当前点位到达指定休息点)
    bool bIsResting = false; // 是否正在休息

    bool bIsLookfor = false; // 是否已经找到了休息回去的路
    bool bIsHello = false; // 是否正在打招呼

public:
    FORCEINLINE bool GetIsNavgation() const { return !bIsNeedRest; } // 如果为true,打印寻路中...
    FORCEINLINE bool GetIsNeedRest() const { return bIsNeedRest; } // 如果为true,打印需要休息...
    FORCEINLINE bool GetIsResting() const { return bIsResting; } // 如果为true,打印正在休息...
    FORCEINLINE bool GetIsHello() const { return bIsHello; } // 如果为true,打印Hello！...

};
