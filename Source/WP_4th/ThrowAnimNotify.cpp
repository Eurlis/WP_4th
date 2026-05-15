// Fill out your copyright notice in the Description page of Project Settings.


#include "ThrowAnimNotify.h"

#include "Character/ApexCharacterBase.h"

void UThrowAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	AApexCharacterBase* Character = MeshComp ? Cast<AApexCharacterBase>(MeshComp->GetOwner()) : nullptr;
	if (Character)
	{
		Character->OnThrowAnimNotify();
	}
}
