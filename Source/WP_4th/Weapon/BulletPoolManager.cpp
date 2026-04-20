// Fill out your copyright notice in the Description page of Project Settings.

#include "BulletPoolManager.h"
#include "ProjectileBase.h"
#include "BulletProjectile.h"
#include "Engine/World.h"

ABulletPoolManager::ABulletPoolManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ProjectileClass = ABulletProjectile::StaticClass();
	PoolSize = 50;
}

void ABulletPoolManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitializePool();
	}
}

void ABulletPoolManager::InitializePool()
{
	if (!ProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Pool.Reset();
	Pool.Reserve(PoolSize);

	for (int32 i = 0; i < PoolSize; i++)
	{
		AProjectileBase* NewProj = World->SpawnActor<AProjectileBase>(
			ProjectileClass,
			FVector(0.f, 0.f, -10000.f),
			FRotator::ZeroRotator,
			SpawnParams);

		if (NewProj)
		{
			NewProj->Deactivate();
			Pool.Add(NewProj);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Pool] Initialized %d projectiles"), Pool.Num());
}

AProjectileBase* ABulletPoolManager::GetProjectile()
{
	for (AProjectileBase* Proj : Pool)
	{
		if (Proj && !Proj->bIsActive)
		{
			UE_LOG(LogTemp, Log, TEXT("[Pool] Dispensed projectile, Active: %d/%d"), GetActiveCount() + 1, Pool.Num());
			return Proj;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Pool] No available projectile! Size=%d (모든 풀이 사용 중)"), Pool.Num());
	return nullptr;
}

void ABulletPoolManager::ReturnProjectile(AProjectileBase* Projectile)
{
	if (!Projectile) return;
	Projectile->Deactivate();
	UE_LOG(LogTemp, Log, TEXT("[Pool] Returned projectile, Active: %d/%d"), GetActiveCount(), Pool.Num());
}

int32 ABulletPoolManager::GetActiveCount() const
{
	int32 Count = 0;
	for (const AProjectileBase* Proj : Pool)
	{
		if (Proj && Proj->bIsActive) Count++;
	}
	return Count;
}
