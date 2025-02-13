// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "SquareActor.generated.h"

// 第一次命中后加分1分,销毁加2分
UCLASS()
class BLASTER_API ASquareActor : public AActor, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASquareActor();
	virtual void Tick(float DeltaTime) override;
protected:

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 销毁之后加5分
	// 双倍加10分
	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable)
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(Server, Reliable)
	void ServerHandleHit(AActor* OtherActor);

private:

	UPROPERTY(Replicated)
	bool bIsFirstCollision = true; // 是否是第一次碰撞

	UPROPERTY(ReplicatedUsing = OnRep_Scale)
	FVector ReplicatedScale;

	UFUNCTION()
	void OnRep_Scale();

	UPROPERTY(EditDefaultsOnly)
	int32 FirstScore = 1;

	UPROPERTY(EditDefaultsOnly)
	int32 SecondScore = 2;

	UPROPERTY(VisibleAnyWhere)
	UStaticMeshComponent* SquareMesh;

	UPROPERTY(EditAnyWhere)
	FColor SquareColor;

	UPROPERTY(EditAnyWhere)
	class USoundCue* HitSound;

};
