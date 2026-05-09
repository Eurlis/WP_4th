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
	UPROPERTY(ReplicatedUsing = OnRep_PickupWeaponID, EditAnywhere, BlueprintReadOnly, Category = "Pickup")
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

	// PickupKind 별 바닥 snap 보정값 (cm). 메시 두께는 BoundingBox 자동 반영, 이 값은 미세 조정용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Snap")
	float WeaponSnapGroundOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Snap")
	float AmmoSnapGroundOffset = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Snap")
	float ThrowableSnapGroundOffset = 1.0f;

	// 픽업 표시 전용 절대 회전. true 면 DT MeshRotation 무시하고 이 값 사용. 손 장착 자세는 영향 없음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Rotation")
	bool bUsePickupAbsoluteRotation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup|Rotation")
	FRotator PickupAbsoluteRotation = FRotator(0.f, 0.f, 0.f);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_PickupWeaponID();

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void RefreshFromDataTable();

	// === IInteractableInterface ===
	virtual void OnInteract(ACharacter* Interactor) override;
	virtual FString GetInteractionPrompt() const override;
	virtual FText GetInteractionPromptText() const override;
	virtual bool CanInteract(ACharacter* Interactor) const override;

protected:
	virtual void BeginPlay() override;

	// 서버 권한에서 BeginPlay 시 바닥으로 LineTrace 후 위치 보정
	void SnapToGround();
};
