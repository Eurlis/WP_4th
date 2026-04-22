// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MotionWarping/Public/MotionWarpingComponent.h"
#include "PakousComponent.generated.h"

UENUM(BlueprintType)
enum class EVaultType : uint8
{
	OneHand,
	TwoHand,
};
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
	
	float WallHeight = 0.0f;
	bool TryParkour();
	
	UPROPERTY(EditDefaultsOnly, Category="Parkour")
	UAnimMontage* OneHandVaultMontage;
	
	UPROPERTY(EditDefaultsOnly, Category="Parkour")
	UAnimMontage* TwoHandVaultMontage;
	
private:
	bool bCanParkour = true;
	UFUNCTION()
	void TryVault(EVaultType VaultType);
	UFUNCTION()
	void OnVaultEnd(UAnimMontage* Montage, bool bInterrupted);
private:
	UPROPERTY()
	ACharacter* OwnerCharacter;
	void DetectWall();
	void ScanWallTop();
	void ScanWallEdge();
	void ScanLanding();
	void MeasureWall();
	FCollisionQueryParams WallTraceParams;
	
	FHitResult ScanHitResult;
	bool bFoundTop;
	bool bIsOnLand = false;
	FHitResult LastTopHit;
	
	UPROPERTY()
	USkeletalMeshComponent* PlayerMesh;
	
	// Motion Warping
	UPROPERTY()
	UMotionWarpingComponent* MotionWarpingComp;
};
