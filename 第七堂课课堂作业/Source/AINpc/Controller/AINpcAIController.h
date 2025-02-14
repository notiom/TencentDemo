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
    float SplitRestTime = 20.f; //ÿ��20s��Ҫȥָ���ص���Ϣ5s
    float SplitTimer = 20.f;
    float HelloTime = 2.0f;
    float HelloTimer = 2.0f;

    bool bIsNeedRest = false; //�Ƿ���Ҫ��Ϣ(��Ҫ�ӵ�ǰ��λ����ָ����Ϣ��)
    bool bIsResting = false; // �Ƿ�������Ϣ

    bool bIsLookfor = false; // �Ƿ��Ѿ��ҵ�����Ϣ��ȥ��·
    bool bIsHello = false; // �Ƿ����ڴ��к�

public:
    FORCEINLINE bool GetIsNavgation() const { return !bIsNeedRest; } // ���Ϊtrue,��ӡѰ·��...
    FORCEINLINE bool GetIsNeedRest() const { return bIsNeedRest; } // ���Ϊtrue,��ӡ��Ҫ��Ϣ...
    FORCEINLINE bool GetIsResting() const { return bIsResting; } // ���Ϊtrue,��ӡ������Ϣ...
    FORCEINLINE bool GetIsHello() const { return bIsHello; } // ���Ϊtrue,��ӡHello��...

};
