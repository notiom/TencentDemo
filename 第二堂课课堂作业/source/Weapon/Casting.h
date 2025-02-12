// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Casting.generated.h"

UCLASS()
class BLASTER_API ACasting : public AActor
{
	GENERATED_BODY()
	
public:	
	ACasting();

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

private:
	UPROPERTY(VisibleAnyWhere)
	UStaticMeshComponent* CasingMesh;
	UPROPERTY(EditAnyWhere)
	float ShellEjectionImpulse;

	UPROPERTY(EditAnyWhere)
	class USoundCue* ShellSound;
};
