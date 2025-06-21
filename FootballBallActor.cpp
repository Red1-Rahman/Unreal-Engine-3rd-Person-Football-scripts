#include "FootballBallActor.h"
#include "FootballPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

AFootballBallActor::AFootballBallActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Collision and Mesh
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;

    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    BallMesh->SetupAttachment(RootComponent);

    // Movement
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->InitialSpeed = 0.0f;
    ProjectileMovement->MaxSpeed = 3000.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.6f;
    ProjectileMovement->Friction = 0.1f;
    ProjectileMovement->ProjectileGravityScale = 1.0f;

    // Ball parameters
    BallRadius = 22.0f;
    BallMass = 0.45f;
    PossessionRadius = 150.0f;

    // State
    PossessingPlayer = nullptr;
    CurrentOwner = nullptr;
    bCanBePossessed = true;
    bIsInAir = false;
    bIsRolling = false;
    bWasInAirLastFrame = false;
    TimeInAir = 0.0f;
    TimeOnGround = 0.0f;
    LastKickPower = 0.0f;

    // Collision settings
    CollisionSphere->SetSphereRadius(BallRadius);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionProfileName("BlockAll");

    CollisionSphere->OnComponentHit.AddDynamic(this, &AFootballBallActor::OnHit);
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AFootballBallActor::OnBeginOverlap);
}

void AFootballBallActor::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimer(BallStateUpdateTimer, this, &AFootballBallActor::UpdateBallState, 0.1f, true);
    GetWorldTimerManager().SetTimer(PossessionCheckTimer, this, &AFootballBallActor::CheckForNearbyPlayers, 0.2f, true);
}

void AFootballBallActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ApplyCustomPhysics(DeltaTime);

    if (bIsInAir)
    {
        TimeInAir += DeltaTime;
        TimeOnGround = 0.0f;
    }
    else
    {
        TimeOnGround += DeltaTime;
        TimeInAir = 0.0f;
    }

    PreviousVelocity = GetBallVelocity();
}

void AFootballBallActor::Kick(const FVector& Force)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = Force;
        ProjectileMovement->Activate();
    }

    if (PossessingPlayer)
    {
        PossessingPlayer->OnBallLost();
        PossessingPlayer = nullptr;
    }

    ReleaseBall();
}

void AFootballBallActor::KickBall(FVector Direction, float Power)
{
    if (!bCanBePossessed) return;

    Direction.Normalize();
    FVector KickVelocity = Direction * Power;
    KickVelocity.Z += FMath::Max(100.0f, Power * 0.3f);

    ProjectileMovement->Velocity = KickVelocity;
    LastKickDirection = Direction;
    LastKickPower = Power;

    if (CurrentOwner)
    {
        ReleaseBall();
    }

    OnBallKicked.Broadcast(Direction, Power);
    UE_LOG(LogTemp, Log, TEXT("Ball kicked with power: %f"), Power);
}

void AFootballBallActor::PassBall(AFootballPlayerCharacter* TargetPlayer, float Power)
{
    if (!TargetPlayer) return;

    FVector PassDirection = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
    float Distance = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
    float AdjustedPower = FMath::Clamp(Power * (Distance / 500.0f), 300.0f, 1200.0f);

    KickBall(PassDirection, AdjustedPower);
}

void AFootballBallActor::ShootBall(FVector TargetLocation, float Power)
{
    FVector ShootDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();
    float Accuracy = CalculateKickAccuracy(ShootDirection, Power);

    FVector RandomOffset = FVector(
        FMath::RandRange(-Accuracy, Accuracy),
        FMath::RandRange(-Accuracy, Accuracy),
        0.0f
    );

    ShootDirection += RandomOffset;
    ShootDirection.Normalize();

    KickBall(ShootDirection, Power);
}

void AFootballBallActor::StopBall()
{
    ProjectileMovement->Velocity = FVector::ZeroVector;
    ProjectileMovement->StopMovementImmediately();
}

void AFootballBallActor::SetPossessingPlayer(AFootballPlayerCharacter* NewOwner)
{
    PossessingPlayer = NewOwner;
    CurrentOwner = NewOwner;

    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
    }

    if (CurrentOwner)
    {
        bCanBePossessed = false;
    }
    else
    {
        bCanBePossessed = true;
    }

    OnBallPossessionChanged.Broadcast(CurrentOwner);
}

AFootballPlayerCharacter* AFootballBallActor::GetPossessingPlayer() const
{
    return PossessingPlayer;
}

void AFootballBallActor::ReleaseBall()
{
    SetPossessingPlayer(nullptr);
}

bool AFootballBallActor::CanBePickedUp() const
{
    return bCanBePossessed && !CurrentOwner && GetBallSpeed() < 200.0f;
}

AFootballPlayerCharacter* AFootballBallActor::GetNearestPlayer() const
{
    TArray<AActor*> FoundPlayers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFootballPlayerCharacter::StaticClass(), FoundPlayers);

    AFootballPlayerCharacter* NearestPlayer = nullptr;
    float NearestDistance = FLT_MAX;

    for (AActor* Actor : FoundPlayers)
    {
        if (AFootballPlayerCharacter* Player = Cast<AFootballPlayerCharacter>(Actor))
        {
            float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
            if (Distance < NearestDistance && Distance <= PossessionRadius)
            {
                NearestDistance = Distance;
                NearestPlayer = Player;
            }
        }
    }

    return NearestPlayer;
}

void AFootballBallActor::ApplyCustomPhysics(float DeltaTime)
{
    if (!CurrentOwner && ProjectileMovement)
    {
        if (!bIsInAir && GetBallSpeed() > 0.1f)
        {
            ApplyGroundFriction(DeltaTime);
        }

        if (ProjectileMovement->Velocity.Size() > MaxSpeed)
        {
            ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * MaxSpeed;
        }
    }
}

void AFootballBallActor::HandleBounce(const FHitResult& HitResult)
{
    FVector IncomingVelocity = ProjectileMovement->Velocity;
    FVector SurfaceNormal = HitResult.Normal;

    float BounceReduction = FMath::Clamp(BounceDamping, 0.1f, 1.0f);
    FVector BounceVelocity = IncomingVelocity - 2.0f * FVector::DotProduct(IncomingVelocity, SurfaceNormal) * SurfaceNormal;

    ProjectileMovement->Velocity = BounceVelocity * BounceReduction;
}

void AFootballBallActor::ApplyGroundFriction(float DeltaTime)
{
    FVector CurrentVelocity = ProjectileMovement->Velocity;
    FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);

    if (HorizontalVelocity.Size() > 0.1f)
    {
        FVector FrictionForce = -HorizontalVelocity.GetSafeNormal() * RollingFriction * 981.0f;
        FVector NewVelocity = HorizontalVelocity + FrictionForce * DeltaTime;

        if (FVector::DotProduct(HorizontalVelocity, NewVelocity) < 0.0f)
        {
            NewVelocity = FVector::ZeroVector;
        }

        ProjectileMovement->Velocity = FVector(NewVelocity.X, NewVelocity.Y, CurrentVelocity.Z);
    }
}

float AFootballBallActor::GetBallSpeed() const
{
    return ProjectileMovement ? ProjectileMovement->Velocity.Size() : 0.0f;
}

FVector AFootballBallActor::GetBallVelocity() const
{
    return ProjectileMovement ? ProjectileMovement->Velocity : FVector::ZeroVector;
}

void AFootballBallActor::ResetBallPosition(FVector NewPosition)
{
    SetActorLocation(NewPosition);
    StopBall();
    ReleaseBall();
}

void AFootballBallActor::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    HandleBounce(Hit);

    if (AFootballPlayerCharacter* Player = Cast<AFootballPlayerCharacter>(OtherActor))
    {
        if (!CurrentOwner && CanBePickedUp())
        {
            Player->TryPossessBall(this);
        }
    }
}

void AFootballBallActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AFootballPlayerCharacter* Player = Cast<AFootballPlayerCharacter>(OtherActor))
    {
        if (!CurrentOwner && CanBePickedUp())
        {
            Player->TryPossessBall(this);
        }
    }
}

void AFootballBallActor::UpdateBallState()
{
    FVector Velocity = GetBallVelocity();
    bool bCurrentlyInAir = FMath::Abs(Velocity.Z) > 10.0f || !ProjectileMovement->IsMovingOnGround();

    if (bCurrentlyInAir != bWasInAirLastFrame)
    {
        bIsInAir = bCurrentlyInAir;
        bIsRolling = !bCurrentlyInAir && Velocity.Size2D() > 10.0f;
        bWasInAirLastFrame = bCurrentlyInAir;

        UE_LOG(LogTemp, Log, TEXT("Ball state changed - InAir: %s, Rolling: %s"), 
            bIsInAir ? TEXT("true") : TEXT("false"),
            bIsRolling ? TEXT("true") : TEXT("false"));
    }
}

void AFootballBallActor::CheckForNearbyPlayers()
{
    if (!CurrentOwner && CanBePickedUp())
    {
        AFootballPlayerCharacter* NearestPlayer = GetNearestPlayer();
        if (NearestPlayer)
        {
            NearestPlayer->TryPossessBall(this);
        }
    }
}

float AFootballBallActor::CalculateKickAccuracy(FVector Direction, float Power) const
{
    float PowerFactor = FMath::Clamp(Power / MaxSpeed, 0.0f, 1.0f);
    return PowerFactor * 0.2f;
}
