#include "FootballGameGameModeBase.h"
#include "FootballPlayerCharacter.h"
#include "FootballPlayerController.h"

AFootballGameGameModeBase::AFootballGameGameModeBase()
{
	DefaultPawnClass = AFootballPlayerCharacter::StaticClass();
	PlayerControllerClass = AFootballPlayerController::StaticClass();
}
