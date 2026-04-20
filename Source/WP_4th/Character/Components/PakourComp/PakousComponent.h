// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PakousComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UPakousComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPakousComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
public:
	bool bwallFoward;
	FHitResult wallHitResult;
	
	bool CanWallJump() const;
	
	FVector WallNormal;
	FVector WallNormalReversed;
	
	FHitResult LastTopHitResult;
	
	FVector VaultLandingLocation;
	bool bCanVault;
private:
	UPROPERTY()
	ACharacter* OwnerCharacter;
	
	void DetectWall();
	void ScanWallTop();
	void ScanWallEdge();
	void ScanLanding();
	
	FCollisionQueryParams WallTraceParams;
	
	FHitResult ScanHitResult;
	bool bFoundTop;
	
	FHitResult LastTopHit;
};
