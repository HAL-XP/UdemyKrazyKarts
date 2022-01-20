// Fill out your copyright notice in the Description page of Project Settings.
#include "GoKart.h"

#include "Components/InputComponent.h"
#include "Engine/EngineTypes.h"
#include "Math/Quat.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();

	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * DeltaTime;

	UpdateRotation(DeltaTime);
	UpdateLocationFromVelocity(DeltaTime);
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

void AGoKart::UpdateRotation(float DeltaTime)
{
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * Steering;
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