// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LoginHUD.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALoginHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> LoginScreenClass;

	void AddLoginScreen();

	UPROPERTY()
	class ULoginScreen* LoginScreen;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> LoadingScreenClass;
	
	void AddLoadingScreen();

	UPROPERTY()
	class ULoadingScreen* LoadingScreen;

protected:
	virtual void BeginPlay() override;

};
