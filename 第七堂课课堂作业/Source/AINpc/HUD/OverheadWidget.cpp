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


void UOverheadWidget::ShowInformation(FString ir)
{
	SetDisplayText(ir);
}



void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();

	Super::NativeDestruct();
}