// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridActor.generated.h"

USTRUCT()
struct FTileData
{
	GENERATED_BODY()
		
};

UENUM(BlueprintType, meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ETileState
{
	None=0 UMETA(Hidden),
	Hovered = 0 << 1,
	Selected = 1 << 1,
};

UCLASS()
class TACTICALRPG_API AGridActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGridActor();

	FIntVector2& GetSelectedTileIndex() {return SelectedTileIndex;};
	

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
	void AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex);
	bool RemoveTileAt(const FIntVector2& GridIndexToRemove);
	
	void HighlightTile(int TargetTile);
	void UnlightTile(int TargetTile);
	FIntVector2 GetTileIndexByCursorPosition(int PlayerControllerIndex) const;

	FIntVector2 SelectedTileIndex{0,0};

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};



