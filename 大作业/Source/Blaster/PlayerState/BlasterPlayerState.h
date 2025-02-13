// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()

// 	存放玩家得分等的代码块
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	// 复制函数
	virtual void OnRep_Score() override;

	// Rep函数必须要用UFUNCTION宏标记
	UFUNCTION()
	virtual void OnRep_Defeats();

	UFUNCTION()
	virtual void OnRep_ScoreSquare();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	void AddToScoreSquare(int32 ScoreAmount);
private:

	// 加宏的意义是将其正确的初始化为nullptr
	// 使其成为真正意义上的零指针
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_ScoreSquare)
	int32 ScoreSquare;


};
