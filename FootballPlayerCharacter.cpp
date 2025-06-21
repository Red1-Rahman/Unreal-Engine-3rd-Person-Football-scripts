#include "FootballPlayerCharacter.h"
#include "FootballBallActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AFootballPlayerCharacter::AFootballPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    BallPossessed = nullptr;
}

void AFootballPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AFootballPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // If possessing the ball, keep it in front of the player
    if (BallPossessed)
    {
        FVector BallLocation = GetActorLocation() + GetActorForwardVector() * 100.f + FVector(0, 0, 50.f);
        BallPossessed->SetActorLocation(BallLocation);
    }
}

void AFootballPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis("MoveForward", this, &AFootballPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AFootballPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAction("Kick", IE_Pressed, this, &AFootballPlayerCharacter::KickBall);
    PlayerInputComponent->BindAction("PossessBall", IE_Pressed, this, &AFootballPlayerCharacter::TryPossessBall);
}

void AFootballPlayerCharacter::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AFootballPlayerCharacter::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AFootballPlayerCharacter::TryPossessBall()
{
    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * 200.f;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(50.f), Params))
    {
        AFootballBallActor* Ball = Cast<AFootballBallActor>(Hit.GetActor());
        if (Ball)
        {
            BallPossessed = Ball;
            Ball->SetPossessingPlayer(this);
        }
    }
}

void AFootballPlayerCharacter::KickBall()
{
    if (BallPossessed)
    {
        FVector KickDirection = GetActorForwardVector();
        BallPossessed->Kick(KickDirection * 1500.f);
        BallPossessed->SetPossessingPlayer(nullptr);
        BallPossessed = nullptr;
    }
}

void AFootballPlayerCharacter::OnBallLost()
{
    BallPossessed = nullptr;
}
