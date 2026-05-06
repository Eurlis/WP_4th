#include "PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Test/WeaponTestCharacter.h"
#include "Weapon/WeaponData.h"
#include "Weapon/WeaponTypes.h"

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

void APickupBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshFromDataTable();
}

void APickupBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshFromDataTable();
}

void APickupBase::RefreshFromDataTable()
{
	if (!WeaponDataTable || PickupWeaponID.IsNone())
	{
		return;
	}

	const FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(
		PickupWeaponID, TEXT("PickupBase::RefreshFromDataTable"));
	if (!Data)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickupBase] DT row not found: %s"),
			*PickupWeaponID.ToString());
		return;
	}

	const bool bIsThrowable = (Data->Category == EWeaponType::Throwable);
	PickupKind = bIsThrowable ? EPickupKind::Throwable : EPickupKind::Weapon;

	if (bIsThrowable)
	{
		if (PickupMesh)
		{
			PickupMesh->SetStaticMesh(Data->ThrowableMesh);
			PickupMesh->SetRelativeScale3D(Data->ThrowableMeshScale);
			PickupMesh->SetRelativeRotation(Data->ThrowableMeshRotation);
			PickupMesh->SetVisibility(Data->ThrowableMesh != nullptr);
		}
		if (PickupSkeletalMesh)
		{
			PickupSkeletalMesh->SetSkeletalMesh(nullptr);
			PickupSkeletalMesh->SetVisibility(false);
		}
	}
	else
	{
		if (PickupSkeletalMesh)
		{
			PickupSkeletalMesh->SetSkeletalMesh(Data->WeaponMesh3P);
			PickupSkeletalMesh->SetRelativeScale3D(Data->MeshScale);
			PickupSkeletalMesh->SetRelativeRotation(Data->MeshRotation);
			PickupSkeletalMesh->SetRelativeLocation(Data->MeshLocationOffset);
			PickupSkeletalMesh->SetVisibility(Data->WeaponMesh3P != nullptr);
		}
		if (PickupMesh)
		{
			PickupMesh->SetStaticMesh(nullptr);
			PickupMesh->SetVisibility(false);
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("[PickupBase] Refreshed: %s, Kind: %d"),
		*PickupWeaponID.ToString(), (int32)PickupKind);
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

FText APickupBase::GetInteractionPromptText() const
{
	if (PickupWeaponID.IsNone() || !WeaponDataTable)
	{
		return NSLOCTEXT("Interaction", "DefaultPickup", "줍기");
	}

	const FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(
		PickupWeaponID, TEXT("PickupBase::GetInteractionPromptText"));

	if (!Data)
	{
		return FText::FromName(PickupWeaponID);
	}

	const FString Combined = FString::Printf(TEXT("%s 줍기"), *Data->DisplayName.ToString());
	return FText::FromString(Combined);
}

bool APickupBase::CanInteract(ACharacter* Interactor) const
{
	if (!Interactor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanInteract] FAIL: Interactor NULL on %s"), *GetName());
		return false;
	}

	if (PickupWeaponID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CanInteract] FAIL: PickupWeaponID is None on %s (Kind: %d) — check BP default value"),
			*GetName(), (int32)PickupKind);
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[CanInteract] PASS: %s, Kind: %d, ID: %s"),
		*GetName(), (int32)PickupKind, *PickupWeaponID.ToString());
	return true;
}
