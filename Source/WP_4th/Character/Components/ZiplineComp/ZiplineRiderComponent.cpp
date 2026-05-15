// Fill out your copyright notice in the Description page of Project Settings.


#include "ZiplineRiderComponent.h"

#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Zipline/ZiplineActor.h"


// Sets default values for this component's properties
UZiplineRiderComponent::UZiplineRiderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UZiplineRiderComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar) MoveComp = OwnerChar->GetCharacterMovement();

	// ...

}

void UZiplineRiderComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UZiplineRiderComponent, bIsRiding);
	DOREPLIFETIME(UZiplineRiderComponent, CurrentZipline);

}

void UZiplineRiderComponent::TryInterract()
{
	if (bIsRiding)
	{
		Server_Detach(true);
		return;
	}

	AZiplineActor* Found = nullptr;
	float Dist =0.f;
	int8 Dir = 1;

	if (FindNearestZipline(Found, Dist, Dir))
		Server_Attach(Found, Dist, Dir);

}

void UZiplineRiderComponent::RequestAttach(AZiplineActor* Zipline)
{
	if (!Zipline || bIsRiding || !OwnerChar) return;

	USplineComponent* Spline = Zipline->GetSplineComp();
	const FVector PlayerPos = OwnerChar->GetActorLocation();

	const float key = Spline->FindInputKeyClosestToWorldLocation(PlayerPos);
	const float Dist = Spline->GetDistanceAlongSplineAtSplineInputKey(key);

	const FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World).GetSafeNormal();

	const int8 Dir =(FVector::DotProduct(OwnerChar->GetActorForwardVector(), Tangent) >= 0.f) ? 1 : -1;

	Server_Attach(Zipline, Dist, Dir);
}

void UZiplineRiderComponent::RequestDetach(bool bJump)
{
	Server_Detach(bJump);
}
void UZiplineRiderComponent::Server_Detach_Implementation(bool bJump)
{
	if (!bIsRiding || !CurrentZipline) return;

	FVector LaunchVelocity = FVector::ZeroVector;
	if (bJump)
	{
		const FVector Tangent = CurrentZipline->GetSplineComp()->GetTangentAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World).GetSafeNormal();

		LaunchVelocity = Tangent * (MoveDirection * CurrentSpeed * JumpExitSpeedMultiplier) + FVector(0.f, 0.f, JumpExitUpForce);
	}

	bIsRiding = false;
	CurrentZipline = nullptr;
	CurrentSpeed = 0.f;

	RestorePhysics();
	Multicast_OnDetach(LaunchVelocity);
}

void UZiplineRiderComponent::OnRep_IsRiding()
{

	if (bIsRiding)
	{
		ApplyZiplinePhysics();
		if (MoveComp) MoveComp->bUseControllerDesiredRotation = false;
	}
	else
	{
		RestorePhysics();
		if (MoveComp) MoveComp->bUseControllerDesiredRotation = true;
	}
}

void UZiplineRiderComponent::TickZiplineMovement(float DeltaTime)
{
	if (!CurrentZipline || !OwnerChar) return;

	USplineComponent* Spline = CurrentZipline->GetSplineComp();
	const float SplineLen = Spline->GetSplineLength();

	CurrentSpeed = FMath::Min(CurrentSpeed + CurrentZipline->AccelerationRate * DeltaTime, CurrentZipline->MaxRideSpeed);

	CurrentSplineDistance += MoveDirection * CurrentSpeed * DeltaTime;

	if (CurrentSplineDistance >= SplineLen || CurrentSplineDistance <= 0.f)
	{
		CurrentSplineDistance = FMath::Clamp(CurrentSplineDistance, 0.f, SplineLen);
		Server_Detach_Implementation(false);
		return;
	}

	const FVector SplinePos = Spline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance, ESplineCoordinateSpace::World);
	const FVector TargetPos = SplinePos + FVector(0.f, 0.f, CurrentZipline->RiderHeightOffset);

	OwnerChar->SetActorLocation(TargetPos, false, nullptr, ETeleportType::TeleportPhysics);
}

void UZiplineRiderComponent::ApplyZiplinePhysics()
{
	if (!MoveComp) return;
	SavedGravityScale = MoveComp->GravityScale;
	MoveComp->GravityScale = 0.f;
	MoveComp->SetMovementMode(MOVE_Flying);
}

void UZiplineRiderComponent::RestorePhysics()
{
	if (!MoveComp) return;
	MoveComp->GravityScale = SavedGravityScale;
	MoveComp->SetMovementMode(MOVE_Walking);
}

bool UZiplineRiderComponent::FindNearestZipline(AZiplineActor*& OutZipline, float& OutDist, int8& OutDir) const
{
	if (!OwnerChar) return false;

	TArray<AActor*> Allziplines;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AZiplineActor::StaticClass(), Allziplines);

	const FVector PlayerPos = OwnerChar->GetActorLocation();
	AZiplineActor* Best = nullptr;
	float BestDist = MAX_FLT;
	float BestSplineDist = 0.f;

	for (AActor* Actor : Allziplines)
	{
		AZiplineActor* zipline = Cast<AZiplineActor>(Actor);
		if (!zipline) continue;

		USplineComponent* spline = zipline->GetSplineComp();
		const float len = spline->GetSplineLength();

		const FVector S = spline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
		const FVector E = spline->GetLocationAtDistanceAlongSpline(len, ESplineCoordinateSpace::World);
		const float D = FMath::Min(FVector::Dist(PlayerPos, S), FVector::Dist(PlayerPos, E));

		if (D < ScanRadius && D < BestDist)
		{
			BestDist = D;
			Best = zipline;

			const float Key = spline->FindInputKeyClosestToWorldLocation(PlayerPos);
			BestSplineDist = spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		}
	}
	if (!Best) return false;

	OutZipline = Best;
	OutDist = BestSplineDist;

	const FVector Tangent = Best->GetSplineComp()->GetTangentAtDistanceAlongSpline(BestSplineDist, ESplineCoordinateSpace::World).GetSafeNormal();
	OutDir = (FVector::DotProduct(OwnerChar->GetActorForwardVector(), Tangent) >= 0.f) ? 1 : -1;

	return true;

}

void UZiplineRiderComponent::Multicast_OnDetach_Implementation(FVector LaunchVelocity)
{
	RestorePhysics();
	if (!LaunchVelocity.IsNearlyZero())
		OwnerChar->LaunchCharacter(LaunchVelocity, true, true);

}

void UZiplineRiderComponent::Server_Attach_Implementation(AZiplineActor* Zipline, float StartDist, int8 Dir)
{
	if (!Zipline || bIsRiding || !OwnerChar) return;

	CurrentZipline = Zipline;
	CurrentSplineDistance = StartDist;
	MoveDirection = Dir;
	CurrentSpeed = 0.f;
	bIsRiding = true;

	ApplyZiplinePhysics();
}

// Called every frame
void UZiplineRiderComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                           FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsRiding && GetOwner()->HasAuthority())
		TickZiplineMovement(DeltaTime);
	// ...
}


