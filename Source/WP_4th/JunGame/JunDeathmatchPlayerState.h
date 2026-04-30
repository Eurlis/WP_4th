#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "JunDeathmatchPlayerState.generated.h"

UCLASS()
class WP_4TH_API AJunDeathmatchPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AJunDeathmatchPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void RegisterElimination();

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void RegisterDeath();

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void RegisterRespawn();

	UFUNCTION(BlueprintPure, Category = "Deathmatch")
	int32 GetEliminations() const { return Eliminations; }

	UFUNCTION(BlueprintPure, Category = "Deathmatch")
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category = "Deathmatch")
	int32 GetRespawnCount() const { return RespawnCount; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	int32 Eliminations;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	int32 Deaths;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	int32 RespawnCount;
};
