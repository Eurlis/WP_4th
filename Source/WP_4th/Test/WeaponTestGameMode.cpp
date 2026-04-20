// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponTestGameMode.h"
#include "WeaponTestCharacter.h"
#include "BulletPoolManager.h"
#include "Engine/World.h"

AWeaponTestGameMode::AWeaponTestGameMode()
{
	DefaultPawnClass = AWeaponTestCharacter::StaticClass();
	BulletPoolManagerClass = ABulletPoolManager::StaticClass();
	BulletPoolInstance = nullptr;
}

void AWeaponTestGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!BulletPoolManagerClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	BulletPoolInstance = World->SpawnActor<ABulletPoolManager>(
		BulletPoolManagerClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);

	if (BulletPoolInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TestGM] BulletPoolManager spawned"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TestGM] BulletPoolManager spawn failed!"));
	}
}
