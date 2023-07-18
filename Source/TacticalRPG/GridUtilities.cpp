// Fill out your copyright notice in the Description page of Project Settings.


#include "GridUtilities.h"

FLinearColor UGridUtilitiesFunctionLibrary::GenerateVolumeColor(const int MovementCombination)
{
	FLinearColor ReturnColor{FLinearColor::Black};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Ground)) {ReturnColor+= FLinearColor::Red;};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Aquatic)) {ReturnColor+= FLinearColor::Blue;};
	if( MovementCombination & static_cast<uint8>(EGridMovementType::Aerial)) {ReturnColor+= FLinearColor::Green;};
	return ReturnColor;
}
