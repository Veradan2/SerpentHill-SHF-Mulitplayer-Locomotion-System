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
struct FJumpAnimSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe))	TObjectPtr<UAnimSequence> Start = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe)) TObjectPtr<UAnimSequence> StartLoop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe))	TObjectPtr<UAnimSequence> Apex = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe))	TObjectPtr<UAnimSequence> FallLoop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe))	TObjectPtr<UAnimSequence> Land = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BlueprintThreadSafe))	TObjectPtr<UAnimSequence> Recovery = nullptr;
};


USTRUCT(BlueprintType)
struct FMovementAnimSet
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESHFGait, TObjectPtr<UAnimSequence>> IdleAnims;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESHFGait, FAnimSequenceArray> IdleBreakAnims;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESHFGait, FCardinalAnimationSet> CycleAnims;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESHFGait, FCardinalAnimationSet> StartAnims;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<ESHFGait, FCardinalAnimationSet> StopAnims;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTurnInPlaceAnimSet TurnInPlaceAnimSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FJumpAnimSet JumpAnimSet;
};



/**
 * 
 */
UCLASS()
class SHF_API UAnimSetAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SHF|AnimSet")
	TMap<ESHFEquipMode,FMovementAnimSet> AnimSet;
};
