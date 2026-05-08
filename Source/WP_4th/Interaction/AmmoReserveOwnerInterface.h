// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Weapon/WeaponTypes.h"
#include "AmmoReserveOwnerInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAmmoReserveOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

class WP_4TH_API IAmmoReserveOwnerInterface
{
	GENERATED_BODY()

public:
	virtual int32 GetReserveAmmo(EAmmoType Type) const = 0;

	virtual int32 AddAmmo(EAmmoType Type, int32 Count) = 0;

	virtual int32 ConsumeReserve(EAmmoType Type, int32 Needed) = 0;
};
