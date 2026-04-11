// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimFunctionLibrary.generated.h"


#define ANIM_SPEED_WALKING 160.f
#define ANIM_SPEED_JOGGING 380.f

/**
 * 
 */
UCLASS()
class SHF_API UAnimFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	static ESHFMovementDirection CalculateCardinalDirection(float PawnYaw, ESHFMovementDirection LastCardinalDirection, float Hysteresis = 10.f);
	static float GetAnimRefSpeed(ESHFGait CurrentGait);
	
};
