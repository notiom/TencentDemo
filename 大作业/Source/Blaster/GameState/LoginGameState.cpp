// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginGameState.h"
#include "BlasterGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/GameMode/LoginGameMode.h"
#include "Blaster/PlayerController/LoginPlayerController.h"

// ȷ�������ڿͻ���ͬ��
void ALoginGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALoginGameState, PlayersNum);
}

void ALoginGameState::OnRep_PlayersNum()
{
    CheckAllPlayersReady();
}

void ALoginGameState::CheckAllPlayersReady()
{
    // ȷ��������Ҷ��ѵ�¼
    UE_LOG(LogTemp, Warning, TEXT("logingamestate34PlayerArray.Num:%d"), PlayerArray.Num());
    UE_LOG(LogTemp, Warning, TEXT("logingamestate35PlayerLoginStats.Num:%d"), PlayersNum);
    
    bool bAllReady = (PlayersNum == PlayerArray.Num());

    if (bAllReady)
    {
        ALoginGameMode* LoginGameMode = GetWorld()->GetAuthGameMode<ALoginGameMode>();
        if (LoginGameMode)
        {
            LoginGameMode->ServerTeleport();
        }
    }
}
