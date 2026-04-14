#pragma once

#include "CoreMinimal.h"
#include "TurnInPlaceTypes.h"
#include "SHFGlobals.generated.h"


USTRUCT(BlueprintType)
struct FMovementState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	bool bGateChanged = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	bool bEquipModeChanged = false;
};




UENUM(BlueprintType)
enum class ESHFAnimLayerTag : uint8
{
	Unarmed,
	Pistol,
	Rifle,
	None
};

UENUM(BlueprintType)
enum class ESHFEquipMode : uint8
{
	Mode1,
	Mode2,
	Mode3
};


USTRUCT(BlueprintType)
struct FTiPAnimationSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* TurnL90 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* TurnR90 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* TurnL180 = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimSequence* TurnR180 = nullptr;
};


USTRUCT(BlueprintType)
struct FCMCStates
{
	GENERATED_BODY()
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bIsFalling = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bIsStrafing = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bIsCrouching = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bTransitionFallToJump = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bShouldMove = false;
};

USTRUCT(BlueprintType)
struct FCMCMovementConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float MaxWalkSpeed = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float MaxAcceleration = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float BrakingDecelerationWalking = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float GroundFriction = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float BrakingFrictionFactor = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	float BrakingFriction  = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Movement Config")
	bool bUseSeparateBrakingFriction = false;
};


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
	Crouch
};

// Dieses Struct bündelt alles, was wir an die Layer schicken wollen
USTRUCT(BlueprintType)
struct FSHFSharedAnimData {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	FMovementState MovementState = FMovementState();
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	FVector WorldLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	float GroundSpeed = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	FVector CharacterVelocity = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	bool bIsMoving = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	bool bIsAtMaxWalkSpeed = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	FVector CharacterAcceleration = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	bool bIsAccelerating = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|TurnInPlace")
	float RootYawOffset = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	float LocomotionAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	float SmoothedLocomotionAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	float AccelerationAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	ESHFGait Gait = ESHFGait::Run;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	ESHFEquipMode EquipMode = ESHFEquipMode::Mode1;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	ESHFMovementDirection MovementDirection = ESHFMovementDirection::Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	ESHFMovementDirection AccelerationDirection = ESHFMovementDirection::Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|State")
	float OrientationWarpingLocomotionAngle = 0.f; // 0 bis 1 (Wie stark soll gestreckt werden?)
	
	UPROPERTY(BlueprintReadOnly)
	float StrideWarpingAlpha = 0.f; // 0 bis 1 (Wie stark soll gestreckt werden?)

	UPROPERTY(BlueprintReadOnly)
	float StrideWarpingLocomotionSpeed = 0.f; // Die tatsächliche Geschwindigkeit (cm/s)
	
	UPROPERTY(BlueprintReadOnly)
	FRotator ActorRotation = FRotator::ZeroRotator;;
	
	UPROPERTY(BlueprintReadOnly)
	FRotator LastFrameActorRotation = FRotator::ZeroRotator;
	
	UPROPERTY(BlueprintReadOnly)
	float DeltaActorYaw = 0.f;

	UPROPERTY(BlueprintReadOnly)
	FCMCStates CMCStates;
	
	UPROPERTY(BlueprintReadOnly)
	FCMCMovementConfig MovementConfig;
	
	UPROPERTY(BlueprintReadOnly)
	FTurnInPlaceAnimSet TurnInPlaceAnimSet;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStartFinished = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bShouldPlayExplosiveStart = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bShouldPlayDynamicStop = false;
	
	UPROPERTY(BlueprintReadOnly)
	float Stop_DistanceRemaining = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	float LeanAngle = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	float LeanBlendSpaceGait = 0.f;
};

USTRUCT(BlueprintTYpe)
struct FTransitionRuleContainer
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	bool bIdle2Start = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStart2Cycle = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bCycle2Idle = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStart2CycleCond2 = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bCycle2Stop = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStop2Idle = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStop2Start = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStart2Stop = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIdle2Cycle = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bStart2Idle = false;
};


