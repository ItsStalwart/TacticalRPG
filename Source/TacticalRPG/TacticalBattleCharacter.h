// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TacticalBattleCharacter.generated.h"

UCLASS()
class TACTICALRPG_API ATacticalBattleCharacter : public ACharacter
{
	GENERATED_BODY()

	friend class AGridActor;

public:
	// Sets default values for this character's properties
	ATacticalBattleCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FIntVector2 CurrentPosition {-1,-1};

public:
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
