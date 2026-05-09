#include "PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Test/WeaponTestCharacter.h"
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
	// SnapToGround 는 RefreshFromDataTable 끝에서 호출됨
	// (BeginPlay 시점에는 PickupWeaponID 미설정일 수 있어 PickupKind 가 부정확)
}

void APickupBase::SnapToGround()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const FVector Start = Origin + FVector(0.f, 0.f, 100.f);
	const FVector End = Origin - FVector(0.f, 0.f, 500.f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PickupSnapToGround), false, this);

	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		// 활성 메시 컴포넌트 결정 (InteractionSphere 제외)
		USceneComponent* ActiveMesh = nullptr;
		if (PickupKind == EPickupKind::Weapon && PickupSkeletalMesh && PickupSkeletalMesh->GetSkeletalMeshAsset())
		{
			ActiveMesh = PickupSkeletalMesh;
		}
		else if (PickupMesh && PickupMesh->GetStaticMesh())
		{
			ActiveMesh = PickupMesh;
		}

		// 메시 Bounds 의 하단을 바닥에 맞춤 (피벗/스케일/회전 자동 반영)
		float PivotToBottom = 0.0f;
		if (ActiveMesh)
		{
			// 회전 변경 직후 Bounds 미갱신 방지 — 강제 재계산
			ActiveMesh->UpdateBounds();

			const FBoxSphereBounds B = ActiveMesh->Bounds;
			const float MeshBottomZ = B.Origin.Z - B.BoxExtent.Z;
			PivotToBottom = GetActorLocation().Z - MeshBottomZ;
		}

		// PickupKind 별 미세 보정값 결정
		float FinalSnapOffset = WeaponSnapGroundOffset;
		switch (PickupKind)
		{
		case EPickupKind::Ammo:      FinalSnapOffset = AmmoSnapGroundOffset; break;
		case EPickupKind::Throwable: FinalSnapOffset = ThrowableSnapGroundOffset; break;
		case EPickupKind::Weapon:
		default:                     FinalSnapOffset = WeaponSnapGroundOffset; break;
		}

		FVector NewLocation = Hit.ImpactPoint;
		NewLocation.Z += PivotToBottom + FinalSnapOffset;
		SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
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

	// DT Category 로 PickupKind 자동 결정 (BP Class Default 무시)
	if (Data->Category == EWeaponType::Ammo)
	{
		PickupKind = EPickupKind::Ammo;
	}
	else if (Data->Category == EWeaponType::Throwable)
	{
		PickupKind = EPickupKind::Throwable;
	}
	else
	{
		PickupKind = EPickupKind::Weapon;
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

			// 픽업 표시 전용 회전: BP 절대값 우선, false 면 DT MeshRotation (손 장착용)
			const FRotator FinalRot = bUsePickupAbsoluteRotation
				? PickupAbsoluteRotation
				: Data->MeshRotation;
			PickupSkeletalMesh->SetRelativeRotation(FinalRot);

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

	// 모든 메시/회전/PickupKind 설정 후 정확한 종류별 Offset 으로 바닥 snap
	if (HasAuthority())
	{
		SnapToGround();
	}
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

	bool bConsumed = false;

	switch (PickupKind)
	{
	case EPickupKind::Weapon:
		TestChar->ServerAddWeaponToSlot(PickupWeaponID);
		bConsumed = true;
		break;

	case EPickupKind::Throwable:
		// helper 직접 호출(서버 권한): 풀(MaxGrenadeCount) 도달 시 false → 픽업 미파괴
		bConsumed = TestChar->TryAddGrenadeAuth(PickupWeaponID);
		if (!bConsumed)
		{
			UE_LOG(LogTemp, Log, TEXT("[Pickup] Grenade stock full for %s, pickup not consumed"),
				*PickupWeaponID.ToString());
		}
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
