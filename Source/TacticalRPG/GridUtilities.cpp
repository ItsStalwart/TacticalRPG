// Fill out your copyright notice in the Description page of Project Settings.


#include "GridUtilities.h"

#include "GridActor.h"

FLinearColor UGridUtilitiesFunctionLibrary::GenerateVolumeColor(const int MovementCombination)
{
	FLinearColor ReturnColor{FLinearColor::Black};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Ground)) {ReturnColor+= FLinearColor::Red;};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Aquatic)) {ReturnColor+= FLinearColor::Blue;};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Aerial)) {ReturnColor+= FLinearColor::Green;};
	return ReturnColor;
}

void UGridUtilitiesFunctionLibrary::GenerateTileNeighborhood(const FIntVector2& TileIndex,
	const AGridActor* ContainingGrid, TArray<FIntVector2>& OutNeighborhood)
{
	if(TileIndex.X <0 || TileIndex.Y <0 || !ContainingGrid->ContainsTileWithIndex(TileIndex))
	{
		UE_LOG(LogTemp,Warning, TEXT("Referenced grid does not contain a tile with the provided index."))
		return;
	}
	OutNeighborhood.Empty();
	for(int i = TileIndex.X-1;i<=TileIndex.X + 1; i++)
	{
		for(int j = TileIndex.Y-1;j<=TileIndex.Y + 1; j++)
		{
			FIntVector2 NeighborIndex {i,j};
			if(ContainingGrid->ContainsTileWithIndex(NeighborIndex) && NeighborIndex != TileIndex)
			{
				OutNeighborhood.AddUnique(NeighborIndex);
			}
		}
	}
}
