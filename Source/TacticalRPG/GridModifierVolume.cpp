// Fill out your copyright notice in the Description page of Project Settings.


#include "GridModifierVolume.h"

#include "GridUtilities.h"


AGridModifierVolume::AGridModifierVolume()
{
	AGridModifierVolume::SetActorHiddenInGame(true);
}

void AGridModifierVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AGridModifierVolume, VolumeAllowedMovement))
	{
		const FLinearColor BrushColor = UGridUtilitiesFunctionLibrary::GenerateVolumeColor(VolumeAllowedMovement);
		const FVector ColorValue {BrushColor.R, BrushColor.G, BrushColor.B};
		GetStaticMeshComponent()->SetVectorParameterValueOnMaterials(TEXT("Color"), ColorValue );
	}
}
