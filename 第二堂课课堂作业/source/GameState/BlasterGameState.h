// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FStatData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	float Score;

	UPROPERTY(BlueprintReadWrite)
	int32 Defeats;

	UPROPERTY(BlueprintReadWrite)
	int32 ScoreSquare;
};

UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
public:

	ABlasterGameState();
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	/* myself*/
	UPROPERTY(Replicated)
	int32 SquareOfAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	class UStaticMesh* SquareRedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMesh* SquareWhiteMesh;

	UPROPERTY(ReplicatedUsing = OnRep_ScoringPlayerStatData)
	TArray<FStatData> ScoringPlayerStatData;

	UFUNCTION()
	void OnRep_ScoringPlayerStatData();

	void UpdateStatData();
protected:

	float SpawnTimer = 0.f;
	float SplitTime = 3.0f;
	virtual void Tick(float DeltaTime) override;
	/* myself*/
private:
	float TopScore = 0.f;
	FString CenterAlignText(const FString& Text, int Width);
	FString CenterAlignNumber(int Number, int Width);
	
};
