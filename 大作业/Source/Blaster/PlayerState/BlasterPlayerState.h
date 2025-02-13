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

// 	�����ҵ÷ֵȵĴ����
public:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	// ���ƺ���
	virtual void OnRep_Score() override;

	// Rep��������Ҫ��UFUNCTION����
	UFUNCTION()
	virtual void OnRep_Defeats();

	UFUNCTION()
	virtual void OnRep_ScoreSquare();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);
	void AddToScoreSquare(int32 ScoreAmount);
private:

	// �Ӻ�������ǽ�����ȷ�ĳ�ʼ��Ϊnullptr
	// ʹ���Ϊ���������ϵ���ָ��
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY(ReplicatedUsing = OnRep_ScoreSquare)
	int32 ScoreSquare;


};
