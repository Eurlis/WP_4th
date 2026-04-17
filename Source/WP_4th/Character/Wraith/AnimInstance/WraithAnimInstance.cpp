// Fill out your copyright notice in the Description page of Project Settings.

#include "WraithAnimInstance.h"
#include "Character/Wraith/Wraith.h"
#include "KismetAnimationLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"

void UWraithAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
		OwnerCharacter = Cast<AWraith>(TryGetPawnOwner());
	if (!OwnerCharacter) return;

	Speed         = OwnerCharacter->GetVelocity().Size2D();
	AnimDirection = UKismetAnimationLibrary::CalculateDirection(OwnerCharacter->GetVelocity(), OwnerCharacter->GetActorRotation());
	bIsWalking = Speed > 0.f && OwnerCharacter->GetCharacterMovement()->IsMovingOnGround();
	bIsCrouching = OwnerCharacter->bIsCrouched;
	bIsInAir     = OwnerCharacter->GetCharacterMovement()->IsFalling();
	bIsSliding   = OwnerCharacter->bIsSliding;
}
