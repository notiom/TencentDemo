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

    // ��ȡ�ؿ������� PatrolPoint
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APatrolPoint::StaticClass(), PatrolPoints);
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARestPoint::StaticClass(), RestPoints);
    MoveToNextPatrolPoint();
}

void AAINpcAIController::MoveToNextPatrolPoint()
{
    if (PatrolPoints.Num() == 0) return;

    // ��ȡ AI ���ƵĽ�ɫ
    AAICharacter* MyCharacter = Cast<AAICharacter>(GetPawn());
    if (!MyCharacter) return;
    if (MyCharacter->bPlayerDetected) return;

    // ���� AI ��ɫ���ƶ��ٶ�
    MyCharacter->GetCharacterMovement()->MaxWalkSpeed = 300.0f; // �����ٶȣ��ɵ�����

    // ���ѡ��һ��Ѳ�ߵ�
     // ��ȡ��ǰĿ��Ѳ�ߵ�
    AActor* TargetPoint = PatrolPoints[CurrentPatrolIndex];

    // �� AI ��ɫ�ƶ���Ŀ���
    MoveToActor(TargetPoint, 5.0f);

}

void AAINpcAIController::StopAIMovement()
{
    // UE_LOG(LogTemp, Warning, TEXT("AI ����������ֹͣ�ƶ�..."));
    bIsHello = true;
    StopMovement();
    // ClearFocus(EAIFocusPriority::Gameplay);

    // ǿ�� AI "����" ����ǰλ�ã�����ֹͣ����
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
    // ��ȡ AI ��ɫ
    ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
    if (MyCharacter)
    {
        // ֹͣ AI �ƶ������ٶ���Ϊ 0
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

        // �Ƿ���Ҫ��Ϣ,���Ҫ��Ϣ,��ҪѰ·��restpoint
        if (RestPoints.Num() == 0) return;

        // ��ȡ AI ���ƵĽ�ɫ
        if (!MyCharacter) return;

        // ���� AI ��ɫ���ƶ��ٶ�
        MyCharacter->GetCharacterMovement()->MaxWalkSpeed = 600.0f; // �����ٶȣ��ɵ�����

        // ���ѡ��һ��Ѳ�ߵ�
         // ��ȡ��ǰĿ��Ѳ�ߵ�
        int32 RandomIndex = FMath::RandRange(0, RestPoints.Num() - 1);
        AActor* TargetPoint = RestPoints[RandomIndex];
        bIsLookfor = true;
        // �� AI ��ɫ�ƶ���Ŀ���
        MoveToActor(TargetPoint, 5.0f);
        return;
    }

    // �� AI ��һ������ƶ���ģ����Ϣ��
    GetWorld()->GetTimerManager().SetTimer(RestartPatrolTimer, this, &AAINpcAIController::MoveToNextPatrolPoint, 0.5f, false);
    // ����������ȷ��ѭ��Ѳ��
    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
}

void AAINpcAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsHello)
    {
        // ���к�ʱ��Ҳ���ڹ���ʱ����
        // �������Ϣʱ���к�Ҳ����Ϣ
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
        // ����Ϣ�Ļ���ֻ��Ҫ����ʣ�����Ϣʱ��
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
        // ����Ҫ��Ϣ�Ļ���ʹ�ù���ʱ���ӳ�
        SplitTimer -= DeltaTime;
    }

    if (SplitTimer < 0.f)
    {
        /// �ֵ���Ϣʱ����
        SplitTimer = SplitRestTime;
        bIsNeedRest = true;
    }
}
