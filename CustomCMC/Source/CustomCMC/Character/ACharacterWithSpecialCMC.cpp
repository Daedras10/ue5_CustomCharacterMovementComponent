// Fill out your copyright notice in the Description page of Project Settings.


#include "ACharacterWithSpecialCMC.h"

// Sets default values
AACharacterWithSpecialCMC::AACharacterWithSpecialCMC(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USprintCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AACharacterWithSpecialCMC::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AACharacterWithSpecialCMC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AACharacterWithSpecialCMC::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

FCollisionQueryParams AACharacterWithSpecialCMC::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;
	TArray<AActor*> Childs;
	GetAllChildActors(Childs);
	Params.AddIgnoredActors(Childs);
	Params.AddIgnoredActor(this);
	return Params;
}

