// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginScreen.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ULoginScreen : public UUserWidget
{
	GENERATED_BODY()
public:	
	UPROPERTY(meta = (BindWidget))
	class UEditableText* UserName;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* Password;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InfoText;

	UFUNCTION(BlueprintCallable)
	void CheckIsLoginSucessful();
};
