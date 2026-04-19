// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto	UMETA(DisplayName = "Full Auto"),
	Semi	UMETA(DisplayName = "Semi Auto"),
	Pump	UMETA(DisplayName = "Pump Action")
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	Light		UMETA(DisplayName = "Light"),
	Shotgun		UMETA(DisplayName = "Shotgun")
};
