// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginHUD.h"
#include "LoginScreen.h"
#include "GameFramework/PlayerController.h"
#include "LoadingScreen.h"

void ALoginHUD::BeginPlay()
{
	Super::BeginPlay();
	AddLoginScreen();
}
void ALoginHUD::AddLoginScreen()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && LoginScreenClass)
	{
		LoginScreen = CreateWidget<ULoginScreen>(PlayerController, LoginScreenClass);
		LoginScreen->AddToViewport();
	}
}

void ALoginHUD::AddLoadingScreen()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && LoadingScreenClass)
	{
		LoadingScreen = CreateWidget<ULoadingScreen>(PlayerController, LoadingScreenClass);
		LoadingScreen->AddToViewport();
	}
}


