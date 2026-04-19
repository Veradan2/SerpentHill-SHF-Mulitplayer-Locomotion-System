// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/AnimFunctionLibrary.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/CharacterMovementComponent.h"



ESHFMovementDirection UAnimFunctionLibrary::CalculateCardinalDirection(float PawnYaw, ESHFMovementDirection CurrentMovementDirection, const float Hysteresis, float ForwardMin, float
                                                                       ForwardMax, float BackwardMin, float BackwardMax)
{
	switch (CurrentMovementDirection)
    {
        case ESHFMovementDirection::Forward:
            if (PawnYaw >= (ForwardMin - Hysteresis) && PawnYaw <= (ForwardMax + Hysteresis))
                return ESHFMovementDirection::Forward;
            break;

        case ESHFMovementDirection::Right:
            if (PawnYaw >= (ForwardMax - Hysteresis) && PawnYaw <= (BackwardMax + Hysteresis))
                return ESHFMovementDirection::Right;
            break;

        case ESHFMovementDirection::Left:
            if (PawnYaw >= (-BackwardMax - Hysteresis) && PawnYaw <= (ForwardMin + Hysteresis))
                return ESHFMovementDirection::Left;
            break;

        case ESHFMovementDirection::Backward:
            // Backward ist gesplittet (130 bis 180 und -180 bis -130)
            if (PawnYaw > (BackwardMax - Hysteresis) || PawnYaw < (-BackwardMax + Hysteresis))
                return ESHFMovementDirection::Backward;
            break;
    }

    if (PawnYaw >= ForwardMin && PawnYaw <= ForwardMax) 
        return ESHFMovementDirection::Forward;
    
    if (PawnYaw > ForwardMax && PawnYaw < BackwardMax) 
        return ESHFMovementDirection::Right;
    
    if (PawnYaw < ForwardMin && PawnYaw > -BackwardMax) 
        return ESHFMovementDirection::Left;

    return ESHFMovementDirection::Backward;
}


float UAnimFunctionLibrary::GetAnimRefSpeed(ESHFGait CurrentGait)
{
	return (CurrentGait == ESHFGait::Walk) ? ANIM_SPEED_WALKING : ANIM_SPEED_JOGGING;
}

float UAnimFunctionLibrary::PredictTimeToJumpApex(const FVector& PawnVelocity,
    const UCharacterMovementComponent* MovementComponent)
{
    if (MovementComponent->IsFalling())
        return (0.f - PawnVelocity.Z ) / FMath::Max(0.001f, MovementComponent->GetGravityZ());
    
    return 0.f;
}

