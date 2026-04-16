// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponTestGameMode.h"
#include "WeaponTestCharacter.h"

AWeaponTestGameMode::AWeaponTestGameMode()
{
	DefaultPawnClass = AWeaponTestCharacter::StaticClass();
}
