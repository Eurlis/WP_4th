// Fill out your copyright notice in the Description page of Project Settings.

#include "ThrowableBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

AThrowableBase::AThrowableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = PickupMesh;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bAutoActivate = false;

	ThrowForce = 2000.f;
	FuseTime = 2.0f;
	ExplosionDamage = 80.f;
	ExplosionRadius = 500.f; // 5m = 500 UU (1m ≈ 100 UU)
}

void AThrowableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AThrowableBase, WeaponID);
}

void AThrowableBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !WeaponID.IsNone() && WeaponDataTable)
	{
		InitFromDataTable(WeaponID);
	}
}

// ==================== Data ====================

void AThrowableBase::InitFromDataTable(FName InWeaponID)
{
	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[ThrowableBase] WeaponDataTable not set!"));
		return;
	}

	FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(InWeaponID, TEXT("ThrowableBase InitFromDataTable"));
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("[ThrowableBase] WeaponID '%s' not found!"), *InWeaponID.ToString());
		return;
	}

	WeaponID = InWeaponID;
	CurrentWeaponData = *Data;
	ApplyThrowableData(*Data);

	UE_LOG(LogTemp, Warning, TEXT("[ThrowableBase] Init: %s (Dmg:%.1f Radius:%.1f Fuse:%.2f)"),
		*Data->DisplayName.ToString(), Data->ExplosionDamage, Data->ExplosionRadius, Data->FuseTime);
}

void AThrowableBase::ApplyThrowableData(const FWeaponData& Data)
{
	ThrowForce = Data.ThrowForce;
	FuseTime = Data.FuseTime;
	ExplosionDamage = Data.ExplosionDamage;
	ExplosionRadius = Data.ExplosionRadius;

	if (Data.ThrowableMesh && PickupMesh)
	{
		PickupMesh->SetStaticMesh(Data.ThrowableMesh);
		PickupMesh->SetRelativeScale3D(Data.ThrowableMeshScale);
		PickupMesh->SetRelativeRotation(Data.ThrowableMeshRotation);
	}
}

void AThrowableBase::OnRep_WeaponID()
{
	if (!WeaponID.IsNone())
	{
		InitFromDataTable(WeaponID);
	}
}

bool AThrowableBase::ServerThrow_Validate(FVector ThrowDirection)
{
	return true;
}

void AThrowableBase::ServerThrow_Implementation(FVector ThrowDirection)
{
	// Detach from owner
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetItemState(EItemState::Dropped);
	PickupMesh->SetVisibility(true);
	PickupMesh->SetSimulatePhysics(false);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Activate projectile movement
	FVector LaunchVelocity = ThrowDirection.GetSafeNormal() * ThrowForce;
	ProjectileMovement->Velocity = LaunchVelocity;
	ProjectileMovement->Activate();

	// Start fuse timer
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AThrowableBase::Explode, FuseTime, false);
}

void AThrowableBase::Explode()
{
	if (!HasAuthority()) return;

	FVector ExplosionLocation = GetActorLocation();

	// Radial damage with falloff
	TArray<AActor*> IgnoredActors;
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		ExplosionDamage,								// BaseDamage
		ExplosionDamage * 0.1f,							// MinimumDamage
		ExplosionLocation,								// Origin
		ExplosionRadius * 0.3f,							// DamageInnerRadius
		ExplosionRadius,								// DamageOuterRadius
		1.f,											// DamageFalloff
		nullptr,										// DamageTypeClass
		IgnoredActors,									// IgnoreActors
		this,											// DamageCauser
		OwningCharacter ? OwningCharacter->GetInstigatorController() : nullptr	// InstigatedBy
	);

	MulticastExplosionEffects(ExplosionLocation);

	// Debug sphere
	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 16, FColor::Yellow, false, 2.0f);

	Destroy();
}

void AThrowableBase::MulticastExplosionEffects_Implementation(FVector ExplosionLocation)
{
	// TODO: Explosion particle, explosion sound
	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 16, FColor::Yellow, false, 2.0f);
}
