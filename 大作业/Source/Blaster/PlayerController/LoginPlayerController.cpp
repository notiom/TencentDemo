// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginPlayerController.h"
#include "Blaster/GameState/LoginGameState.h"
#include "Blaster/HUD/LoadingScreen.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/HUD/LoginHUD.h"
#include "Blaster/HUD/LoginScreen.h"

void ALoginPlayerController::BeginPlay()
{
    Super::BeginPlay();
    LoginGameState = GetWorld()->GetGameState<ALoginGameState>();
}

void ALoginPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (LoginGameState)
    {
        UpdateLoadingScreenHUD((float)(LoginGameState->PlayersNum / 3));
    }

}

void ALoginPlayerController::ServerPlayerLoginComplete_Implementation()
{
    if (!HasAuthority()) return; // 仅服务器执行

    // 标记该玩家已输入
    LoginGameState->PlayersNum++;

    LoginGameState->CheckAllPlayersReady();
}


void ALoginPlayerController::UpdateLoadingScreenHUD(float Percent)
{
    ALoginHUD* LoginHUD = GetHUD<ALoginHUD>();
    if (LoginHUD && LoginHUD->LoadingScreen)
    {
        LoginHUD->LoadingScreen->UpdateProgress(Percent);
    }

}

