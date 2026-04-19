// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "BulletPoolManager.h"
#include "ProjectileBase.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 1P Mesh (OwnerOnly)
	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	WeaponMesh1P->SetupAttachment(GetRootComponent());
	WeaponMesh1P->SetOnlyOwnerSee(true);
	WeaponMesh1P->CastShadow = false;

	// 3P Mesh (OwnerNoSee)
	WeaponMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh3P"));
	WeaponMesh3P->SetupAttachment(GetRootComponent());
	WeaponMesh3P->SetOwnerNoSee(true);

	// Muzzle Point
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh3P);

	// Default Stats
	BaseDamage = 10.f;
	HeadshotMultiplier = 2.0f;
	LegMultiplier = 0.75f;
	FireRate = 0.1f;
	WeaponRange = 10000.f;
	MaxAmmo = 30;
	ReloadTime = 2.0f;
	FireMode = EFireMode::Auto;
	AmmoType = EAmmoType::Light;

	// Projectile Defaults
	BulletSpeed = 30000.f;
	BulletGravityScale = 0.3f;
	BulletPool = nullptr;

	// Recoil Defaults
	RecoilPitchMin = -0.3f;
	RecoilPitchMax = -0.5f;
	RecoilYawMin = -0.1f;
	RecoilYawMax = 0.1f;
	RecoilRecoverySpeed = 5.f;

	// ADS
	ADSFOVMultiplier = 0.75f;

	// Runtime
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;
	bIsFiring = false;
	LastFireTime = 0.f;
	CurrentRecoilPitch = 0.f;
	CurrentRecoilYaw = 0.f;
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RecoverRecoil(DeltaTime);
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponBase, CurrentAmmo);
	DOREPLIFETIME(AWeaponBase, bIsReloading);
	DOREPLIFETIME(AWeaponBase, bIsFiring);
}

// ==================== Fire ====================

void AWeaponBase::StartFire()
{
	if (bIsReloading || CurrentAmmo <= 0) return;

	bIsFiring = true;

	// Fire first shot immediately
	FireShot();

	if (FireMode == EFireMode::Auto)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AWeaponBase::FireShot, FireRate, true);
	}
}

void AWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AWeaponBase::FireShot()
{
	if (bIsReloading || CurrentAmmo <= 0)
	{
		StopFire();
		return;
	}

	FVector MuzzleLoc = MuzzlePoint->GetComponentLocation();
	FVector AimDir;

	// Get aim direction from controller
	if (OwningCharacter)
	{
		APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController());
		if (PC)
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			AimDir = CamRot.Vector();
		}
		else
		{
			AimDir = OwningCharacter->GetActorForwardVector();
		}
	}
	else
	{
		AimDir = GetActorForwardVector();
	}

	// Call Server RPC
	ServerFire(MuzzleLoc, AimDir);

	// Local recoil
	ApplyRecoil();

	// Semi / Pump: one shot only
	if (FireMode == EFireMode::Semi || FireMode == EFireMode::Pump)
	{
		StopFire();
	}
}

bool AWeaponBase::ServerFire_Validate(FVector MuzzleLocation, FVector AimDirection)
{
	return true;
}

void AWeaponBase::ServerFire_Implementation(FVector MuzzleLocation, FVector AimDirection)
{
	if (bIsReloading || CurrentAmmo <= 0) return;

	CurrentAmmo--;
	LastFireTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Warning, TEXT("[Weapon] Fire! Ammo: %d / %d"), CurrentAmmo, MaxAmmo);

	ProcessHit(MuzzleLocation, AimDirection);

	// 자동 재장전 체크
	if (CurrentAmmo <= 0 && !bIsReloading)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Weapon] Auto-reload triggered!"));

		// 풀오토 연사 멈추기
		StopFire();

		// 재장전 시작 (서버 권한 직접 실행)
		ServerStartReload_Implementation();
	}
}

void AWeaponBase::ProcessHit(const FVector& MuzzleLocation, const FVector& AimDirection)
{
	// Pool 매니저 지연 바인딩
	if (!BulletPool)
	{
		BulletPool = Cast<ABulletPoolManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ABulletPoolManager::StaticClass()));
	}

	if (BulletPool)
	{
		AProjectileBase* Bullet = BulletPool->GetProjectile();
		if (Bullet)
		{
			Bullet->Activate(MuzzleLocation, AimDirection, BaseDamage, BulletSpeed, BulletGravityScale, OwningCharacter);
			// 대표 trace end: 탄도 궤적 시각화는 풀에서 DrawDebugLine 처리
			MulticastFireEffects(MuzzleLocation, MuzzleLocation + AimDirection * WeaponRange);
			return;
		}
	}

	// 폴백: 풀이 없거나 소진된 경우 히트스캔
	FHitResult HitResult;
	PerformLineTrace(MuzzleLocation, AimDirection, HitResult);

	FVector TraceEnd = MuzzleLocation + AimDirection * WeaponRange;

	if (HitResult.bBlockingHit)
	{
		TraceEnd = HitResult.ImpactPoint;
		ApplyDamage(HitResult, BaseDamage);
	}

	DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
	MulticastFireEffects(MuzzleLocation, TraceEnd);
}

void AWeaponBase::PerformLineTrace(const FVector& Start, const FVector& Direction, FHitResult& OutHit) const
{
	FVector End = Start + Direction * WeaponRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (OwningCharacter)
	{
		QueryParams.AddIgnoredActor(OwningCharacter);
	}
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);
}

void AWeaponBase::ApplyDamage(const FHitResult& HitResult, float Damage)
{
	if (!HasAuthority()) return;

	AActor* HitActor = HitResult.GetActor();
	if (!HitActor) return;

	float FinalDamage = Damage;
	FName BoneName = HitResult.BoneName;

	// Body part multiplier
	if (BoneName == FName("head"))
	{
		FinalDamage *= HeadshotMultiplier;

		// TODO: ClientShowHitMarker(true) — 캐릭터 베이스에서 구현 예정
	}
	else if (BoneName == FName("thigh_l") || BoneName == FName("thigh_r") ||
	         BoneName == FName("calf_l") || BoneName == FName("calf_r") ||
	         BoneName == FName("foot_l") || BoneName == FName("foot_r"))
	{
		FinalDamage *= LegMultiplier;

		// TODO: ClientShowHitMarker(false) — 캐릭터 베이스에서 구현 예정
	}
	else
	{
		// TODO: ClientShowHitMarker(false) — 캐릭터 베이스에서 구현 예정
	}

	// TODO: ServerApplyDamage(FinalDamage, OwningCharacter, HitResult) — 캐릭터 베이스에서 구현 예정
	// 임시로 UE 기본 데미지 시스템 사용
	UGameplayStatics::ApplyPointDamage(
		HitActor,
		FinalDamage,
		HitResult.TraceEnd - HitResult.TraceStart,
		HitResult,
		OwningCharacter ? OwningCharacter->GetInstigatorController() : nullptr,
		this,
		nullptr
	);
}

// ==================== Reload ====================

void AWeaponBase::StartReload()
{
	if (bIsReloading || CurrentAmmo >= MaxAmmo) return;
	ServerStartReload();
}

bool AWeaponBase::ServerStartReload_Validate()
{
	return true;
}

void AWeaponBase::ServerStartReload_Implementation()
{
	if (bIsReloading || CurrentAmmo >= MaxAmmo) return;

	bIsReloading = true;
	StopFire();

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponBase::FinishReload, ReloadTime, false);
}

void AWeaponBase::FinishReload()
{
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;
}

// ==================== Equip ====================

void AWeaponBase::OnEquipped()
{
	WeaponMesh1P->SetVisibility(true);
	WeaponMesh3P->SetVisibility(true);
}

void AWeaponBase::OnUnequipped()
{
	StopFire();
	WeaponMesh1P->SetVisibility(false);
	WeaponMesh3P->SetVisibility(false);
}

// ==================== Effects ====================

void AWeaponBase::MulticastFireEffects_Implementation(FVector MuzzleLocation, FVector TraceEnd)
{
	// Debug line (visible for 1 second)
	DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);

	// TODO: Muzzle flash particle, impact particle, fire sound
}

// ==================== Recoil ====================

void AWeaponBase::ApplyRecoil()
{
	if (!OwningCharacter) return;

	APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController());
	if (!PC) return;

	float PitchRecoil = FMath::FRandRange(RecoilPitchMin, RecoilPitchMax);
	float YawRecoil = FMath::FRandRange(RecoilYawMin, RecoilYawMax);

	CurrentRecoilPitch += PitchRecoil;
	CurrentRecoilYaw += YawRecoil;

	PC->AddPitchInput(PitchRecoil);
	PC->AddYawInput(YawRecoil);
}

void AWeaponBase::RecoverRecoil(float DeltaTime)
{
	if (bIsFiring) return;

	if (FMath::Abs(CurrentRecoilPitch) > KINDA_SMALL_NUMBER || FMath::Abs(CurrentRecoilYaw) > KINDA_SMALL_NUMBER)
	{
		float RecoverPitch = FMath::FInterpTo(CurrentRecoilPitch, 0.f, DeltaTime, RecoilRecoverySpeed);
		float RecoverYaw = FMath::FInterpTo(CurrentRecoilYaw, 0.f, DeltaTime, RecoilRecoverySpeed);

		float PitchDelta = RecoverPitch - CurrentRecoilPitch;
		float YawDelta = RecoverYaw - CurrentRecoilYaw;

		CurrentRecoilPitch = RecoverPitch;
		CurrentRecoilYaw = RecoverYaw;

		if (OwningCharacter)
		{
			APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController());
			if (PC)
			{
				PC->AddPitchInput(PitchDelta);
				PC->AddYawInput(YawDelta);
			}
		}
	}
}

// ==================== RepNotify ====================

void AWeaponBase::OnRep_CurrentAmmo()
{
	// UI update hook — 블루프린트에서 바인딩 가능
}
