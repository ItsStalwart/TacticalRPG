// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridUtilities.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

struct FGridModifierVolumeData;

USTRUCT()
struct FPathData
{
	GENERATED_BODY()
private:
	
	TOptional<FIntVector2> Connection {};

	UPROPERTY(VisibleAnywhere)
	int G{INT_MAX};
	
	UPROPERTY(VisibleAnywhere)
	int H{0};

	friend class UTileData;
};
UCLASS()
class UTileData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
	int InstanceIndex;

	UPROPERTY(VisibleAnywhere)
	int MovementCost{1};

	UPROPERTY(VisibleAnywhere)
	int Height{1};
	
	UPROPERTY(VisibleAnywhere,meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState"))
	uint8 TileState;

	UPROPERTY(VisibleAnywhere,meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType"))
	uint8 AllowedMovementTypes{ static_cast<uint8>(EGridMovementType::Any)};
	
	UPROPERTY(VisibleAnywhere)
	FPathData TilePathData;

	TOptional<FIntVector2>& GetConnectedTile() {return TilePathData.Connection;}
	void SetConnectedTile(const FIntVector2& InConnection) {TilePathData.Connection = InConnection;}
	void ResetConnectedTile() {TilePathData.Connection.Reset();}
	void SetGValue(const int32 InValue) {TilePathData.G = InValue;}
	int& GetGValue() {return TilePathData.G;}
	void SetHValue(const int32 InValue) {TilePathData.H = InValue;}
	int& GetHValue() {return TilePathData.H;}
	int GetFValue() const {return TilePathData.G + TilePathData.H;}
	int& GetMovementCost() {return MovementCost;}
	void SetMovementCost(const int32 InValue) {MovementCost = InValue;}

	UFUNCTION()
	bool IsTileWalkable(UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const uint8 MoveTypeToCheck) const { return AllowedMovementTypes & MoveTypeToCheck;};


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
	bool TraceForGround(FVector TraceStartLocation, FVector& TraceHitLocation, FGridModifierVolumeData& HitVolumeData) const;

	void RetracePathFromIndex(const FIntVector2& IntVector2, TArray<FIntVector2>& Array) const;
	UFUNCTION()
	bool FindPath(const FIntVector2& StartIndex, const FIntVector2& TargetIndex, TArray<FIntVector2>& OutPath, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const uint8 UnitMovementType = static_cast<uint8>(EGridMovementType::Any), const int UnitJumpPower = INT_MAX, TMap<FIntVector2, UTileData*> TileSetData = {}) const;
	UFUNCTION()
	void GetWalkableTilesInRange(const FIntVector2& StartIndex, const int MovementRange, TArray<FIntVector2>& OutRange, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const uint8 UnitMovementType = static_cast<uint8>(EGridMovementType::Any), const int UnitJumpPower = INT_MAX );
	UFUNCTION()
	int CalculatePathingCost(TArray<FIntVector2>& Path, bool HinderedByTerrain) const;

private:
	TMap<FIntVector2, int> GridIndexToInstanceIndex{};

	UPROPERTY(VisibleInstanceOnly)
	TMap<FIntVector2, UTileData*> TileDataMap{};
	int GetTileGValueByIndex(const FIntVector2& TileIndex) const;
	int GetTileMovementCost(const FIntVector2& TileIndex, bool bHinderedByTerrain) const;
	FIntVector2 GetLowestFValueTileIndex(const TArray<FIntVector2>& GroupToSearch) const;
	

	void AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex, const FGridModifierVolumeData InTileSettings);
	bool RemoveTileAt(const FIntVector2& GridIndexToRemove);
	
	void HighlightTile(const FIntVector2& GridIndex);
	void UnlightTile(const FIntVector2& GridIndex);
	void UnlightAllTiles();

	UFUNCTION()
	void ApplyStateToTile(const FIntVector2& TileIndex, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState")) const uint8 StateToAdd);
	UFUNCTION()
	void RemoveStateFromTile(const FIntVector2& TileIndex,UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.ETileState")) const uint8 StateToRemove);
	
	FIntVector2 GetTileIndexByCursorPosition(int PlayerControllerIndex) const;

	FIntVector2 HoveredTileIndex{-1,-1};

	static int GetDistanceBetweenTiles(const FIntVector2& TileAIndex,const FIntVector2& TileBIndex);
	void GetAllTilesInRange(const FIntVector2& StartIndex, const int MovementRange, TArray<FIntVector2>& OutRange,TMap<FIntVector2, UTileData*> TileSetData = {}) const;
	UFUNCTION()
	void SelectHoveredTile();

	void GetTileNeighborhood(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood, TMap<FIntVector2, UTileData*> TileSetData = {}) const;
	UFUNCTION()
	void GetWalkableNeighbors(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood, UPARAM(meta=(BitMask, BitMaskEnum = "/Script/TacticalRPG.EGridMovementType")) const uint8 MoveTypeToCheck = static_cast<uint8>(EGridMovementType::Any), int JumpPower = INT_MAX, TMap<FIntVector2, UTileData*> TileSetData = {}) const;

	bool IsTileSelected(const FIntVector2& TileIndex);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	bool ContainsTileWithIndex(const FIntVector2& TileIndex) const;
};



