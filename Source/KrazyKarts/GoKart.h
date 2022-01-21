// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"


USTRUCT()
struct FGoKartMove
{
	GENERATED_BODY()
	
	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float Steering;

	UPROPERTY()
	float DeltaTime;

	UPROPERTY()
	float TimeStamp;
};

USTRUCT()
struct FGoKartMoveState
{
	GENERATED_BODY();

	UPROPERTY()
	FVector Velocity {};

	UPROPERTY()
	FGoKartMove LastMove;

	UPROPERTY()
	FTransform Transform;
};

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(EditAnywhere)
	float Mass = 1000.f; // kg
	UPROPERTY(EditAnywhere)
	float MaxDrivingForce = 10000.f; // Newtons // aiming for 10 m/s/s
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10.f;
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.f; // kg/m
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.015f;

	float Throttle{};
	float Steering{};
	FVector Velocity{};

	UPROPERTY(ReplicatedUsing=OnRep_ServerState)
	FGoKartMoveState ServerState;
	UFUNCTION()
	void OnRep_ServerState();

	FVector GetAirResistance();
	FVector GetRollingResistance();
	void UpdateLocationFromVelocity(float DeltaTime);
	void UpdateRotation(const float DeltaTime, const float inSteering);

	void MoveForward(float Val);
	void MoveRight(float Val);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(FGoKartMove Move);

	void SimulateMove(const FGoKartMove& Move);
};
