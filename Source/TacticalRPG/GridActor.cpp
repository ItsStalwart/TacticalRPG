// Fill out your copyright notice in the Description page of Project Settings.


#include "GridActor.h"
#include "GridData.h"
#include "GridModifierVolume.h"
#include "GridUtilities.h"
#include "MathUtil.h"
#include "Components/InstancedStaticMeshComponent.h"
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
		InstancedStaticMeshComponent->GetStaticMesh()->SetMaterial(0, SetGridData->GetTileBorderMaterial().LoadSynchronous());
	}
}

// Called when the game starts or when spawned
void AGridActor::BeginPlay()
{
	Super::BeginPlay();
	
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
	InstancedStaticMeshComponent->GetStaticMesh()->SetMaterial(0, SetGridData->GetTileBorderMaterial().LoadSynchronous());
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
	GridIndexToArrayIndex.Empty();
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
	GridIndexToArrayIndex.Add({GridIndex.X,GridIndex.Y}, InstancedStaticMeshComponent->GetInstanceCount());
	InstancedStaticMeshComponent->AddInstance(TileTransform);
}

bool AGridActor::RemoveTileAt(const FIntVector2& GridIndexToRemove)
{
	if(!GridIndexToArrayIndex.Contains(GridIndexToRemove))
	{
		return false;
	}
	const int TargetIndex = GridIndexToArrayIndex.FindAndRemoveChecked(GridIndexToRemove);
	InstancedStaticMeshComponent->RemoveInstance(TargetIndex);
	return true;
}

// Called every frame
void AGridActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

