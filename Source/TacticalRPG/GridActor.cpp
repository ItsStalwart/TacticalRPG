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
			const float RowOffset = GridStep * (j%2) *.5f; //shift every other row half a step
			FVector TileLocation {GridStep * i + RowOffset, GridStep*j*FMathf::Sin(60*FMathf::DegToRad), 0};
			FTransform TileTransform;
			if(bUseEnvironment)
			{
				FVector NewLocation{};
				const bool bSpawnTile = TraceForGround(GetActorLocation() + TileLocation,NewLocation);
				if(bSpawnTile)
				{
					TileTransform = FTransform{NewLocation-GetActorLocation()+FVector{0,0,1}};
					AddTileAt(TileTransform, {i,j});
				}
				
				continue;
			}
			TileTransform = FTransform{TileLocation};
			AddTileAt(TileTransform, {i,j});
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

bool AGridActor::TraceForGround(FVector TraceStartLocation, FVector& TraceHitLocation) const
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
			//TODO:Set other movement types behavior
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
                          const uint8 UnitMovementType, const int UnitJumpPower) const
{
	OutPath.Empty();
	TArray<FIntVector2> OpenList{};
	OpenList.AddUnique(StartIndex);
	TArray<FIntVector2> ClosedList{};
	for (auto& [Index, Tile] : TileDataMap)
	{
		Tile->ResetConnectedTile();
		Tile->SetGValue(MAX_int32);
	}
	auto* StartNode = TileDataMap.FindChecked(StartIndex);
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
		GetWalkableNeighbors(CurrentIndex,Neighbors, UnitMovementType, UnitJumpPower);
		for (auto& Index : Neighbors)
		{
			if(ClosedList.Contains(Index))
			{
				continue;
			}
			int TentativeGValue = GetTileGValueByIndex(CurrentIndex) + GetDistanceBetweenTiles(CurrentIndex, Index);
			if(TentativeGValue < GetTileGValueByIndex(Index))
			{
				auto* TargetTile = TileDataMap.FindChecked(Index);
				TargetTile->SetConnectedTile(CurrentIndex);
				TargetTile->SetGValue(TentativeGValue);
				TargetTile->SetHValue(GetDistanceBetweenTiles(Index,TargetIndex));
				OpenList.AddUnique(Index);
			}
		}
	}
	return false;
}

int AGridActor::GetTileGValueByIndex(const FIntVector2& TileIndex) const
{
	if(!ContainsTileWithIndex(TileIndex))
	{
		return INT_MAX;
	}
	return TileDataMap.FindChecked(TileIndex)->GetGValue();
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

void AGridActor::AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex)
{
	const int InstanceIndex = InstancedStaticMeshComponent->GetInstanceCount();
	UTileData* TileData = NewObject<UTileData>();
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
		TArray<FIntVector2> Path;
		if(FindPath({0,0},HoveredTileIndex,Path))
		{
			for (auto& Index : Path)
			{
				HighlightTile(Index);
			}
		}
	}
}

void AGridActor::GetTileNeighborhood(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood) const
{
	if(TileIndex.X <0 || TileIndex.Y <0 || !this->ContainsTileWithIndex(TileIndex))
	{
		UE_LOG(LogTemp,Warning, TEXT("Referenced grid does not contain a tile with the provided index."))
		return;
	}
	OutNeighborhood.Empty();
	if(TileIndex.Y % 2 == 0)
	{
		//(X-1,Y),(X+1,Y),(X-1,Y-1),(X-1,Y+1),(X,Y-1),(X,Y+1)
		for(int i = TileIndex.X-1;i<=TileIndex.X; i++)
		{
			for(int j = TileIndex.Y-1;j<=TileIndex.Y + 1; j++)
			{
				FIntVector2 NeighborIndex {i,j};
				if(this->ContainsTileWithIndex(NeighborIndex) && NeighborIndex != TileIndex)
				{
					OutNeighborhood.AddUnique(NeighborIndex);
				}
			}
		}
		const FIntVector2 ExceptionNeighbor {TileIndex.X+1, TileIndex.Y};
		if(this->ContainsTileWithIndex(ExceptionNeighbor))
		{
			OutNeighborhood.AddUnique(ExceptionNeighbor);
		}
		return;
	}
	//(X-1,Y),(X+1,Y),(X,Y-1),(X+1,Y-1),(X,Y+1),(X+1,Y+1)
	for(int i = TileIndex.X;i<=TileIndex.X + 1; i++)
	{
		for(int j = TileIndex.Y-1;j<=TileIndex.Y + 1; j++)
		{
			FIntVector2 NeighborIndex {i,j};
			if(this->ContainsTileWithIndex(NeighborIndex) && NeighborIndex != TileIndex)
			{
				OutNeighborhood.AddUnique(NeighborIndex);
			}
		}
	}
	const FIntVector2 ExceptionNeighbor {TileIndex.X-1, TileIndex.Y};
	if(this->ContainsTileWithIndex(ExceptionNeighbor))
	{
		OutNeighborhood.AddUnique(ExceptionNeighbor);
	}
}

void AGridActor::GetWalkableNeighbors(const FIntVector2& TileIndex, TArray<FIntVector2>& OutNeighborhood,
	const uint8 MoveTypeToCheck, int JumpPower) const
{
	if(TileIndex.X <0 || TileIndex.Y <0 || !this->ContainsTileWithIndex(TileIndex))
	{
		UE_LOG(LogTemp,Warning, TEXT("Referenced grid does not contain a tile with the provided index."))
		return;
	}
	OutNeighborhood.Empty();
	if(TileIndex.Y % 2 == 0)
	{
		//(X-1,Y),(X+1,Y),(X-1,Y-1),(X-1,Y+1),(X,Y-1),(X,Y+1)
		for(int i = TileIndex.X-1;i<=TileIndex.X; i++)
		{
			for(int j = TileIndex.Y-1;j<=TileIndex.Y + 1; j++)
			{
				FIntVector2 NeighborIndex {i,j};
				
				if(this->ContainsTileWithIndex(NeighborIndex) && NeighborIndex != TileIndex)
				{
					const auto* NeighborTile = TileDataMap.FindChecked(NeighborIndex);
					const auto* OriginalTile = TileDataMap.FindChecked(TileIndex);
					
					const bool bWalkable = NeighborTile->IsTileWalkable(MoveTypeToCheck);
					const bool bEnoughJump = FMath::Abs(NeighborTile->Height-OriginalTile->Height) <= JumpPower;
					
					if(bWalkable && bEnoughJump)
					{
						OutNeighborhood.AddUnique(NeighborIndex);
					}
				}
			}
		}
		const FIntVector2 ExceptionNeighbor {TileIndex.X+1, TileIndex.Y};
		if(this->ContainsTileWithIndex(ExceptionNeighbor) && ExceptionNeighbor != TileIndex)
		{
			const bool bWalkable = TileDataMap.FindChecked(ExceptionNeighbor)->IsTileWalkable(MoveTypeToCheck);
			const bool bEnoughJump = FMath::Abs(TileDataMap.FindChecked(ExceptionNeighbor)->Height-TileDataMap.FindChecked(TileIndex)->Height) <= JumpPower;
			if(bWalkable && bEnoughJump)
			{
				OutNeighborhood.AddUnique(ExceptionNeighbor);
			}
		}
		return;
	}
	//(X-1,Y),(X+1,Y),(X,Y-1),(X+1,Y-1),(X,Y+1),(X+1,Y+1)
	for(int i = TileIndex.X;i<=TileIndex.X + 1; i++)
	{
		for(int j = TileIndex.Y-1;j<=TileIndex.Y + 1; j++)
		{
			FIntVector2 NeighborIndex {i,j};
			if(this->ContainsTileWithIndex(NeighborIndex) && NeighborIndex != TileIndex)
			{
				const bool bWalkable = TileDataMap.FindChecked(NeighborIndex)->IsTileWalkable(MoveTypeToCheck);
				const bool bEnoughJump = FMath::Abs(TileDataMap.FindChecked(NeighborIndex)->Height-TileDataMap.FindChecked(TileIndex)->Height) <= JumpPower;
				if(bWalkable && bEnoughJump)
				{
					OutNeighborhood.AddUnique(NeighborIndex);
				}
			}
		}
	}
	const FIntVector2 ExceptionNeighbor {TileIndex.X-1, TileIndex.Y};
	if(this->ContainsTileWithIndex(ExceptionNeighbor) && ExceptionNeighbor != TileIndex)
	{
		const bool bWalkable = TileDataMap.FindChecked(ExceptionNeighbor)->IsTileWalkable(MoveTypeToCheck);
		const bool bEnoughJump = FMath::Abs(TileDataMap.FindChecked(ExceptionNeighbor)->Height-TileDataMap.FindChecked(TileIndex)->Height) <= JumpPower;
		if(bWalkable && bEnoughJump)
		{
			OutNeighborhood.AddUnique(ExceptionNeighbor);
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

