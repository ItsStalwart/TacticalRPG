// Fill out your copyright notice in the Description page of Project Settings.


#include "GridActor.h"

#include "Editor.h"
#include "GridData.h"
#include "GridModifierVolume.h"
#include "MathUtil.h"
#include "TacticalBattleCameraPawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Editor/EditorEngine.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
AGridActor::AGridActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	InstancedStaticMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstanceStaticMeshComponent"));
	const UGridData* SetGridData = GridData.LoadSynchronous();
	if(SetGridData != nullptr)
	{
		InstancedStaticMeshComponent->SetStaticMesh(SetGridData->GetTileMesh().LoadSynchronous());
	}
}

// Called when the game starts or when spawned
void AGridActor::BeginPlay()
{
	Super::BeginPlay();

	RegenerateEnvironmentGrid();

	auto* CameraControl = Cast<ATacticalBattleCameraPawn>(UGameplayStatics::GetPlayerController(GetWorld(), 0)->GetPawn());
	if(CameraControl!= nullptr)
	{
		CameraControl->GetSelectionEvent().AddUniqueDynamic(this, &AGridActor::SelectHoveredTile);
	}
	
}

void AGridActor::RegenerateEnvironmentGrid()
{
	SpawnGridAt(GetActorLocation(), true, true);
}

void AGridActor::RegenerateDefaultGrid()
{
	SpawnGridAt(GetActorLocation(), false, true);
}

void AGridActor::SpawnGridAt(FVector SpawnLocation, bool bUseEnvironment, bool bDestroyIfExists)
{
	if(GridData.IsNull())
	{
		UE_LOG(LogTemp,Warning, TEXT("No GridData set before spawning!"));
		return;
	}
	const auto* SetGridData = GridData.LoadSynchronous();
	SetActorLocation(SpawnLocation);
	if(bDestroyIfExists)
	{
		DestroyGrid();
	}
	InstancedStaticMeshComponent->SetStaticMesh(SetGridData->GetTileMesh().LoadSynchronous());
	
	const FIntVector2 GridDimension = SetGridData->GetGridDimension();
	const float GridStep = InstancedStaticMeshComponent->GetStaticMesh()->GetBoundingBox().GetSize().X;
	for(int i = 0; i<GridDimension.X; i++)
	{
		for(int j = 0; j<GridDimension.Y; j++)
		{
			FVector TileLocation {GridStep * i, GridStep*j, 0};
			FTransform TileTransform;
			FGridModifierVolumeData VolumeData;
			if(bUseEnvironment)
			{
				FVector NewLocation{};
				const bool bSpawnTile = TraceForGround(GetActorLocation() + TileLocation,NewLocation, VolumeData);
				if(bSpawnTile)
				{
					TileTransform = FTransform{NewLocation-GetActorLocation()+FVector{0,0,1}};
					AddTileAt(TileTransform, {i,j}, VolumeData);
				}
				
				continue;
			}
			TileTransform = FTransform{TileLocation};
			AddTileAt(TileTransform, {i,j}, VolumeData);
		}
	}
}

void AGridActor::DestroyGrid()
{
	InstancedStaticMeshComponent->ClearInstances();
	GridIndexToInstanceIndex.Empty();
	TileDataMap.Empty();
	const auto* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if(Controller != nullptr)
	{
		auto* CameraControl = Cast<ATacticalBattleCameraPawn>(Controller->GetPawn());
		if(CameraControl != nullptr)
		{
			if(CameraControl->GetSelectionEvent().IsBound())
			{
				CameraControl->GetSelectionEvent().RemoveAll(this);
			}
		}
	}
	
}

bool AGridActor::TraceForGround(FVector TraceStartLocation, FVector& TraceHitLocation, FGridModifierVolumeData& HitVolumeData) const
{
	TArray<FHitResult> TraceHits{};
	constexpr float TraceRadius = 50.f;
	const bool bHit = UKismetSystemLibrary::SphereTraceMulti(GetWorld(), TraceStartLocation, TraceStartLocation-FVector{0,0,1000}, TraceRadius, UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), false, TArray<AActor*>{}, EDrawDebugTrace::None, TraceHits, true );
	if(bHit)
	{
		const auto* ModifierVolume = Cast<AGridModifierVolume>(TraceHits[0].GetActor());
		if(ModifierVolume != nullptr)
		{
			if(ModifierVolume->DoesBlockAllMovement())
			{
				return false;
			}
			HitVolumeData = ModifierVolume->GetVolumeSettings();
		}
		TraceHitLocation = TraceHits[0].Location - FVector(0,0, TraceRadius);
	}
	return bHit;
}

void AGridActor::RetracePathFromIndex(const FIntVector2& IntVector2, TArray<FIntVector2>& Array) const
{
	Array.Empty();
	Array.Emplace(IntVector2);
	UTileData* CurrentTile = TileDataMap.FindChecked(IntVector2);
	while(CurrentTile->GetConnectedTile().IsSet())
	{
		auto Index = CurrentTile->GetConnectedTile().GetValue();
		Array.Emplace(Index);
		CurrentTile = TileDataMap.FindChecked(Index);
	}
	Algo::Reverse(Array);
}

bool AGridActor::FindPath(const FIntVector2& StartIndex, const FIntVector2& TargetIndex, TArray<FIntVector2>& OutPath,
                          const uint8 UnitMovementType, const int UnitJumpPower, TMap<FIntVector2, UTileData*> TileSetData) const
{
	if(TileSetData.IsEmpty())
	{
		TileSetData = TileDataMap;
	}
	OutPath.Empty();
	TArray<FIntVector2> OpenList{};
	OpenList.AddUnique(StartIndex);
	TArray<FIntVector2> ClosedList{};
	for (auto& [Index, Tile] : TileSetData)
	{
		Tile->ResetConnectedTile();
		Tile->SetGValue(MAX_int32);
	}
	auto* StartNode = TileSetData.FindChecked(StartIndex);
	StartNode->SetGValue(0);
	StartNode->SetHValue(GetDistanceBetweenTiles(StartIndex,TargetIndex));

	while(!OpenList.IsEmpty())
	{
		FIntVector2 CurrentIndex = GetLowestFValueTileIndex(OpenList);
		if(CurrentIndex == TargetIndex)
		{
			OpenList.Empty();
			RetracePathFromIndex(TargetIndex, OutPath);
			return true;
		}
		OpenList.Remove(CurrentIndex);
		ClosedList.AddUnique(CurrentIndex);
		TArray<FIntVector2> Neighbors;
		GetWalkableNeighbors(CurrentIndex,Neighbors, UnitMovementType, UnitJumpPower, TileSetData);
		for (auto& Index : Neighbors)
		{
			if(ClosedList.Contains(Index))
			{
				continue;
			}
			const int TentativeGValue = GetTileGValueByIndex(CurrentIndex) + GetTileMovementCost(Index, UnitMovementType & static_cast<uint8>(EGridMovementType::Aerial));
			if(TentativeGValue < GetTileGValueByIndex(Index))
			{
				auto* TargetTile = TileSetData.FindChecked(Index);
				TargetTile->SetConnectedTile(CurrentIndex);
				TargetTile->SetGValue(TentativeGValue);
				TargetTile->SetHValue(GetDistanceBetweenTiles(Index,TargetIndex));
				OpenList.AddUnique(Index);
			}
		}
	}
	return false;
}

void AGridActor::GetWalkableTilesInRange(const FIntVector2& StartIndex, const int MovementRange,TArray<FIntVector2>& OutRange,
	const uint8 UnitMovementType, const int UnitJumpPower)
{
	OutRange.Empty();
	if (MovementRange<=0)
	{
		return;
	}
	GetAllTilesInRange(StartIndex,MovementRange,OutRange);
	if(OutRange.IsEmpty())
	{
		return;
	}
	TMap<FIntVector2, UTileData*> RangeDataSet {};
	RangeDataSet.Reserve(OutRange.Num());
	TArray<FIntVector2> AuxRange{};
	for(auto Index : OutRange)
	{
		if(this->ContainsTileWithIndex(Index))
		{
			RangeDataSet.Emplace(Index, TileDataMap.FindChecked(Index));
		}
	}
	TArray<FIntVector2> AuxPath{};
	for(auto Index : OutRange)
	{
		if(FindPath(StartIndex,Index, AuxPath, UnitMovementType, UnitJumpPower, RangeDataSet ))
		{
			if( CalculatePathingCost(AuxPath, UnitMovementType & static_cast<uint8>(EGridMovementType::Aerial)) <= MovementRange)
			{
				AuxRange.Emplace(Index);
			}
		}
	}
	OutRange = AuxRange;
}

int AGridActor::CalculatePathingCost(TArray<FIntVector2>& Path, bool HinderedByTerrain) const
{
	int Result = 0;
	for (int i = 1; i < Path.Num(); i++)
	{
		Result += GetTileMovementCost(Path[i], HinderedByTerrain);
	}
	return Result;
}

int AGridActor::GetTileGValueByIndex(const FIntVector2& TileIndex) const
{
	if(!ContainsTileWithIndex(TileIndex))
	{
		return INT_MAX;
	}
	return TileDataMap.FindChecked(TileIndex)->GetGValue();
}

int AGridActor::GetTileMovementCost(const FIntVector2& TileIndex, bool bHinderedByTerrain) const
{
	if(!ContainsTileWithIndex(TileIndex))
	{
		return INT_MAX;
	}
	if(!bHinderedByTerrain)
	{
		return 1;
	}
	return TileDataMap.FindChecked(TileIndex)->GetMovementCost();
}

FIntVector2 AGridActor::GetLowestFValueTileIndex(const TArray<FIntVector2>& GroupToSearch) const
{
	FIntVector2 LowestFValueIndex = GroupToSearch[0];
	int LowestFValue = TileDataMap.FindChecked(LowestFValueIndex)->GetFValue();
	for(auto& Index : GroupToSearch)
	{
		const auto* Tile = TileDataMap.FindChecked(Index);
		if(Tile->GetFValue() < LowestFValue)
		{
			LowestFValue = Tile->GetFValue();
			LowestFValueIndex = Index;
		}
	}
	return  LowestFValueIndex;
}

int AGridActor::GetDistanceBetweenTiles(const FIntVector2& TileAIndex, const FIntVector2& TileBIndex)
{
	const int DistanceY = FMath::Abs(TileAIndex.Y - TileBIndex.Y);
	const int DistanceX = FMath::Abs(TileAIndex.X - TileBIndex.X);
	return DistanceX + DistanceY;
}

void AGridActor::GetAllTilesInRange(const FIntVector2& StartIndex, const int MovementRange,
	TArray<FIntVector2>& OutRange, TMap<FIntVector2, UTileData*> TileSetData) const
{
	if(TileSetData.IsEmpty())
	{
		TileSetData = TileDataMap;
	}
	OutRange.Empty();
	for(int i = -MovementRange; i <= MovementRange;i++)
	{
		for(int j = -MovementRange;j <= MovementRange;j++)
		{
			const FIntVector2 TentativeIndex {StartIndex.X+i, StartIndex.Y+j};
			if(TileSetData.Contains(TentativeIndex) && GetDistanceBetweenTiles(StartIndex,TentativeIndex) <= MovementRange)
			{
				OutRange.AddUnique(TentativeIndex);
			}
		}
	}
}

void AGridActor::AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex, const FGridModifierVolumeData InTileSettings)
{
	const int InstanceIndex = InstancedStaticMeshComponent->GetInstanceCount();
	UTileData* TileData = NewObject<UTileData>();
	TileData->SetMovementCost(InTileSettings.ModifiedMovementCost);
	TileData->AllowedMovementTypes = InTileSettings.VolumeAllowedMovement;
	TileData->InstanceIndex = InstanceIndex;
	GridIndexToInstanceIndex.Add(GridIndex, InstanceIndex);
	InstancedStaticMeshComponent->AddInstance(TileTransform);
	TileDataMap.Add(GridIndex,TileData);
}

bool AGridActor::RemoveTileAt(const FIntVector2& GridIndexToRemove)
{
	if(!GridIndexToInstanceIndex.Contains(GridIndexToRemove))
	{
		return false;
	}
	const int TargetIndex = GridIndexToInstanceIndex.FindAndRemoveChecked(GridIndexToRemove);
	TileDataMap.FindAndRemoveChecked(GridIndexToRemove);
	InstancedStaticMeshComponent->RemoveInstance(TargetIndex);
	return true;
}

void AGridActor::HighlightTile(const FIntVector2& GridIndex)
{
	const int TargetTile = *GridIndexToInstanceIndex.Find(GridIndex);
	InstancedStaticMeshComponent->SetCustomDataValue(TargetTile, 0, 1,true);
	ApplyStateToTile(GridIndex,static_cast<int>(ETileState::Hovered) );
}

void AGridActor::UnlightTile(const FIntVector2& GridIndex)
{
	const int TargetTile = *GridIndexToInstanceIndex.Find(GridIndex);
	InstancedStaticMeshComponent->SetCustomDataValue(TargetTile, 0, 0, true);
	RemoveStateFromTile(GridIndex, static_cast<int>(ETileState::Hovered));
}

void AGridActor::UnlightAllTiles()
{
	for(auto& [Index, Tile] : TileDataMap)
	{
		UnlightTile(Index);
	}
}

void AGridActor::ApplyStateToTile(const FIntVector2& TileIndex, const uint8 StateToAdd)
{
	UTileData* TileData = TileDataMap.FindChecked(TileIndex);
	TileData->TileState |= StateToAdd;
}

void AGridActor::RemoveStateFromTile(const FIntVector2& TileIndex, const uint8 StateToRemove)
{
	UTileData* TileData = TileDataMap.FindChecked(TileIndex);
	TileData->TileState &= ~StateToRemove;
}

FIntVector2 AGridActor::GetTileIndexByCursorPosition(int PlayerControllerIndex) const
{
	FVector ControllerCursorLocation;
	FVector WorldDirection;
	FHitResult TraceHit;
	FVector CursorProjectionInGrid;
	UGameplayStatics::GetPlayerController(GetWorld(), PlayerControllerIndex)->DeprojectMousePositionToWorld(ControllerCursorLocation, WorldDirection);
	const bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), ControllerCursorLocation, ControllerCursorLocation+2000*WorldDirection, 20.f, UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel2), false, TArray<AActor*>{}, EDrawDebugTrace::ForOneFrame, TraceHit, true );
	if(bHit)
	{
		CursorProjectionInGrid = TraceHit.Location;
	}
	TArray<int32> InstancesNearCursorByIndex = InstancedStaticMeshComponent->GetInstancesOverlappingSphere(CursorProjectionInGrid, 20.f); 
	if(InstancesNearCursorByIndex.IsEmpty())
	{
#if WITH_EDITOR
		GEngine->AddOnScreenDebugMessage(0, 5, FColor::Yellow, FString::Format(TEXT("Player cursor at World Location: {0},{1},{2}. No tile at location"), {CursorProjectionInGrid.X, CursorProjectionInGrid.Y, CursorProjectionInGrid.Z}));
		GEngine->RemoveOnScreenDebugMessage(1);
		GEngine->RemoveOnScreenDebugMessage(2);
#endif
		return {-1,-1}; //return negative index to represent no tile overlapped
	}
	
	InstancesNearCursorByIndex.Sort([this, ControllerCursorLocation](const int32 IndexA, const int32 IndexB)
	{
		FTransform InstanceATransform;
		FTransform InstanceBTransform;
		InstancedStaticMeshComponent->GetInstanceTransform(IndexA,InstanceATransform, true);
		InstancedStaticMeshComponent->GetInstanceTransform(IndexB,InstanceBTransform, true);
		return FVector::Distance(InstanceATransform.GetLocation(), ControllerCursorLocation) > FVector::Distance(InstanceBTransform.GetLocation(), ControllerCursorLocation);
	});
	
	auto& TileIndex = *GridIndexToInstanceIndex.FindKey(InstancesNearCursorByIndex[0]);
#if WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(0, 5, FColor::Yellow, FString::Format(TEXT("Player cursor at World Location: {0},{1},{2}. Closest Tile: {3}, {4}"), {CursorProjectionInGrid.X, CursorProjectionInGrid.Y, CursorProjectionInGrid.Z, TileIndex.X, TileIndex.Y}));
	auto* TargetTileData = *TileDataMap.Find(TileIndex);
	TArray<FIntVector2> TileNeighborhood{};
	GetTileNeighborhood(TileIndex, TileNeighborhood);
	GEngine->AddOnScreenDebugMessage(1, 5, FColor::Yellow, FString::Format(TEXT("Data for tile - InstanceIndex : {0}. CurrentState: {1}. AllowedMovement: {2}"), {TargetTileData->InstanceIndex, TargetTileData->TileState, TargetTileData->AllowedMovementTypes}));
	FString NeighborListString{TEXT("Tile neighbors: ")};
	for(auto& Index : TileNeighborhood)
	{
		NeighborListString.Append(FString::Format(TEXT("({0}, {1}); "), {Index.X, Index.Y}));
	}
	GEngine->AddOnScreenDebugMessage(2,5,FColor::Yellow, NeighborListString);
#endif
	return {TileIndex};
	
}

void AGridActor::SelectHoveredTile()
{
	if(HoveredTileIndex.X<0)
	{
		return;
	}
	
	if(!IsTileSelected(HoveredTileIndex))
	{
		UnlightAllTiles();
		// TArray<FIntVector2> Path;
		// if(FindPath({0,0},HoveredTileIndex,Path, static_cast<uint8>(EGridMovementType::Ground)))
		// {
		// 	for (auto& Index : Path)
		// 	{
		// 		HighlightTile(Index);
		// 	}
		// }

		TArray<FIntVector2> Range;
		GetWalkableTilesInRange(HoveredTileIndex,5,Range, static_cast<uint8>(EGridMovementType::Ground));
		for (auto& Index : Range)
		{
			HighlightTile(Index);
		}
	}
}

void AGridActor::GetTileNeighborhood(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood, TMap<FIntVector2, UTileData*> TileSetData) const
{
	if(TileSetData.IsEmpty())
	{
		TileSetData = TileDataMap;
	}
	if(TileIndex.X <0 || TileIndex.Y <0 || !this->ContainsTileWithIndex(TileIndex))
	{
		UE_LOG(LogTemp,Warning, TEXT("Referenced grid does not contain a tile with the provided index."))
		return;
	}
	OutNeighborhood.Empty();
	//(X-1,Y), (X+1,Y), (X,Y-1), (X,Y+1)
	for(int i = -1;i <= 1; i++)
	{
		for(int j = -1;j <= 1; j++)
		{
			if(i == j || i == -j)
			{
				continue;
			}
			FIntVector2 NeighborIndex {TileIndex.X+i,TileIndex.Y+j};
			if(TileSetData.Contains(NeighborIndex) && NeighborIndex != TileIndex)
			{
				OutNeighborhood.AddUnique(NeighborIndex);
			}
		}
	}
}

void AGridActor::GetWalkableNeighbors(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood,
	const uint8 MoveTypeToCheck, int JumpPower, TMap<FIntVector2, UTileData*> TileSetData) const
{
	if(TileSetData.IsEmpty())
	{
		TileSetData = TileDataMap;
	}
	if(TileIndex.X <0 || TileIndex.Y <0 || !this->ContainsTileWithIndex(TileIndex))
	{
		UE_LOG(LogTemp,Warning, TEXT("Referenced grid does not contain a tile with the provided index."))
		return;
	}
	OutNeighborhood.Empty();
	for(int i = -1;i <= 1; i++)
	{
		for(int j = -1;j <= 1; j++)
		{
			if(i == j || i == -j)
			{
				continue;
			}
			FIntVector2 NeighborIndex {TileIndex.X+i,TileIndex.Y+j};
			
			if(TileSetData.Contains(NeighborIndex) && NeighborIndex != TileIndex)
			{
				const auto* NeighborTile = TileSetData.FindChecked(NeighborIndex);
				const auto* OriginalTile = TileSetData.FindChecked(TileIndex);
				
				const bool bWalkable = NeighborTile->IsTileWalkable(MoveTypeToCheck);
				const bool bEnoughJump = FMath::Abs(NeighborTile->Height-OriginalTile->Height) <= JumpPower;
				
				if(bWalkable && bEnoughJump)
				{
					OutNeighborhood.AddUnique(NeighborIndex);
				}
			}
		}
	}
}

bool AGridActor::IsTileSelected(const FIntVector2& TileIndex)
{
	const auto* TargetTileData = *TileDataMap.Find(TileIndex);
	return TargetTileData->TileState & static_cast<uint8>(ETileState::Selected);
}

// Called every frame
void AGridActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const auto NewHoveredTileIndex = GetTileIndexByCursorPosition(0);
	if(NewHoveredTileIndex != HoveredTileIndex) // if selection didn't change, do nothing
	{
		//Return previous selection to default visuals before switching selection if it exists
		if(HoveredTileIndex.X>=0 && !IsTileSelected(HoveredTileIndex))
		{
			UnlightTile(HoveredTileIndex);
		}
		
		HoveredTileIndex = NewHoveredTileIndex;
		if(HoveredTileIndex.X<0)
		{
			return;
		}
		//Handle updating visuals of new selection if it exists
		HighlightTile(HoveredTileIndex);
		
		
	}
	
}

bool AGridActor::ContainsTileWithIndex(const FIntVector2& TileIndex) const
{
	return GridIndexToInstanceIndex.Contains(TileIndex);
}

