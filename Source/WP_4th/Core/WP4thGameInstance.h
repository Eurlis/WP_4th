#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WP4thGameInstance.generated.h"

class USessionManager;

UCLASS()
class WP_4TH_API UWP4thGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UWP4thGameInstance();

	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category="Session")
	USessionManager* GetSessionManager() const { return SessionManager; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Session")
	TObjectPtr<USessionManager> SessionManager;
};
