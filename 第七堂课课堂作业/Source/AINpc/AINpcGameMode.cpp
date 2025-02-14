// Copyright Epic Games, Inc. All Rights Reserved.

#include "AINpcGameMode.h"
#include "AINpcCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAINpcGameMode::AAINpcGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
