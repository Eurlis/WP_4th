#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "PickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UDataTable;

UENUM(BlueprintType)
enum class EPickupKind : uint8
{
	Weapon    UMETA(DisplayName = "Weapon"),
	Throwable UMETA(DisplayName = "Throwable"),
	Ammo      UMETA(DisplayName = "Ammo"),
};

UCLASS()
class WP_4TH_API APickupBase : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	APickupBase();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	EPickupKind PickupKind = EPickupKind::Weapon;

	// DataTable Row 이름 (WeaponBase::InitFromDataTable 에 그대로 전달)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FName PickupWeaponID;

	// DT_Weapons (BP 디폴트로 할당) — 메시/카테고리 자동 로드 소스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	USphereComponent* InteractionSphere;

	// 무기는 PickupSkeletalMesh 사용, 수류탄은 PickupMesh(Static) 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	UStaticMeshComponent* PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	USkeletalMeshComponent* PickupSkeletalMesh;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void RefreshFromDataTable();

	// === IInteractableInterface ===
	virtual void OnInteract(ACharacter* Interactor) override;
	virtual FString GetInteractionPrompt() const override;
	virtual bool CanInteract(ACharacter* Interactor) const override;

protected:
	virtual void BeginPlay() override;
};
