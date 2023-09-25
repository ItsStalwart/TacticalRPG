// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridUtilities.h"
#include "Engine/StaticMeshActor.h"
#include "GridModifierVolume.generated.h"

USTRUCT(BlueprintType)
struct FGridModifierVolumeData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	int ModifiedMovementCost{1};

	UPROPERTY(EditAnywhere, meta=(Bitmask, BitmaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 VolumeAllowedMovement {7};
};
/**
 * 
 */
UCLASS(Blueprintable)
class TACTICALRPG_API AGridModifierVolume : public AStaticMeshActor
{
	GENERATED_BODY()

	AGridModifierVolume();

	UPROPERTY(EditInstanceOnly)
	FGridModifierVolumeData VolumeSettings;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UFUNCTION(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 GetVolumeAllowedMovement() const { return VolumeSettings.VolumeAllowedMovement;};

	UFUNCTION()
	bool DoesBlockAllMovement() const {return VolumeSettings.VolumeAllowedMovement == 0;};

	UFUNCTION()
	int GetModifiedMovementCost() const {return VolumeSettings.ModifiedMovementCost;}

	UFUNCTION()
	FGridModifierVolumeData GetVolumeSettings() const {return VolumeSettings;}
};
