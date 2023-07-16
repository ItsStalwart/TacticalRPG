// Fill out your copyright notice in the Description page of Project Settings.


#include "TacticalBattleCharacter.h"

#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ATacticalBattleCharacter::ATacticalBattleCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATacticalBattleCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATacticalBattleCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATacticalBattleCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

