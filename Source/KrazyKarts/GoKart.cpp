// Fill out your copyright notice in the Description page of Project Settings.
#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/EngineTypes.h"
#include "Math/Quat.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Algo/RemoveIf.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGoKart::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoKart, ServerState);
}

FString GetRoleAsString(ENetRole inRole)
{
	switch (inRole)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR!";
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove CurrentMove{ CreateMove(DeltaTime) };
		SimulateMove(CurrentMove);
		UnackowledgedMoves.Add(CurrentMove);
		Server_SendMove(CurrentMove);
	}
	else if (GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
	{
		// We are the server and in control of the pawn
		FGoKartMove CurrentMove{ CreateMove(DeltaTime) };
		Server_SendMove(CurrentMove);
	}
	else if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(GetWorld(), FVector(0.f, 0.f, 100.f), GetRoleAsString(GetLocalRole()), this, FColor::White, DeltaTime);
}

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * Move.DeltaTime;

	UpdateRotation(Move.DeltaTime, Move.Steering);
	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime)
{
	FGoKartMove NewMove;
	NewMove.DeltaTime = DeltaTime;
	NewMove.Throttle = Throttle;
	NewMove.Steering = Steering;
	NewMove.TimeStamp = GetWorld()->TimeSeconds;
	
	return NewMove;
}

void AGoKart::ClearAknowledgeMoves(const FGoKartMove& inLastMove)
{
	//if (!HasAuthority())
	{
		UnackowledgedMoves.SetNum(Algo::RemoveIf(UnackowledgedMoves, [&inLastMove](FGoKartMove curMove)
			{
				return curMove.TimeStamp <= inLastMove.TimeStamp;
			}));
	}
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
	ClearAknowledgeMoves(ServerState.LastMove);

	for (const FGoKartMove& CurMove : UnackowledgedMoves)
	{
		SimulateMove(CurMove);
	}
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100.f;
	float NormalForceAcceleration = Mass * AccelerationDueToGravity;
	FVector Resistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForceAcceleration;
	return Resistance;
}

FVector AGoKart::GetAirResistance()
{
	FVector Resistance;
	Resistance = - Velocity.GetSafeNormal() *  Velocity.SizeSquared() * DragCoefficient;
	return Resistance;
}

void AGoKart::UpdateRotation(const float DeltaTime, const float inSteering)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * inSteering;
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
	AddActorLocalRotation(RotationDelta, true);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector DeltaLocation = Velocity * DeltaTime * 100.f;
	FHitResult OutSweepHitResult;
	AddActorWorldOffset(DeltaLocation, true, &OutSweepHitResult);
	if (OutSweepHitResult.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}


void AGoKart::MoveForward(float Val)
{
	Throttle = Val;
}

void AGoKart::MoveRight(float Val)
{
	Steering = Val;
}

bool AGoKart::Server_SendMove_Validate(const FGoKartMove& Move)
{
	return true;// FMath::Abs(Val) <= 1.0f;
}

void AGoKart::Server_SendMove_Implementation(const FGoKartMove& Move)
{
	SimulateMove(Move);
	ServerState.LastMove	= Move;
	ServerState.Transform	= GetActorTransform();
	ServerState.Velocity	= Velocity;
}