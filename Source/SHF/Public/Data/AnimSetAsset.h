// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Engine/DataAsset.h"
#include "AnimSetAsset.generated.h"

USTRUCT(BlueprintType)
struct FMovementAnimSet
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimSequence> IdleAnim;
	
	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UAnimSequence>> IdleBreakAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FCardinalAnimationSet> CycleAnims;
	
	UPROPERTY(EditAnywhere)
	TMap<ESHFGait, FCardinalAnimationSet> StartAnims;
	
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
