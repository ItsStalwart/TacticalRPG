// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridUtilities.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

USTRUCT()
struct FTileData
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
	int InstanceIndex;

	UPROPERTY(VisibleAnywhere)
	TArray<FIntVector2> NeighboringIndexes{};
	
	UPROPERTY(VisibleAnywhere,meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState"))
	uint8 TileState;

	UPROPERTY(VisibleAnywhere,meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 AllowedMovementTypes;
	FTileData(): InstanceIndex(0), TileState(0), AllowedMovementTypes(0)
	{
	}
	;

	explicit FTileData(const int InIndex)
	{
		InstanceIndex = InIndex;
		TileState = 0;
		AllowedMovementTypes = static_cast<uint8>(EGridMovementType::Any); //Allow any movement by default
	}
	
};

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ETileState
{
	None=0 UMETA(Hidden),
	Hovered = 1 << 0 UMETA(DisplayName="Hovered"),
	Selected = 1 << 1 UMETA(DisplayName="Selected"),
};
ENUM_CLASS_FLAGS(ETileState);

UCLASS()
class TACTICALRPG_API AGridActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridActor();

	FIntVector2& GetHoveredTileIndex() {return HoveredTileIndex;};
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<class UGridData> GridData{nullptr};

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	TObjectPtr<class UInstancedStaticMeshComponent> InstancedStaticMeshComponent{nullptr};

	UFUNCTION(CallInEditor, Category = "Debug Utilities")
	void RegenerateEnvironmentGrid();

	UFUNCTION(CallInEditor, Category = "Debug Utilities")
	void RegenerateDefaultGrid();

	UFUNCTION(BlueprintCallable)
	void SpawnGridAt(FVector SpawnLocation, bool bUseEnvironment = false, bool bUnspawnIfExists = true);

	UFUNCTION(BlueprintCallable,CallInEditor, Category = "Debug Utilities")
	void DestroyGrid();

	UFUNCTION(BlueprintCallable)
	bool TraceForGround(FVector TraceStartLocation, FVector& TraceHitLocation) const;

	

private:
	TMap<FIntVector2, int> GridIndexToInstanceIndex{};
	UPROPERTY(VisibleInstanceOnly)
	TMap<FIntVector2, FTileData> TileDataMap{};
	void AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex);
	bool RemoveTileAt(const FIntVector2& GridIndexToRemove);
	
	void HighlightTile(const FIntVector2& GridIndex);
	void UnlightTile(const FIntVector2& GridIndex);

	UFUNCTION()
	void ApplyStateToTile(const FIntVector2& TileIndex, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState")) const uint8 StateToAdd);
	UFUNCTION()
	void RemoveStateFromTile(const FIntVector2& TileIndex,UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState")) const uint8 StateToRemove);
	
	FIntVector2 GetTileIndexByCursorPosition(int PlayerControllerIndex) const;

	FIntVector2 HoveredTileIndex{-1,-1};

	UFUNCTION()
	void SelectHoveredTile();

	bool IsTileSelected(const FIntVector2& TileIndex);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool ContainsTileWithIndex(const FIntVector2& TileIndex) const;
};



