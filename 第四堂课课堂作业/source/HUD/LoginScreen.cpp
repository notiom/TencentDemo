// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginScreen.h"
#include "Components/TextBlock.h"
#include "Components/EditableText.h"
#include "LoginHUD.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/GameMode/LoginGameMode.h"
#include "Blaster/GameState/LoginGameState.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/PlayerController/LoginPlayerController.h"
void ULoginScreen::CheckIsLoginSucessful()
{

    FString Username = UserName->GetText().ToString();

    FString PassWord = Password->GetText().ToString();

    if (Username.IsEmpty() || PassWord.IsEmpty())
    {
        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("Username or Password is not empty!")));
        }
        return;
    }

    // ������Լ���ʵ�ʵ���֤�߼��������ѯ������
    if (Username == "TestUser" && PassWord == "12345")
    {
        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("sucessful!")));
        }
        RemoveFromParent(); // �رյ�ǰ UI

        ALoginPlayerController* LoginPlayerController = Cast<ALoginPlayerController>(GetOwningPlayer());
        if (LoginPlayerController)
        {
            LoginPlayerController->ServerPlayerLoginComplete();
            ALoginHUD* LoginHUD = LoginPlayerController->GetHUD<ALoginHUD>();
            LoginHUD->AddLoadingScreen();
        }
        
    }
    else
    {
        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("Username or Password is error!")));
        }
    }
}
