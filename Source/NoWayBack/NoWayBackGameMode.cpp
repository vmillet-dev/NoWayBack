// Copyright Epic Games, Inc. All Rights Reserved.

#include "NoWayBackGameMode.h"
#include "NoWayBackCharacter.h"
#include "UObject/ConstructorHelpers.h"

ANoWayBackGameMode::ANoWayBackGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
