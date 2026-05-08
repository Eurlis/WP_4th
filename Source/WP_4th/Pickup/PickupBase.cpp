#include "PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Character/ApexCharacterBase.h"
#include "Weapon/WeaponData.h"
#include "Weapon/WeaponTypes.h"
#include "Interaction/AmmoReserveOwnerInterface.h"

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

void APickupBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickupBase, PickupWeaponID);
}

void APickupBase::OnRep_PickupWeaponID()
{
	RefreshFromDataTable();
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

	switch (Data->Category)
	{
	case EWeaponType::Throwable:
		PickupKind = EPickupKind::Throwable;
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
		break;

	case EWeaponType::Ammo:
		PickupKind = EPickupKind::Ammo;
		if (PickupMesh)
		{
			PickupMesh->SetStaticMesh(Data->PickupStaticMesh);
			PickupMesh->SetRelativeScale3D(FVector(1.f));
			PickupMesh->SetRelativeRotation(FRotator::ZeroRotator);
			PickupMesh->SetVisibility(Data->PickupStaticMesh != nullptr);
		}
		if (PickupSkeletalMesh)
		{
			PickupSkeletalMesh->SetSkeletalMesh(nullptr);
			PickupSkeletalMesh->SetVisibility(false);
		}
		break;

	default:
		PickupKind = EPickupKind::Weapon;
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
		break;
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

	AApexCharacterBase* TestChar = Cast<AApexCharacterBase>(Interactor);
	if (!TestChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pickup] Interactor is not ApexCharacterBase"));
		return;
	}

	if (PickupWeaponID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pickup] PickupWeaponID is empty on %s"), *GetName());
		return;
	}

	bool bConsumed = false;

	switch (PickupKind)
	{
	case EPickupKind::Weapon:
		TestChar->ServerAddWeaponToSlot(PickupWeaponID);
		bConsumed = true;
		break;

	case EPickupKind::Throwable:
		TestChar->AddGrenade(PickupWeaponID);
		bConsumed = true;
		break;

	case EPickupKind::Ammo:
	{
		const FWeaponData* Data = WeaponDataTable
			? WeaponDataTable->FindRow<FWeaponData>(PickupWeaponID, TEXT("PickupBase::OnInteract"))
			: nullptr;
		if (!Data)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Pickup] Ammo DT row missing: %s"),
				*PickupWeaponID.ToString());
			return;
		}

		IAmmoReserveOwnerInterface* AmmoOwner = Cast<IAmmoReserveOwnerInterface>(Interactor);
		if (!AmmoOwner)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Pickup] Interactor doesn't implement IAmmoReserveOwnerInterface"));
			return;
		}

		const int32 Added = AmmoOwner->AddAmmo(Data->AmmoType, Data->PickupAmmoCount);
		if (Added > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[Pickup] Ammo +%d %s"),
				Added, *UEnum::GetValueAsString(Data->AmmoType));
			bConsumed = true;
		}
		else
		{
			// Apex 동작: Reserve가 가득이면 픽업 그대로 둠
			UE_LOG(LogTemp, Log, TEXT("[Pickup] Ammo full, pickup not consumed"));
		}
		break;
	}
	}

	if (bConsumed)
	{
		Destroy();
	}
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

	FString Combined;
	if (PickupKind == EPickupKind::Ammo)
	{
		Combined = FString::Printf(TEXT("%s +%d 줍기"),
			*Data->DisplayName.ToString(), Data->PickupAmmoCount);
	}
	else
	{
		Combined = FString::Printf(TEXT("%s 줍기"), *Data->DisplayName.ToString());
	}
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
