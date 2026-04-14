// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Engine/DataAsset.h"
#include "AnimSetAsset.generated.h"


USTRUCT(BlueprintType) 
struct FAnimSequenceArray
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UAnimSequence>> AnimArray;
};



USTRUCT(BlueprintType)
struct FMovementAnimSet
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, TObjectPtr<UAnimSequence>> IdleAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FAnimSequenceArray> IdleBreakAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FCardinalAnimationSet> CycleAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FCardinalAnimationSet> StartAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FCardinalAnimationSet> StopAnims;
	
	UPROPERTY(EditAnywhere)
	FTurnInPlaceAnimSet TurnInPlaceAnimSet;
	
	UPROPERTY(EditAnywhere)
	FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched;
};



/**
 * 
 */
UCLASS()
class SHF_API UAnimSetAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, Category = "SHF|AnimSet")
	TMap<ESHFEquipMode,FMovementAnimSet> AnimSet;
};
