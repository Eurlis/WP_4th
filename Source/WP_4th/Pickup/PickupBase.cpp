#include "PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Test/WeaponTestCharacter.h"

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->SetSphereRadius(100.f);
	// Pawn 통과 + Visibility 채널은 Block (LineTrace 감지용)
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(InteractionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));

	PickupSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupSkeletalMesh"));
	PickupSkeletalMesh->SetupAttachment(InteractionSphere);
	PickupSkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupSkeletalMesh->SetCollisionProfileName(TEXT("NoCollision"));
}

void APickupBase::OnInteract(ACharacter* Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}

	AWeaponTestCharacter* TestChar = Cast<AWeaponTestCharacter>(Interactor);
	if (!TestChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pickup] Interactor is not WeaponTestCharacter"));
		return;
	}

	if (PickupWeaponID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pickup] PickupWeaponID is empty on %s"), *GetName());
		return;
	}

	switch (PickupKind)
	{
	case EPickupKind::Weapon:
		TestChar->SwitchWeaponByID(PickupWeaponID);
		break;
	case EPickupKind::Throwable:
		TestChar->AddGrenade(PickupWeaponID);
		break;
	case EPickupKind::Ammo:
		// TODO Phase 2: AddAmmo 구현
		UE_LOG(LogTemp, Warning, TEXT("[Pickup] Ammo pickup not yet implemented"));
		return;
	}

	Destroy();
}

FString APickupBase::GetInteractionPrompt() const
{
	return FString::Printf(TEXT("Pickup [%s]"), *PickupWeaponID.ToString());
}

bool APickupBase::CanInteract(ACharacter* Interactor) const
{
	return Interactor != nullptr && !PickupWeaponID.IsNone();
}
