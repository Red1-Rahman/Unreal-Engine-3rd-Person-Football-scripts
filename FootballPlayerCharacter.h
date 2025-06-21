#pragma once
#include "GameFramework/Character.h"
#include "FootballBallActor.h"
#include "FootballPlayerCharacter.generated.h"

UCLASS()
class FOOTBALLGAME_API AFootballPlayerCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AFootballPlayerCharacter();
protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Ball possession
	void TryPossessBall();
	void KickBall();
	void MoveForward(float Value);
	void MoveRight(float Value);

public:
	void OnBallLost();

private:
	AFootballBallActor* BallPossessed;
};
