// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameState.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Components/TextBlock.h"
#include "Blaster/HUD/CharacterOverLay.h"
#include "Blaster/Suqare/SquareActor.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/HUD/Announcement.h"
#include "Blaster/GameMode/BlasterGameMode.h"

ABlasterGameState::ABlasterGameState()
{
	bReplicates = true;  // �������縴��
	PrimaryActorTick.bCanEverTick = true;
}

void ABlasterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterGameState, SquareOfAmount);
	DOREPLIFETIME(ABlasterGameState, ScoringPlayerStatData);
}

void ABlasterGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState != MatchState::InProgress) return;

	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
	if (HasAuthority() && BlasterPlayerController)
	{
		if (SpawnTimer < 0.f)
		{
			BlasterPlayerController->ServerCreateSquare();
			SpawnTimer = SplitTime;
			SquareOfAmount++;
		}
		SpawnTimer -= DeltaTime;
	}

}

FString ABlasterGameState::CenterAlignText(const FString& Text, int Width)
{
    int TextLen = Text.Len();
    if (TextLen >= Width) return Text;  // ���ַ���������ȣ�ֱ�ӷ���

    int Padding = Width - TextLen;
    int LeftPadding = Padding / 2;
    int RightPadding = Padding - LeftPadding;

    return FString::Printf(TEXT("%s%s%s"),
        *FString::ChrN(LeftPadding, TEXT(' ')), // ������ո�
        *Text,
        *FString::ChrN(RightPadding, TEXT(' ')) // �Ҳ����ո�
    );
}


FString ABlasterGameState::CenterAlignNumber(int Number, int Width)
{
    FString NumStr = FString::Printf(TEXT("%d"), Number);
    return CenterAlignText(NumStr, Width);
}

void ABlasterGameState::OnRep_ScoringPlayerStatData()
{
    FString InfoTextString;

    // ���ù̶��п��ɸ���ʵ�����������
    const int NameWidth = 20;
    const int ScoreWidth = 8;
    const int DefeatsWidth = 8;
    const int ScoreSquareWidth = 12;

    // ��ͷ
    InfoTextString.Append(FString::Printf(TEXT("%s %s %s %s\n"),
        *CenterAlignText(TEXT("Player"), NameWidth),
        *CenterAlignText(TEXT("Score"), ScoreWidth),
        *CenterAlignText(TEXT("Defeats"), DefeatsWidth),
        *CenterAlignText(TEXT("ScoreSquare"), ScoreSquareWidth)
    ));

    // �������ݲ���ʽ�����
    for (const FStatData& PlayerStatData : ScoringPlayerStatData)
    {
        InfoTextString.Append(FString::Printf(TEXT("%s %s %s %s\n"),
            *CenterAlignText(PlayerStatData.PlayerName, NameWidth),
            *CenterAlignNumber(static_cast<int>(PlayerStatData.Score), ScoreWidth),
            *CenterAlignNumber(PlayerStatData.Defeats, DefeatsWidth),
            *CenterAlignNumber(PlayerStatData.ScoreSquare, ScoreSquareWidth)
        ));
    }

    // ��ȡHUD������
    ABlasterPlayerController* LocalController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
    if (LocalController && LocalController->BlasterHUD)
    {
        LocalController->BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
    }
}

void ABlasterGameState::UpdateStatData()
{
 
     if (!HasAuthority()) return;

     ScoringPlayerStatData.Empty();

     for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
     {
	  ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
	  if (BlasterPlayer)
	  {
             APlayerState* PlayerState = BlasterPlayer->GetPlayerState<APlayerState>();
             if (PlayerState)
             {
				 FStatData NewPlayer;
                 NewPlayer.PlayerName = PlayerState->GetPlayerName();
                 NewPlayer.Score = BlasterPlayer->HUDScore;
                 NewPlayer.Defeats = BlasterPlayer->HUDDefeats;
                 NewPlayer.ScoreSquare = BlasterPlayer->HUDSquarePoint;

				 ScoringPlayerStatData.Add(NewPlayer);
             }
         }
     }

     // ֪ͨ���пͻ��˸���
	 OnRep_ScoringPlayerStatData();
}
