// Fill out your copyright notice in the Description page of Project Settings.


#include "LoadingScreen.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void ULoadingScreen::UpdateProgress(float Percent)
{
	if (Progress && ProgressPercent)
	{
		Progress->SetPercent(Percent);
		FString PercentText = FString::Printf(TEXT("%d%"), FMath::CeilToInt(Percent * 100));
		ProgressPercent->SetText(FText::FromString(PercentText));
	}
}
