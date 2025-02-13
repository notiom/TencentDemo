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

	// 服务器端：检查是否所有人都已登录
	void CheckAllPlayersReady();

protected:

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 当所有玩家都输入完成后，触发此函数
	UFUNCTION()
	void OnRep_PlayersNum();

	
};
