// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridData.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class TACTICALRPG_API UGridData : public UDataAsset
{
	GENERATED_BODY()
public:

	const FVector& GetGridSize() const
	{
		return GridSize;
	}

	const FIntVector2& GetGridDimension() const
	{
		return GridDimension;
	}
	
	const TSoftObjectPtr<UStaticMesh>& GetGridMesh() const
	{
		return GridMesh;
	}
	
	const TSoftObjectPtr<UMaterialInstance>& GetMeshMaterial() const
	{
		return MeshMaterial;
	}
	
	const TSoftObjectPtr<UStaticMesh>& GetTileMesh() const
	{
		return TileMesh;
	}
	
	const TSoftObjectPtr<UMaterialInstance>& GetTileBorderMaterial() const
	{
		return TileBorderMaterial;
	}
	
	const TSoftObjectPtr<UMaterialInstance>& GetTileFilledMaterial() const
	{
		return TileFilledMaterial;
	}

private:
	

	UPROPERTY(EditDefaultsOnly)
	FVector GridSize{0,0,0};

	UPROPERTY(EditDefaultsOnly)
	FIntVector2 GridDimension{1,1};

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UStaticMesh> GridMesh{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UMaterialInstance> MeshMaterial{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UStaticMesh> TileMesh{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UMaterialInstance> TileBorderMaterial{nullptr};

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UMaterialInstance> TileFilledMaterial{nullptr};
};
