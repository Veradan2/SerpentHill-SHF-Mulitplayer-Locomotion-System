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
	//UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	//bool bShouldMove = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bMovingOnGround = false;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SHF|State")
	bool bIsFlying = false;
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

USTRUCT(BlueprintType)
struct FLocationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly) FVector WorldLocation = FVector::ZeroVector;
	
};

USTRUCT(BlueprintType)
struct FVelocityData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Velocity Data") FVector Velocity = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Velocity Data") float GroundSpeed = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Velocity Data") bool bIsMoving = false;
	
};

USTRUCT(BlueprintType)
struct FLocomotionData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data") ESHFMovementDirection MovementDirection = ESHFMovementDirection::Forward;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data") ESHFMovementDirection AccelerationDirection = ESHFMovementDirection::Forward;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data") float MovementAngle = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data") float SmoothedMovementAngle = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data") float AccelerationAngle = 0.f;
};

USTRUCT(BlueprintType)
struct FAccelerationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Acceleration Data") FVector Acceleration = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Acceleration Data") float AccelAmount = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Acceleration Data") bool bIsAccelerating = false;
};

USTRUCT(BlueprintType)
struct FRotationData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Rotation Data") FRotator ActorRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Rotation Data") FRotator LastFrameActorRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Rotation Data") float DeltaActorYaw = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Rotation Data") float RootYawOffset = 0.f;
};

USTRUCT(BlueprintType)
struct FStrideWarpingData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Warping Data") float Alpha = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Warping Data") float LocomotionSpeed = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Warping Data") float OrientationWarpingLocomotionAngle = 0.f;
};

USTRUCT(BlueprintType)
struct FJumpData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump Data") float TimeToJumpApex = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump Data") float GroundDistance = 1001.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump Data") float JumpPlayrate = 600.f;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump Data") float LandingImpactAlpha = 0.f;
};

USTRUCT(BlueprintType)
struct FLeaningData
{
	GENERATED_BODY()
	
	UPROPERTY (BlueprintReadOnly, Category = "SHF|Leaning Data") float Angle = 0.f;
	UPROPERTY (BlueprintReadOnly, Category = "SHF|Leaning Data") FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY (BlueprintReadOnly, Category = "SHF|Leaning Data") float BlendSpaceGait = 0.f;
	
};

USTRUCT(BlueprintType)
struct FStartStopData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|StartStop") bool bStartFinished = false;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|StartStop") bool bShouldPlayExplosiveStart = false;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|StartStop") bool bShouldPlayDynamicStop = false;
	UPROPERTY(BlueprintReadOnly, Category = "SHF|StartStop") float Stop_DistanceRemaining = 0.f;
};

// Dieses Struct bündelt alles, was wir an die Layer schicken wollen
USTRUCT(BlueprintType)
struct FSHFSharedAnimData {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Global")
	FMovementState MovementState = FMovementState();
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Global")
	ESHFGait Gait = ESHFGait::Run;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Global")
	ESHFEquipMode EquipMode = ESHFEquipMode::Mode1;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Global")
	FCMCStates CMCStates;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Global")
	FCMCMovementConfig MovementConfig;

	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Location Data")
	FLocationData LocationData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Velocity Data")
	FVelocityData VelocityData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion Data")
	FLocomotionData LocomotionData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Acceleration Data")
	FAccelerationData AccelerationData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Rotation Data")
	FRotationData RotationData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Warping Data")
	FStrideWarpingData WarpingData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|TIP Data")
	FTurnInPlaceAnimSet TurnInPlaceAnimSet;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump Data")
	FJumpData JumpData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Leaning Data")
	FLeaningData LeaningData;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|StartStop Data")
	FStartStopData StartStopData;

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
	
	UPROPERTY(BlueprintReadOnly)
	bool bJumpStartLoop2JumpApex = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bJumpFallLoop2FallLand = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bFallLandToEndInAir = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bEndInAirToCycle = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bJumpSelector2JumpStart = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bJumpSelector2JumpApex = false;
	
	UPROPERTY(BlueprintReadOnly)	
	bool bEndInAir2Idle = false;
};


