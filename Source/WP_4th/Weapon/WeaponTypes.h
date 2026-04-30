// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto	UMETA(DisplayName = "Full Auto"),
	Semi	UMETA(DisplayName = "Semi Auto"),
	Pump	UMETA(DisplayName = "Pump Action"),
	Burst	UMETA(DisplayName = "Burst")
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	Light		UMETA(DisplayName = "Light"),
	Shotgun		UMETA(DisplayName = "Shotgun"),
	Heavy		UMETA(DisplayName = "Heavy"),
	Energy		UMETA(DisplayName = "Energy"),
	Sniper		UMETA(DisplayName = "Sniper")
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None        UMETA(DisplayName = "Unarmed"),
	Rifle       UMETA(DisplayName = "Rifle"),
	Pistol      UMETA(DisplayName = "Pistol"),
	Shotgun     UMETA(DisplayName = "Shotgun"),
	Sniper      UMETA(DisplayName = "Sniper"),
	Throwable   UMETA(DisplayName = "Throwable")

};
