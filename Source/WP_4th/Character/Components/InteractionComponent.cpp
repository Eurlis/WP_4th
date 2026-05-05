#include "InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Interaction/InteractableInterface.h"
#include "Engine/World.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 로컬 컨트롤(플레이어)만 LineTrace — 서버/AI/리모트 클라는 스킵
	const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !OwnerChar->IsLocallyControlled())
	{
		return;
	}

	TimeSinceLastTrace += DeltaTime;
	if (TimeSinceLastTrace < TraceInterval)
	{
		return;
	}
	TimeSinceLastTrace = 0.0f;
	PerformTrace();
}

bool UInteractionComponent::GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return false;
	}

	// FirstPersonCamera 컴포넌트가 있으면 우선 사용 (1인칭 정확)
	if (UCameraComponent* Cam = OwnerChar->FindComponentByClass<UCameraComponent>())
	{
		OutLocation = Cam->GetComponentLocation();
		OutRotation = Cam->GetComponentRotation();
		return true;
	}

	if (AController* PC = OwnerChar->GetController())
	{
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return true;
	}

	return false;
}

void UInteractionComponent::PerformTrace()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector ViewLoc;
	FRotator ViewRot;
	if (!GetViewPoint(ViewLoc, ViewRot))
	{
		SetCurrentInteractable(nullptr);
		return;
	}

	const FVector TraceEnd = ViewLoc + ViewRot.Vector() * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionTrace), false, GetOwner());
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLoc, TraceEnd, TraceChannel, Params);

	AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
	if (HitActor && HitActor->Implements<UInteractableInterface>())
	{
		SetCurrentInteractable(HitActor);
	}
	else
	{
		SetCurrentInteractable(nullptr);
	}
}

void UInteractionComponent::SetCurrentInteractable(AActor* NewInteractable)
{
	if (CurrentInteractable == NewInteractable)
	{
		return;
	}

	CurrentInteractable = NewInteractable;

	if (NewInteractable)
	{
		if (IInteractableInterface* Iface = Cast<IInteractableInterface>(NewInteractable))
		{
			CurrentPrompt = Iface->GetInteractionPrompt();
		}
		else
		{
			CurrentPrompt.Empty();
		}
	}
	else
	{
		CurrentPrompt.Empty();
	}

	OnInteractableChanged.Broadcast(NewInteractable);
}
