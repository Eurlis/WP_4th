// Fill out your copyright notice in the Description page of Project Settings.


#include "Wraith.h"


// Sets default values
AWraith::AWraith()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	ConstructorHelpers::FObjectFinder<USkeletalMesh>ChaMesh(TEXT("/Game/Models/Wraith/Wraith.Wraith"));
	if (ChaMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(ChaMesh.Object);
	}
}

// Called when the game starts or when spawned
void AWraith::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWraith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AWraith::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

