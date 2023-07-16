// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridData.generated.h"

/**
 * 
 */
UCLASS()
class TACTICALRPG_API UGridData : public UDataAsset
{
public:
	[[nodiscard]] const FVector& GetGridSize() const
	{
		return GridSize;
	}

	[[nodiscard]] const TSoftObjectPtr<UStaticMesh>& GetGridMesh() const
	{
		return GridMesh;
	}

	[[nodiscard]] const TSoftObjectPtr<UMaterialInstance>& GetMeshMaterial() const
	{
		return MeshMaterial;
	}

	[[nodiscard]] const TSoftObjectPtr<UStaticMesh>& GetTileMesh() const
	{
		return TileMesh;
	}

	[[nodiscard]] const TSoftObjectPtr<UMaterialInstance>& GetTileBorderMaterial() const
	{
		return TileBorderMaterial;
	}

	[[nodiscard]] const TSoftObjectPtr<UMaterialInstance>& GetTileFilledMaterial() const
	{
		return TileFilledMaterial;
	}

private:
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FVector GridSize{0,0,0};

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
