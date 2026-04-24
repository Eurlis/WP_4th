// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/DataTable.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
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
	DOREPLIFETIME(AWeaponBase, bIsAiming);
	DOREPLIFETIME(AWeaponBase, bIsBursting);
	DOREPLIFETIME(AWeaponBase, CurrentBurstCount);
	DOREPLIFETIME(AWeaponBase, WeaponID);
}

// ==================== ADS ====================

void AWeaponBase::StartAiming()
{
	bIsAiming = true;
	ServerSetAiming(true);
}

void AWeaponBase::StopAiming()
{
	bIsAiming = false;
	ServerSetAiming(false);
}

void AWeaponBase::ServerSetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming;
}

float AWeaponBase::GetADSFOVMultiplier() const
{
	return ADSFOVMultiplier > 0.f ? ADSFOVMultiplier : 1.0f;
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !WeaponID.IsNone() && WeaponDataTable)
	{
		InitFromDataTable(WeaponID);
	}
}

// ==================== Data ====================

void AWeaponBase::InitFromDataTable(FName InWeaponID)
{
	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponBase] WeaponDataTable not set!"));
		return;
	}

	FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(InWeaponID, TEXT("WeaponBase InitFromDataTable"));
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("[WeaponBase] WeaponID '%s' not found!"), *InWeaponID.ToString());
		return;
	}

	WeaponID = InWeaponID;
	CurrentWeaponData = *Data;
	ApplyWeaponData(*Data);

	UE_LOG(LogTemp, Warning, TEXT("[WeaponBase] Init: %s (Dmg:%.1f Ammo:%d)"),
		*Data->DisplayName.ToString(), Data->BaseDamage, Data->MaxAmmo);
}

void AWeaponBase::ApplyWeaponData(const FWeaponData& Data)
{
	BaseDamage = Data.BaseDamage;
	HeadshotMultiplier = Data.HeadshotMultiplier;
	LegMultiplier = Data.LegMultiplier;
	FireRate = Data.FireRate;
	WeaponRange = Data.WeaponRange;
	MaxAmmo = Data.MaxAmmo;
	CurrentAmmo = Data.MaxAmmo;
	ReloadTime = Data.ReloadTime;
	FireMode = Data.FireMode;
	AmmoType = Data.AmmoType;
	BulletSpeed = Data.BulletSpeed;
	BulletGravityScale = Data.BulletGravityScale;
	RecoilPitchMin = Data.RecoilPitchMin;
	RecoilPitchMax = Data.RecoilPitchMax;
	RecoilYawMin = Data.RecoilYawMin;
	RecoilYawMax = Data.RecoilYawMax;
	RecoilRecoverySpeed = Data.RecoilRecoverySpeed;
	ADSFOVMultiplier = Data.ADSFOVMultiplier;

	if (Data.WeaponMesh1P && WeaponMesh1P)
	{
		WeaponMesh1P->SetSkeletalMesh(Data.WeaponMesh1P);
		WeaponMesh1P->SetRelativeScale3D(Data.MeshScale);
		WeaponMesh1P->SetRelativeRotation(Data.MeshRotation);
		WeaponMesh1P->SetRelativeLocation(Data.MeshLocationOffset);
	}

	if (Data.WeaponMesh3P && WeaponMesh3P)
	{
		WeaponMesh3P->SetSkeletalMesh(Data.WeaponMesh3P);
		WeaponMesh3P->SetRelativeScale3D(Data.MeshScale);
		WeaponMesh3P->SetRelativeRotation(Data.MeshRotation);
		WeaponMesh3P->SetRelativeLocation(Data.MeshLocationOffset);
	}
}

void AWeaponBase::OnRep_WeaponID()
{
	if (!WeaponID.IsNone())
	{
		InitFromDataTable(WeaponID);
	}
}

// ==================== Muzzle ====================

FVector AWeaponBase::GetMuzzleLocation() const
{
	static const FName MuzzleSocketName(TEXT("MuzzleSocket"));

	if (WeaponMesh1P && WeaponMesh1P->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh1P->GetSocketLocation(MuzzleSocketName);
	}

	if (WeaponMesh3P && WeaponMesh3P->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh3P->GetSocketLocation(MuzzleSocketName);
	}

	if (MuzzlePoint)
	{
		return MuzzlePoint->GetComponentLocation();
	}

	return GetActorLocation();
}

FVector AWeaponBase::GetMuzzleForward() const
{
	static const FName MuzzleSocketName(TEXT("MuzzleSocket"));

	if (WeaponMesh1P && WeaponMesh1P->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh1P->GetSocketRotation(MuzzleSocketName).Vector();
	}

	if (WeaponMesh3P && WeaponMesh3P->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh3P->GetSocketRotation(MuzzleSocketName).Vector();
	}

	if (MuzzlePoint)
	{
		return MuzzlePoint->GetForwardVector();
	}

	return GetActorForwardVector();
}

// ==================== Fire ====================

bool AWeaponBase::CanFireNow() const
{
	if (bIsReloading) return false;
	if (CurrentAmmo <= 0) return false;

	if (const UWorld* World = GetWorld())
	{
		const float TimeSinceLastFire = World->GetTimeSeconds() - LastFireTime;
		if (TimeSinceLastFire < FireRate) return false;
	}

	return true;
}

void AWeaponBase::StartFire()
{
	if (!CanFireNow()) return;

	// Burst 진행 중이면 재클릭 무시 (서버 권한 플래그; 리플리케이션 지연 창 없을 때 추가 안전망)
	if (FireMode == EFireMode::Burst && bIsBursting) return;

	bIsFiring = true;

	// Fire first shot immediately (gated by CanFireNow already)
	FireShot();

	if (FireMode == EFireMode::Auto)
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AWeaponBase::FireShot, FireRate, true);
	}
	else
	{
		// Semi / Pump / Burst: single press → single shot; next shot gated by FireRate on next StartFire
		// TODO(Burst): implement multi-shot burst with BurstShotCount / BurstInterval once DataTable fields added
		bIsFiring = false;
	}
}

void AWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void AWeaponBase::FireShot()
{
	if (!CanFireNow())
	{
		if (CurrentAmmo <= 0 || bIsReloading)
		{
			StopFire();
		}
		return;
	}

	// Mark local fire time so CanFireNow() gates subsequent presses/ticks on the client.
	// Server-side authoritative LastFireTime is set in ServerFire_Implementation.
	LastFireTime = GetWorld()->GetTimeSeconds();

	FVector MuzzleLoc = GetMuzzleLocation();
	UE_LOG(LogTemp, Log, TEXT("[Weapon] Fired at %.2f, Next available at %.2f (MuzzleLoc: %s)"),
		LastFireTime, LastFireTime + FireRate, *MuzzleLoc.ToString());

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
}

bool AWeaponBase::ServerFire_Validate(FVector MuzzleLocation, FVector AimDirection)
{
	return true;
}

void AWeaponBase::ServerFire_Implementation(FVector MuzzleLocation, FVector AimDirection)
{
	// NOTE: cooldown gate lives client-side in CanFireNow() — if we also gated the server here,
	// on a listen-server host the client's FireShot sets LastFireTime first, then this RPC self-blocks
	// in the same frame and CurrentAmmo never decrements (infinite ammo bug).
	if (bIsReloading || CurrentAmmo <= 0) return;

	// Burst 모드: 단발 소비 없이 서버 타이머로 N발 시퀀스 실행
	if (FireMode == EFireMode::Burst)
	{
		if (bIsBursting) return;
		StartBurstFire(MuzzleLocation, AimDirection);
		return;
	}

	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	LastFireTime = GetWorld()->GetTimeSeconds();

	UE_LOG(LogTemp, Warning, TEXT("[Ammo] Current: %d/%d"), CurrentAmmo, MaxAmmo);

	ProcessHit(MuzzleLocation, AimDirection);

	// 자동 재장전 체크
	if (CurrentAmmo <= 0 && !bIsReloading)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ammo] Empty - Auto reload"));

		// 풀오토 연사 멈추기
		StopFire();

		// 살짝 딜레이 후 재장전 (연사 막 끝난 프레임에 곧바로 장전 애니 밀리지 않게)
		FTimerHandle AutoReloadHandle;
		GetWorldTimerManager().SetTimer(
			AutoReloadHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (!bIsReloading && HasAuthority())
				{
					ServerStartReload_Implementation();
				}
			}),
			0.3f, false
		);
	}
}

// ==================== Burst ====================

void AWeaponBase::StartBurstFire(const FVector& MuzzleLocation, const FVector& AimDirection)
{
	if (!HasAuthority()) return;

	const int32 MaxBurst = CurrentWeaponData.BurstShotCount > 0 ? CurrentWeaponData.BurstShotCount : 3;
	const float Interval = CurrentWeaponData.BurstInterval > 0.f ? CurrentWeaponData.BurstInterval : 0.06f;

	UE_LOG(LogTemp, Warning, TEXT("[Burst] START - Count:%d, Interval:%.3fs"), MaxBurst, Interval);

	bIsBursting = true;
	CurrentBurstCount = 0;
	CachedBurstMuzzle = MuzzleLocation;
	CachedBurstDir = AimDirection;

	FireBurstShot();
}

void AWeaponBase::FireBurstShot()
{
	if (!HasAuthority())
	{
		EndBurstFire();
		return;
	}

	const int32 MaxBurst = CurrentWeaponData.BurstShotCount > 0 ? CurrentWeaponData.BurstShotCount : 3;
	const float Interval = CurrentWeaponData.BurstInterval > 0.f ? CurrentWeaponData.BurstInterval : 0.06f;

	if (bIsReloading)
	{
		EndBurstFire();
		return;
	}

	if (CurrentAmmo <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Burst] Ammo ran out at shot %d"), CurrentBurstCount);
		EndBurstFire();
		if (!bIsReloading)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Ammo] Empty - Auto reload"));
			ServerStartReload_Implementation();
		}
		return;
	}

	// 현재 에이밍 방향 재취득 (버스트 중 에임 이동 반영)
	FVector MuzzleLoc = GetMuzzleLocation();
	FVector AimDir = CachedBurstDir;
	if (OwningCharacter)
	{
		if (APlayerController* PC = Cast<APlayerController>(OwningCharacter->GetController()))
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			AimDir = CamRot.Vector();
		}
	}

	CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	LastFireTime = GetWorld()->GetTimeSeconds();
	CurrentBurstCount++;

	UE_LOG(LogTemp, Log, TEXT("[Burst] Shot %d/%d fired, Ammo:%d/%d"),
		CurrentBurstCount, MaxBurst, CurrentAmmo, MaxAmmo);

	ProcessHit(MuzzleLoc, AimDir);

	if (CurrentBurstCount < MaxBurst && CurrentAmmo > 0)
	{
		GetWorldTimerManager().SetTimer(BurstTimerHandle, this, &AWeaponBase::FireBurstShot, Interval, false);
	}
	else
	{
		const bool bOutOfAmmo = (CurrentAmmo <= 0);
		EndBurstFire();
		if (bOutOfAmmo && !bIsReloading)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Ammo] Empty - Auto reload"));
			ServerStartReload_Implementation();
		}
	}
}

void AWeaponBase::EndBurstFire()
{
	UE_LOG(LogTemp, Warning, TEXT("[Burst] END - Fired %d shots"), CurrentBurstCount);

	GetWorldTimerManager().ClearTimer(BurstTimerHandle);
	bIsBursting = false;
	CurrentBurstCount = 0;
}

// ==================== Hit ====================

void AWeaponBase::ProcessHit(const FVector& MuzzleLocation, const FVector& AimDirection)
{
	// Muzzle Flash + Fire Sound 브로드캐스트 (샷 당 1회, 산탄총 펠릿 수와 무관)
	MulticastSpawnMuzzleFlash(MuzzleLocation, AimDirection.Rotation());

	// DataTable 기반 산탄총 처리
	if (CurrentWeaponData.bIsShotgun)
	{
		const float SpreadRad = FMath::DegreesToRadians(CurrentWeaponData.SpreadAngle);
		for (int32 i = 0; i < CurrentWeaponData.PelletCount; i++)
		{
			FVector SpreadDir = FMath::VRandCone(AimDirection, SpreadRad);
			FireProjectile(MuzzleLocation, SpreadDir);
		}
		MulticastFireEffects(MuzzleLocation, MuzzleLocation + AimDirection * WeaponRange);
		return;
	}

	// 단발 (AR/Pistol 등)
	FireProjectile(MuzzleLocation, AimDirection);
	MulticastFireEffects(MuzzleLocation, MuzzleLocation + AimDirection * WeaponRange);
}

void AWeaponBase::FireProjectile(const FVector& MuzzleLocation, const FVector& Direction)
{
	UE_LOG(LogTemp, Warning, TEXT("[Fire] Muzzle: %s, AimDir: %s, Speed=%.1f"),
		*MuzzleLocation.ToString(), *Direction.ToString(), BulletSpeed);

	if (!BulletPool)
	{
		BulletPool = Cast<ABulletPoolManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ABulletPoolManager::StaticClass()));
	}

	if (!BulletPool)
	{
		UE_LOG(LogTemp, Error, TEXT("[Fire] BulletPool is NULL! Falling back to hitscan."));
	}
	else
	{
		AProjectileBase* Bullet = BulletPool->GetProjectile();
		if (!Bullet)
		{
			UE_LOG(LogTemp, Error, TEXT("[Fire] No available bullet in pool!"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Fire] Got bullet: %s"), *Bullet->GetName());

			// WeaponData 전달 (임팩트 이펙트 정보 포함)
			Bullet->SetWeaponData(CurrentWeaponData);

			Bullet->Activate(MuzzleLocation, Direction, BaseDamage, BulletSpeed, BulletGravityScale, OwningCharacter);

			const FVector ActualVel = Direction.GetSafeNormal() * BulletSpeed;
			UE_LOG(LogTemp, Warning, TEXT("[Fire] Bullet activated at %s with velocity %s"),
				*Bullet->GetActorLocation().ToString(), *ActualVel.ToString());

			if (Bullet->BulletMesh)
			{
				UE_LOG(LogTemp, Log, TEXT("[Fire] Bullet mesh visible: %s, StaticMesh=%s"),
					Bullet->BulletMesh->IsVisible() ? TEXT("YES") : TEXT("NO"),
					Bullet->BulletMesh->GetStaticMesh() ? *Bullet->BulletMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Fire] Bullet BulletMesh component is NULL!"));
			}
			return;
		}
	}

	// 폴백: 풀 없거나 고갈 시 히트스캔
	FHitResult HitResult;
	PerformLineTrace(MuzzleLocation, Direction, HitResult);

	FVector TraceEnd = MuzzleLocation + Direction * WeaponRange;
	if (HitResult.bBlockingHit)
	{
		TraceEnd = HitResult.ImpactPoint;
		ApplyDamage(HitResult, BaseDamage);
	}

	DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
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

	UE_LOG(LogTemp, Warning, TEXT("[Reload] Started - duration: %.2fs"), ReloadTime);

	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AWeaponBase::FinishReload, ReloadTime, false);
}

void AWeaponBase::FinishReload()
{
	CurrentAmmo = MaxAmmo;
	bIsReloading = false;

	UE_LOG(LogTemp, Warning, TEXT("[Reload] Finished - Ammo: %d/%d"), CurrentAmmo, MaxAmmo);
}

// ==================== Equip ====================

void AWeaponBase::OnEquipped()
{
	WeaponMesh1P->SetVisibility(true);
	WeaponMesh3P->SetVisibility(true);

	MulticastPlayEquipSound();
}

void AWeaponBase::OnUnequipped()
{
	StopFire();

	// 버스트 진행 중이었다면 즉시 중단
	GetWorldTimerManager().ClearTimer(BurstTimerHandle);
	bIsBursting = false;
	CurrentBurstCount = 0;

	WeaponMesh1P->SetVisibility(false);
	WeaponMesh3P->SetVisibility(false);
}

// ==================== Effects ====================

void AWeaponBase::MulticastFireEffects_Implementation(FVector MuzzleLocation, FVector TraceEnd)
{
	// Debug line (visible for 1 second)
#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Red, false, 1.0f, 0, 1.0f);
#endif
}

void AWeaponBase::MulticastSpawnMuzzleFlash_Implementation(FVector MuzzleLoc, FRotator MuzzleRot)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Muzzle Flash (Niagara)
	if (CurrentWeaponData.MuzzleFlashFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			CurrentWeaponData.MuzzleFlashFX,
			MuzzleLoc,
			MuzzleRot
		);
	}

	// Fire Sound
	if (CurrentWeaponData.FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			CurrentWeaponData.FireSound,
			MuzzleLoc
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[MuzzleFlash] Spawned at %s"), *MuzzleLoc.ToString());
}

void AWeaponBase::MulticastPlayEquipSound_Implementation()
{
	if (!CurrentWeaponData.EquipSound || !OwningCharacter) return;

	if (OwningCharacter->IsLocallyControlled())
	{
		UGameplayStatics::PlaySound2D(
			OwningCharacter,
			CurrentWeaponData.EquipSound,
			1.0f
		);
	}
	else
	{
		UGameplayStatics::SpawnSoundAttached(
			CurrentWeaponData.EquipSound,
			OwningCharacter->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			false,
			1.0f, 1.0f, 0.0f,
			nullptr, nullptr,
			true
		);
	}
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
