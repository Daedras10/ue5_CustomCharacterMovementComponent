// Fill out your copyright notice in the Description page of Project Settings.


#include "SprintCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "CustomCMC/Character/ACharacterWithSpecialCMC.h"
#include "GameFramework/Character.h"

bool USprintCharacterMovementComponent::FSavedMove_SprintCharacter::CanCombineWith(const FSavedMovePtr& NewMove,
                                                                                   ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_SprintCharacter* NewMyMove = static_cast<FSavedMove_SprintCharacter*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewMyMove->Saved_bWantsToSprint)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void USprintCharacterMovementComponent::FSavedMove_SprintCharacter::Clear()
{
	FSavedMove_Character::Clear();
	Saved_bWantsToSprint = 0;
}

uint8 USprintCharacterMovementComponent::FSavedMove_SprintCharacter::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

void USprintCharacterMovementComponent::FSavedMove_SprintCharacter::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	USprintCharacterMovementComponent* CharMov = Cast<USprintCharacterMovementComponent>(C->GetCharacterMovement());
	Saved_bWantsToSprint = CharMov->Safe_bWantsToSprint;
	Saved_bPrevWantsToCrouch = CharMov->bWantsToCrouch;
}

void USprintCharacterMovementComponent::FSavedMove_SprintCharacter::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);
	USprintCharacterMovementComponent* CharMov = Cast<USprintCharacterMovementComponent>(C->GetCharacterMovement());
	CharMov->Safe_bWantsToSprint = Saved_bWantsToSprint;
	CharMov->bWantsToCrouch = Saved_bPrevWantsToCrouch;
}

USprintCharacterMovementComponent::FNetworkPredictionData_Client_My::FNetworkPredictionData_Client_My(
	const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{
}

FSavedMovePtr USprintCharacterMovementComponent::FNetworkPredictionData_Client_My::AllocateNewMove()
{
	return MakeShared<FSavedMove_SprintCharacter>();
	// same as : return FSavedMovePtr(new FSavedMove_SprintCharacter());
}

USprintCharacterMovementComponent::USprintCharacterMovementComponent() : Safe_bWantsToSprint(false)
{
}

void USprintCharacterMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void USprintCharacterMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void USprintCharacterMovementComponent::CrouchPressed()
{
	bWantsToCrouch = !bWantsToCrouch;
}

bool USprintCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode Mode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == Mode;
}

FNetworkPredictionData_Client* USprintCharacterMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

		if (ClientPredictionData == nullptr)
		{
			USprintCharacterMovementComponent* MutableThis = const_cast<USprintCharacterMovementComponent*>(this);

			MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_My(*this);
			MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
			MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
		}
	return ClientPredictionData;
}

void USprintCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	Safe_bWantsToSprint = (Flags & FSavedMove_SprintCharacter::FLAG_Custom_0) != 0;
}

void USprintCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	if (MovementMode == MOVE_Walking)
		MaxWalkSpeed = Safe_bWantsToSprint ? Sprint_MaxWalkSpeed : Walk_MaxWalkSpeed;

	Safe_bPrevWantsToCrouch = bWantsToCrouch;
}

bool USprintCharacterMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

bool USprintCharacterMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && IsMovingOnGround();
}

void USprintCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	SpecialCharacterOwner = Cast<AACharacterWithSpecialCMC>(GetOwner());
}

void USprintCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_bPrevWantsToCrouch)
	{
		FHitResult PotentialSlideSurface;
		if (Velocity.SizeSquared() > pow(Slide_MinSpeed, 2) && GetSlideSurface(PotentialSlideSurface)) EnterSlide();
	}

	if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch) ExitSlide();
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void USprintCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
		case CMOVE_Slide:
			PhysSlide(deltaTime, Iterations);
			break;
		default:
			UE_LOG(LogTemp, Fatal, TEXT("Unknown CustomMovementMode"));
	}
}

void USprintCharacterMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	Velocity += Velocity.GetSafeNormal2D() * Slide_EnterImpulse;
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

void USprintCharacterMovementComponent::ExitSlide()
{
	bWantsToCrouch = false;

	FQuat Rotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, true, Hit);
	SetMovementMode(MOVE_Walking);
}

void USprintCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME) return;

	RestorePreAdditiveRootMotionVelocity();

	FHitResult SurfaceHit;
	if (!GetSlideSurface(SurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		ExitSlide();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Surface Gravity
	Velocity += Slide_GravityForce * FVector::DownVector * deltaTime;

	// Strafe
	if (FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > 0.5f)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	// Calc Velocity
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, Slide_Friction, true, GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(deltaTime);

	// Perform move
	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRotation = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	FVector Adjusted = Velocity * deltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).GetSafeNormal();
	FQuat Rotation = FRotationMatrix::MakeFromXZ(VelPlaneDir, SurfaceHit.Normal).ToQuat();
	SafeMoveUpdatedComponent(Adjusted, Rotation, true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	FHitResult NewSurfaceHit;
	if (!GetSlideSurface(NewSurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		ExitSlide();
	}

	// Update Outgoing Velocity & Acceleration
	if (bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
}

bool USprintCharacterMovementComponent::GetSlideSurface(FHitResult& Hit) const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + SpecialCharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	return GetWorld()->LineTraceSingleByProfile(Hit, Start, End, ProfileName, SpecialCharacterOwner->GetIgnoreCharacterParams());
}
