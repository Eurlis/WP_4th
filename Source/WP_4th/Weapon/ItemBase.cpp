// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetMobility(EComponentMobility::Movable);
	PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	PickupCollision->SetupAttachment(PickupMesh);
	PickupCollision->SetSphereRadius(80.f);
	PickupCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ItemState = EItemState::Dropped;
	OwningCharacter = nullptr;
}

void AItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AItemBase, ItemState);
	DOREPLIFETIME(AItemBase, OwningCharacter);
}

void AItemBase::OnPickedUp(ACharacter* NewOwner)
{
	if (!HasAuthority()) return;

	OwningCharacter = NewOwner;
	SetItemState(EItemState::Equipped);

	SetOwner(NewOwner);
	AttachToActor(NewOwner, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetVisibility(false);
}

void AItemBase::OnDropped()
{
	if (!HasAuthority()) return;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	OwningCharacter = nullptr;
	SetItemState(EItemState::Dropped);

	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupMesh->SetVisibility(true);
}

void AItemBase::OnRep_ItemState()
{
	switch (ItemState)
	{
	case EItemState::Dropped:
		PickupMesh->SetVisibility(true);
		break;
	case EItemState::Equipped:
		PickupMesh->SetVisibility(false);
		break;
	case EItemState::Hidden:
		PickupMesh->SetVisibility(false);
		break;
	}
}

void AItemBase::SetItemState(EItemState NewState)
{
	ItemState = NewState;
	OnRep_ItemState();
}
