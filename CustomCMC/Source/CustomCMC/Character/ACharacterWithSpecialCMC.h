// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CustomCMC/CMC/SprintCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "ACharacterWithSpecialCMC.generated.h"

UCLASS()
class CUSTOMCMC_API AACharacterWithSpecialCMC : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AACharacterWithSpecialCMC(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	USprintCharacterMovementComponent* SprintCharacterMovementComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Blueprintable, Category = "Movement")
	FORCEINLINE class USprintCharacterMovementComponent* GetSprintMovementComponent() const { return SprintCharacterMovementComponent; }

	FCollisionQueryParams GetIgnoreCharacterParams() const;
};
