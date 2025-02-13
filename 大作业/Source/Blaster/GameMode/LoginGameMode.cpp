// Fill out your copyright notice in the Description page of Project Settings.


#include "LoginGameMode.h"

void ALoginGameMode::ServerTeleport()
{
    FString MapName = TEXT("BlasterMap");
    FString TravelURL = MapName + TEXT("?listen");
    GetWorld()->ServerTravel(TravelURL);
}