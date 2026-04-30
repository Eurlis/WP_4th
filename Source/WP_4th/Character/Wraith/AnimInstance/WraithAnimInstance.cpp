// Fill out your copyright notice in the Description page of Project Settings.

#include "WraithAnimInstance.h"
#include "Character/Wraith/Wraith.h"
#include "KismetAnimationLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Weapon/WeaponBase.h"

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
	bIsInAir = OwnerCharacter->GetCharacterMovement()->IsFalling();
	bIsSliding = OwnerCharacter->bIsSliding;
	SlideSpeed = Speed;
	SlidePhase = OwnerCharacter->SlideAnimationPhase;
	bIsSlideEntering = SlidePhase == ESlideAnimationPhase::Enter;
	bIsSlideExiting = SlidePhase == ESlideAnimationPhase::Exit;
	if (bIsSlideExiting)
	{
	}

	// Left hand IK: 총의 left_hand_socket → 캐릭터 메쉬 컴포넌트 공간으로 변환
	AWeaponBase* Weapon = OwnerCharacter->CurrentWeapon;
	
	if (Weapon)
	{
		WeaponType = Weapon->Category;
	}
	else
	{
		WeaponType = EWeaponType::None;
	}
	USkeletalMeshComponent* FPMesh = OwnerCharacter->GetFirstPersonMesh();
	if (Weapon && Weapon->WeaponMesh1P && FPMesh)
	{
		// 왼손 총기 Attach
		FTransform SocketWorldL = Weapon->WeaponMesh1P->GetSocketTransform(FName("left_hand_socket"));
		LeftHandLocation = SocketWorldL.GetRelativeTransform(FPMesh->GetComponentTransform()).GetLocation();
		
	}
}
