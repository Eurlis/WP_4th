// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

UENUM(BlueprintType)
enum class EItemState : uint8
{
	Dropped		UMETA(DisplayName = "Dropped"),
	Equipped	UMETA(DisplayName = "Equipped"),
	Hidden		UMETA(DisplayName = "Hidden")
};

UCLASS(Abstract)
class WP_4TH_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	AItemBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Components")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Components")
	class USphereComponent* PickupCollision;

	// --- State ---
	UPROPERTY(ReplicatedUsing = OnRep_ItemState, BlueprintReadOnly, Category = "Item|State")
	EItemState ItemState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|State")
	ACharacter* OwningCharacter;

	// --- Interface ---
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void OnPickedUp(ACharacter* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void OnDropped();

protected:
	UFUNCTION()
	void OnRep_ItemState();

	void SetItemState(EItemState NewState);
};
