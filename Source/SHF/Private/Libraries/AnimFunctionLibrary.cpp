// Fill out your copyright notice in the Description page of Project Settings.


#include "Libraries/AnimFunctionLibrary.h"


ESHFMovementDirection UAnimFunctionLibrary::CalculateCardinalDirection(float PawnYaw,
                                                                       ESHFMovementDirection LastCardinalDirection, const float Hysteresis)
{
	
	ESHFMovementDirection NewDir = LastCardinalDirection;
	
	if (LastCardinalDirection == ESHFMovementDirection::Forward) {
		if (PawnYaw > (45.f + Hysteresis)) NewDir = ESHFMovementDirection::Right;
		else if (PawnYaw < (-45.f - Hysteresis)) NewDir = ESHFMovementDirection::Left;
	}
	
	else if (LastCardinalDirection == ESHFMovementDirection::Right) {
		if (PawnYaw < (45.f - Hysteresis)) NewDir = ESHFMovementDirection::Forward;
		else if (PawnYaw > (135.f + Hysteresis)) NewDir = ESHFMovementDirection::Backward;
	}
	
	else if (LastCardinalDirection == ESHFMovementDirection::Left) {
		if (PawnYaw > (-45.f + Hysteresis)) NewDir = ESHFMovementDirection::Forward;
		else if (PawnYaw < (-135.f - Hysteresis)) NewDir = ESHFMovementDirection::Backward;
	}
	
	else if (LastCardinalDirection == ESHFMovementDirection::Backward) {
		if (PawnYaw > 0.f && PawnYaw < (135.f - Hysteresis)) NewDir = ESHFMovementDirection::Right;
		else if (PawnYaw < 0.f && PawnYaw > (-135.f + Hysteresis)) NewDir = ESHFMovementDirection::Left;
	}
	
	return NewDir;
}

float UAnimFunctionLibrary::GetAnimRefSpeed(ESHFGait CurrentGait)
{
	return (CurrentGait == ESHFGait::Walk) ? ANIM_SPEED_WALKING : ANIM_SPEED_JOGGING;
}
