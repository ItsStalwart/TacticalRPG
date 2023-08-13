// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridUtilities.generated.h"

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class EGridMovementType : uint8
{
	None=0 UMETA(Hidden, DisplayName="No Movement", DisplayTooltip="Unit may not move. If set to a GridModifierVolume, no tiles will be generated in that area."),
	Ground = 1 << 0  UMETA(DisplayName="Ground Movement", DisplayTooltip="Unit may only move on ground terrain. If set to a GridModifierVolume, tiles generated in that area will block movement of Aquatic, Aerial and AerialAquatic units."),
	Aquatic = 1 << 1 UMETA(DisplayName="Aquatic Movement", DisplayTooltip="Unit may only move on water terrain. If set to a GridModifierVolume, tiles generated in that area will block movement of Ground, Aerial and Hydrophobic units."),
	Aerial = 1 << 2 UMETA(DisplayName="Flying Movement", DisplayTooltip="Unit may only move on air terrain. If set to a GridModifierVolume, tiles generated in that area will block movement of Ground, Aquatic and Amphibious units."),

	Amphibious = Ground + Aquatic UMETA(Hidden, DisplayName="Amphibious Movement", DisplayTooltip="Unit may only move on air terrain. If set to a GridModifierVolume, tiles generated in that area will only block the movement of Aerial units."),
	AerialAquatic = Aquatic + Aerial UMETA(Hidden, DisplayName="Non-Ground Movement", DisplayTooltip="Unit may only move on air terrain. If set to a GridModifierVolume, tiles generated in that area will only block the movement of Ground units."),
	Hydrophobic = Ground + Aerial UMETA(Hidden, DisplayName="Non-Water Movement", DisplayTooltip="Unit may only move on air terrain. If set to a GridModifierVolume, tiles generated in that area will only block the movement of Aquatic units"),
	
	Any = Ground + Aquatic + Aerial UMETA(Hidden, DisplayName="Free Movement", DisplayTooltip="Unit may only move on air terrain. If set to a GridModifierVolume, tiles generated in that area will not block the movement of any unit.")
};
ENUM_CLASS_FLAGS(EGridMovementType);
/**
 * 
 */
UCLASS(Abstract)
class TACTICALRPG_API UGridUtilitiesFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	//TODO:Move this to Unit Actor?
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool CanUnitMoveInTile(UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const int AllowedTileMovement, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const int UnitMovement) {return AllowedTileMovement & UnitMovement;}

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FLinearColor GenerateVolumeColor(UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const int MovementCombination);

};
