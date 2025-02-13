// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreen.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ULoadingScreen : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* Progress;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ProgressPercent;

	void UpdateProgress(float Percent);

};
