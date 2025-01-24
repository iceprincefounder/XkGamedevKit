// Copyright ©ICEPRINCE. All Rights Reserved.

#include "XkGameMode.h"
#include "XkCamera.h"
#include "XkCharacter.h"
#include "XkController.h"
#include "XkGameState.h"
#include "UObject/ConstructorHelpers.h"


AXkGameMode::AXkGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// use our custom Pawn class
	DefaultPawnClass = AXkTopDownCamera::StaticClass();
	// use our custom PlayerController class
	PlayerControllerClass = AXkController::StaticClass();
	// use our custom GameState class
	GameStateClass = AXkGameState::StaticClass();
}