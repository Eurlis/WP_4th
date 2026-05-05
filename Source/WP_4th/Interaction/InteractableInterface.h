#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class WP_4TH_API IInteractableInterface
{
	GENERATED_BODY()

public:
	// 상호작용 실행 (서버에서만 호출되어야 함)
	virtual void OnInteract(class ACharacter* Interactor) = 0;

	// HUD에 표시할 prompt 텍스트
	virtual FString GetInteractionPrompt() const { return TEXT("Interact"); }

	// 상호작용 가능 여부 (필요 시 거리/상태 검증)
	virtual bool CanInteract(class ACharacter* Interactor) const { return true; }
};
