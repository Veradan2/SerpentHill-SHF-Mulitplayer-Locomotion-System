#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.generated.h"


UENUM(BlueprintType)
enum class ESHFMovementDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};


USTRUCT(BlueprintType)
struct FCardinalAnimationSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimSequence> Forward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimSequence> Backward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimSequence> Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
	TObjectPtr<UAnimSequence> Right;

	// Eine kleine Hilfsfunktion, um die richtige Animation direkt abzugreifen
	UAnimSequence* SelectAnim(ESHFMovementDirection Direction) const
	{
		switch (Direction)
		{
		case ESHFMovementDirection::Forward:  return Forward;
		case ESHFMovementDirection::Backward: return Backward;
		case ESHFMovementDirection::Left:     return Left;
		case ESHFMovementDirection::Right:    return Right;
		default: return Forward;
		}
	}
};



UENUM(BlueprintType)
enum class ESHFTurnState : uint8 {
	None,
	TurnLeft90,
	TurnRight90,
	Turn180
};

UENUM(BlueprintType)
enum class ESHFGait : uint8 {
	Walk,
	Run,
	Sprint
};

// Dieses Struct bündelt alles, was wir an die Layer schicken wollen
USTRUCT(BlueprintType)
struct FSHFSharedAnimData {
	GENERATED_BODY()
	
	// Grundlegende Bewegung
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	bool bIsMoving = false;

	// Turn In Place Daten
	UPROPERTY(BlueprintReadOnly, Category = "SHF|TurnInPlace")
	ESHFTurnState TurnState = ESHFTurnState::None;

	UPROPERTY(BlueprintReadOnly, Category = "SHF|TurnInPlace")
	float RootYawOffset = 0.f;

	// Erweiterbar für später (z.B. Gait/Haltung)
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	bool bIsCrouching = false;	
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	float LocomotionAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	ESHFMovementDirection MovementDirection = ESHFMovementDirection::Forward;
};


