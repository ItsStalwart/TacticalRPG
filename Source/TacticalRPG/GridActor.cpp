// Fill out your copyright notice in the Description page of Project Settings.


#include "GridActor.h"

#include "Editor.h"
#include "GridData.h"
#include "GridModifierVolume.h"
#include "GridUtilities.h"
#include "MathUtil.h"
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
		//InstancedStaticMeshComponent->GetStaticMesh()->SetMaterial(0, SetGridData->GetTileBorderMaterial().LoadSynchronous());
	}
}

// Called when the game starts or when spawned
void AGridActor::BeginPlay()
{
	Super::BeginPlay();

	RegenerateEnvironmentGrid();
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
	//InstancedStaticMeshComponent->GetStaticMesh()->SetMaterial(0, SetGridData->GetTileBorderMaterial().LoadSynchronous());
	
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

void AGridActor::AddTileAt(const FTransform& TileTransform, const FIntVector2& GridIndex)
{
	GridIndexToInstanceIndex.Add({GridIndex.X,GridIndex.Y}, InstancedStaticMeshComponent->GetInstanceCount());
	InstancedStaticMeshComponent->AddInstance(TileTransform);
}

bool AGridActor::RemoveTileAt(const FIntVector2& GridIndexToRemove)
{
	if(!GridIndexToInstanceIndex.Contains(GridIndexToRemove))
	{
		return false;
	}
	const int TargetIndex = GridIndexToInstanceIndex.FindAndRemoveChecked(GridIndexToRemove);
	InstancedStaticMeshComponent->RemoveInstance(TargetIndex);
	return true;
}

void AGridActor::HighlightTile(int TargetTile)
{
	InstancedStaticMeshComponent->SetCustomDataValue(TargetTile, 0, 1,true);
}

void AGridActor::UnlightTile(int TargetTile)
{
	InstancedStaticMeshComponent->SetCustomDataValue(TargetTile, 0, 0, true);
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
		GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, FString::Format(TEXT("Player cursor at World Location: {0},{1},{2}. No tile at location"), {CursorProjectionInGrid.X, CursorProjectionInGrid.Y, CursorProjectionInGrid.Z}));
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
	
	auto TileIndex = *GridIndexToInstanceIndex.FindKey(InstancesNearCursorByIndex[0]);
#if WITH_EDITOR
	GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, FString::Format(TEXT("Player cursor at World Location: {0},{1},{2}. Closest Tile: {3}, {4}"), {CursorProjectionInGrid.X, CursorProjectionInGrid.Y, CursorProjectionInGrid.Z, TileIndex.X, TileIndex.Y}));
#endif
	return {TileIndex};
	
}

// Called every frame
void AGridActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto NewSelectedTileIndex = GetTileIndexByCursorPosition(0);
	if(NewSelectedTileIndex != SelectedTileIndex) // if selection didn't change, do nothing
	{
		//Return previous selection to default visuals before switching selection if it exists
		if(SelectedTileIndex.X>=0)
		{
			UnlightTile(*GridIndexToInstanceIndex.Find(SelectedTileIndex));
		}
		
		SelectedTileIndex = NewSelectedTileIndex;
		if(SelectedTileIndex.X<0)
		{
			return;
		}
		//Handle updating visuals of new selection if it exists
		HighlightTile(*GridIndexToInstanceIndex.Find(SelectedTileIndex));
		
	}
	
}

