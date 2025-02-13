// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "LoginGameState.generated.h"

/**
 * 
 */

UCLASS()
class BLASTER_API ALoginGameState : public AGameState
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing = OnRep_PlayersNum)
	int32 PlayersNum = 0;

	// �������ˣ�����Ƿ������˶��ѵ�¼
	void CheckAllPlayersReady();

protected:

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ��������Ҷ�������ɺ󣬴����˺���
	UFUNCTION()
	void OnRep_PlayersNum();

	
};
