#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "FootballPlayerCharacter.h"
#include "FootballBallActor.generated.h"

class AFootballPlayerCharacter;

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBallPossessionChanged, AFootballPlayerCharacter*, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallKicked, FVector, KickDirection, float, KickPower);

UCLASS()
class FOOTBALLGAME_API AFootballBallActor : public AActor
{
	GENERATED_BODY()

public:
	AFootballBallActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** Components */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	/** Ball Physics Properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float BallRadius = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float BallMass = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float BounceDamping = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float RollingFriction = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball Properties")
	float MaxSpeed = 3000.0f;

	/** Possession Logic */
	UPROPERTY(BlueprintReadOnly, Category = "Possession")
	AFootballPlayerCharacter* PossessingPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession")
	float PossessionRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Possession")
	bool bCanBePossessed = true;

	/** Ball State */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsInAir;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsRolling;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	FVector LastKickDirection;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float LastKickPower;

	/** Ball Control Functions */
	UFUNCTION(BlueprintCallable, Category = "Ball Control")
	void Kick(const FVector& Force);

	UFUNCTION(BlueprintCallable, Category = "Ball Control")
	void KickBall(FVector Direction, float Power);

	UFUNCTION(BlueprintCallable, Category = "Ball Control")
	void PassBall(AFootballPlayerCharacter* TargetPlayer, float Power = 1000.0f);

	UFUNCTION(BlueprintCallable, Category = "Ball Control")
	void ShootBall(FVector TargetLocation, float Power = 1500.0f);

	UFUNCTION(BlueprintCallable, Category = "Ball Control")
	void StopBall();

	/** Possession Functions */
	UFUNCTION(BlueprintCallable, Category = "Possession")
	void SetPossessingPlayer(AFootballPlayerCharacter* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Possession")
	void ReleaseBall();

	UFUNCTION(BlueprintCallable, Category = "Possession")
	bool CanBePickedUp() const;

	UFUNCTION(BlueprintCallable, Category = "Possession")
	AFootballPlayerCharacter* GetPossessingPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "Possession")
	AFootballPlayerCharacter* GetNearestPlayer() const;

	/** Physics and Collision */
	UFUNCTION(BlueprintCallable, Category = "Physics")
	void ApplyCustomPhysics(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Physics")
	void HandleBounce(const FHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Physics")
	void ApplyGroundFriction(float DeltaTime);

	/** Utility */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	float GetBallSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	FVector GetBallVelocity() const;

	UFUNCTION(BlueprintCallable, Category = "Utility")
	void ResetBallPosition(FVector NewPosition);

	/** Events */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBallPossessionChanged OnBallPossessionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBallKicked OnBallKicked;

protected:
	/** Collision Handlers */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Internal */
	void UpdateBallState();
	void CheckForNearbyPlayers();
	float CalculateKickAccuracy(FVector Direction, float Power) const;

private:
	/** Timers */
	FTimerHandle BallStateUpdateTimer;
	FTimerHandle PossessionCheckTimer;

	/** Internal State */
	float TimeInAir = 0.0f;
	float TimeOnGround = 0.0f;
	FVector PreviousVelocity;
	bool bWasInAirLastFrame = false;
};
