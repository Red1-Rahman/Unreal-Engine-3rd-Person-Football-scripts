#pragma once
#include "GameFramework/PlayerController.h"
#include "FootballPlayerController.generated.h"

UCLASS()
class FOOTBALLGAME_API AFootballPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFootballPlayerController();
protected:
	virtual void SetupInputComponent() override;
};