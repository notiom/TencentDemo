// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString textToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(textToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* inPawn)
{
	ENetRole localRole = inPawn->GetLocalRole();
	FString role;
	switch (localRole)
	{
	case ENetRole::ROLE_Authority:
		role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		role = FString("None");
		break;
	}

	FString localRoleString = FString::Printf(TEXT("local Role : %s"), *role);
	SetDisplayText(localRoleString);
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();
}
