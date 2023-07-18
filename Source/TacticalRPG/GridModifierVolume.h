// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridUtilities.h"
#include "Engine/StaticMeshActor.h"
#include "GridModifierVolume.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TACTICALRPG_API AGridModifierVolume : public AStaticMeshActor
{
	GENERATED_BODY()

	AGridModifierVolume();

	UPROPERTY(EditInstanceOnly, meta=(Bitmask, BitmaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 VolumeAllowedMovement {0}; 

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UFUNCTION(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 GetVolumeAllowedMovement() const { return VolumeAllowedMovement;};

	UFUNCTION()
	bool DoesBlockAllMovement() const {return VolumeAllowedMovement == 0;};
};
