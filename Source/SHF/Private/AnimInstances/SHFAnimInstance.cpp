// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
#include "TurnInPlace.h"
#include "TurnInPlaceStatics.h"
#include "Animation/AnimNodeMessages.h"
#include "Animation/AnimInertializationRequest.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFLayerAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Dataflow/DataflowContent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Libraries/AnimFunctionLibrary.h"
#include "SHF/Components/AnimComponent.h"

void FSHFAnimInstanceProxy::SetRootYawOffset(float NewYawOffset)
{
	SharedData.RotationData.RootYawOffset = NewYawOffset;
}

void FSHFAnimInstanceProxy::Update(float DeltaSeconds)
{
	FAnimInstanceProxy::Update(DeltaSeconds);
}

void USHFAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	OwningPawn = TryGetPawnOwner();
	if (OwningPawn)
	{
		// 2. Direkt die AnimComponent suchen und speichern
		AnimComp = OwningPawn->FindComponentByClass<UAnimComponent>();
        
		if (AnimComp)
		{
			AnimComp->RegisterMainAnimInstance(this);
			CurrentMovementGait = AnimComp->GetCurrentMovementGait();	//To be sure to have all values synced at the very first time 
			CurrentEquipMode = AnimComp->GetCurrentEquipMode();
		} else 	UE_LOG(LogTemp, Warning, TEXT("SHFAnimInstance: Cannot find AnimComponent (%s)"), *OwningPawn->GetName());
		GetReferences();
		
		TurnInPlaceComp = OwningPawn->FindComponentByClass<UTurnInPlace>();
		checkf(IsValid(TurnInPlaceComp), TEXT("SHFAnimInstance: Cannot find TurnInPlaceComponent (%s)"), *OwningPawn->GetName() );
	}
	
	PreviousMovementGait = CurrentMovementGait;
	PreviousEquipMode = CurrentEquipMode;
}

void USHFAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!OwningPawn || !AnimComp)
	{
		NativeInitializeAnimation();
		if (!OwningPawn || !AnimComp) return;
	}
	
	//CharacterRef && MovementCompRef
	if (!GetReferences()) return;
	
	FSHFAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFAnimInstanceProxy>();

	
	
	// 1. Paket erstellen
	FSHFSharedAnimData NewData = Proxy.SharedData;
	NewData.LocationData.WorldLocation = OwningPawn->GetActorLocation();
	NewData.Gait = AnimComp->GetCurrentMovementGait();
	NewData.EquipMode = AnimComp->GetCurrentEquipMode();
	
	GatherVelocityData(NewData.VelocityData);
	GatherAccelerationData(NewData.AccelerationData);
	GatherRotationData(NewData.RotationData);
	GatherLocomotionData(NewData, DeltaSeconds);
	GatherCMCStates(NewData.CMCStates);
	
	NewData.StartStopData.bShouldPlayExplosiveStart = NewData.AccelerationData.bIsAccelerating && (NewData.AccelerationData.AccelAmount > 0.7f);
	NewData.StartStopData.bShouldPlayDynamicStop = NewData.VelocityData.GroundSpeed > 150.f;	
	
	//NewData.OrientationWarpingLocomotionAngle = -NewData.LocomotionAngle;	//this is calculated by the layers which need the value, LocomotionAngle also works
	
	NewData.WarpingData.LocomotionSpeed = NewData.VelocityData.GroundSpeed;
	NewData.WarpingData.Alpha = FMath::Clamp(NewData.VelocityData.GroundSpeed / 50.f, 0.f, 1.f);
	
	
	
	NewData.CMCStates.bIsCrouching = MovementCompRef->IsCrouching();
	NewData.CMCStates.bIsFalling = MovementCompRef->IsFalling();
	NewData.CMCStates.bIsStrafing = !MovementCompRef->bOrientRotationToMovement;
	NewData.CMCStates.bTransitionFallToJump = NewData.CMCStates.bIsFalling && NewData.VelocityData.Velocity.Z > 100.0;
	//NewData.CMCStates.bShouldMove = NewData.VelocityData.GroundSpeed > 3.f && MovementCompRef->GetCurrentAcceleration().Size() > 0.f;
	NewData.CMCStates.bMovingOnGround = MovementCompRef->IsMovingOnGround();
	NewData.CMCStates.bIsFlying = MovementCompRef->IsFlying();
	
	NewData.MovementConfig.MaxWalkSpeed = MovementCompRef->MaxWalkSpeed;
	NewData.MovementConfig.MaxAcceleration = MovementCompRef->MaxAcceleration;
	NewData.MovementConfig.GroundFriction = MovementCompRef->GroundFriction;
	NewData.MovementConfig.BrakingFriction = MovementCompRef->BrakingFriction;
	NewData.MovementConfig.BrakingDecelerationWalking = MovementCompRef->BrakingDecelerationWalking;
	NewData.MovementConfig.BrakingFrictionFactor = MovementCompRef->BrakingFrictionFactor;
	NewData.MovementConfig.bUseSeparateBrakingFriction = MovementCompRef->bUseSeparateBrakingFriction;
	
	
	NewData.MovementState.bGateChanged = CurrentMovementGait != PreviousMovementGait;
	NewData.MovementState.bEquipModeChanged = CurrentEquipMode != PreviousEquipMode;
	
	CalculateMovementDirection(DeltaSeconds, NewData);
	UpdateJumpFallData(NewData, Proxy.SharedData, DeltaSeconds);
	TraceGroundDistance(NewData);
	
	NewData.JumpData.JumpPlayrate = FMath::Clamp(NewData.VelocityData.Velocity.Z / FMath::Max(0.1f, MovementCompRef->JumpZVelocity), 0.8f, 1.5f);
	
	CalculateLeaningData(NewData, Proxy.SharedData, DeltaSeconds);
	
	/* Daten in den Proxy schreiben */

	Proxy.SharedData = NewData;
	
	// 2. Daten an alle Layer "pushen"
	for (USHFLayerAnimInstance* Layer : LinkedLayers) {
		if (IsValid(Layer)) {
			Layer->UpdateFromMain(NewData);
		}
	}
	UTurnInPlaceStatics::UpdateTurnInPlace(TurnInPlaceComp, DeltaSeconds, TurnData, NewData.CMCStates.bIsStrafing,
	                                       TurnOutput, bCanUpdateTurnInPlace);
	
	PreviousMovementGait = CurrentMovementGait;
	PreviousEquipMode = CurrentEquipMode;
}


void USHFAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	
	for (const USHFLayerAnimInstance* Layer : LinkedLayers)
	{
		if (IsValid(Layer) && Layer->bStartFinished)
		{
			Proxy.SharedData.StartStopData.bStartFinished = true;
			break;
		}
	}
	
	for (const USHFLayerAnimInstance* Layer : LinkedLayers)
	{
		if (IsValid(Layer) && (Layer->Stop_DistanceRemaining != 0.f))
		{
			Proxy.SharedData.StartStopData.Stop_DistanceRemaining = Layer->Stop_DistanceRemaining;
			break;
		}
	}
	
	SharedData = Proxy.SharedData;
	
	TurnInPlaceCurveValues = UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceCurveValues(this, TurnData);
	
	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlace(TurnData, bCanUpdateTurnInPlace, Proxy.SharedData.CMCStates.bIsStrafing, TurnOutput);
	
	TransitionRules.bIdle2Start = Proxy.SharedData.StartStopData.bShouldPlayExplosiveStart;
	TransitionRules.bStart2Cycle = FMath::IsNearlyEqual(Proxy.SharedData.VelocityData.GroundSpeed, Proxy.SharedData.MovementConfig.MaxWalkSpeed, 5.f);
	TransitionRules.bStart2CycleCond2 = Proxy.SharedData.MovementState.bGateChanged || Proxy.SharedData.MovementState.bEquipModeChanged;
	TransitionRules.bStop2Start = Proxy.SharedData.AccelerationData.bIsAccelerating;
	TransitionRules.bStart2Stop = !Proxy.SharedData.AccelerationData.bIsAccelerating;
	TransitionRules.bIdle2Cycle = Proxy.SharedData.AccelerationData.bIsAccelerating && !Proxy.SharedData.StartStopData.bShouldPlayExplosiveStart;

	// 1. Der Weg in den Stop (Nur wenn wir schnell genug für die Anim sind)
	TransitionRules.bCycle2Stop = !Proxy.SharedData.AccelerationData.bIsAccelerating && Proxy.SharedData.StartStopData.bShouldPlayDynamicStop;

	// 2. Der Weg direkt nach Idle (Sanftes Ausrollen ohne Stop-Layer)
	// WICHTIG: Nur wenn wir NICHT in den Stop-Layer gehen, aber auch nicht mehr beschleunigen.
	TransitionRules.bCycle2Idle = !Proxy.SharedData.AccelerationData.bIsAccelerating && !Proxy.SharedData.StartStopData.bShouldPlayDynamicStop;

	// 3. Stop beenden
	// Wir erhöhen den Schwellenwert leicht, um das Pendeln zu verhindern (Hysterese)
	TransitionRules.bStop2Idle = Proxy.SharedData.VelocityData.GroundSpeed < 10.f || Proxy.SharedData.StartStopData.Stop_DistanceRemaining < 5.f;

	// 4. Start-Abbruch (Wichtig!)
	// Wenn wir im Start sind und loslassen, gehen wir je nach Speed in den Stop oder direkt nach Idle
	TransitionRules.bStart2Stop = !Proxy.SharedData.AccelerationData.bIsAccelerating && Proxy.SharedData.StartStopData.bShouldPlayDynamicStop;
	// Falls wir sehr langsam waren, direkt zurück nach Idle
	TransitionRules.bStart2Idle = !Proxy.SharedData.AccelerationData.bIsAccelerating && !Proxy.SharedData.StartStopData.bShouldPlayDynamicStop;
	
	TransitionRules.bJumpStartLoop2JumpApex = Proxy.SharedData.JumpData.TimeToJumpApex < .4f;
	TransitionRules.bJumpFallLoop2FallLand = Proxy.SharedData.JumpData.GroundDistance < 200.f;
	TransitionRules.bFallLandToEndInAir = Proxy.SharedData.CMCStates.bMovingOnGround || Proxy.SharedData.CMCStates.bIsFlying;
	TransitionRules.bEndInAirToCycle = !Proxy.SharedData.CMCStates.bIsFalling && Proxy.SharedData.AccelerationData.bIsAccelerating;
	TransitionRules.bEndInAir2Idle = !Proxy.SharedData.CMCStates.bIsFalling && !Proxy.SharedData.AccelerationData.bIsAccelerating;
	TransitionRules.bJumpSelector2JumpStart = Proxy.SharedData.CMCStates.bIsFalling && Proxy.SharedData.VelocityData.Velocity.Z > 100.f;
	TransitionRules.bJumpSelector2JumpApex = Proxy.SharedData.CMCStates.bIsFalling;
}

void USHFAnimInstance::RegisterLayer(USHFLayerAnimInstance* Layer)
{
	// Garbage collecction: Unlink all unused layers (I M P O R T A N T, is causing crashes otherwise) 
	LinkedLayers.RemoveAll([](TObjectPtr<USHFLayerAnimInstance> L) {
		return L == nullptr || !L->GetSkelMeshComponent()->GetLinkedAnimLayerInstanceByClass(L->GetClass());
	});

	LinkedLayers.AddUnique(Layer);	
}


FTurnInPlaceCurveValues USHFAnimInstance::GetTurnInPlaceCurveValues_Implementation() const
{
	return TurnInPlaceCurveValues;
}

FTurnInPlaceAnimSet USHFAnimInstance::GetTurnInPlaceAnimSet_Implementation() const
{
	// Suche den Layer, der das Interface/die Funktion implementiert
	for (USHFLayerAnimInstance* Layer : LinkedLayers)
	{
		if (IsValid(Layer))
		{
			if (Layer->Implements<UTurnInPlaceAnimInterface>())
				return ITurnInPlaceAnimInterface::Execute_GetTurnInPlaceAnimSet(Layer);
		}
	}
	return TurnData.AnimSet; // Fallback	
}

void USHFAnimInstance::ReceiveNewEquipMode(ESHFEquipMode NewEquipMode)
{
	if (NewEquipMode != CurrentEquipMode)
	{
		CurrentEquipMode = NewEquipMode;
	}
}

void USHFAnimInstance::ReceiveNewGait(ESHFGait NewMovementGait)
{
	if (NewMovementGait != CurrentMovementGait)
	{
		CurrentMovementGait = NewMovementGait;
	}
}

FAnimInstanceProxy* USHFAnimInstance::CreateAnimInstanceProxy()
{
	return new FSHFAnimInstanceProxy(this);
}

void USHFAnimInstance::SetupIdle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	
	TurnInPlaceGraphNodeData.TurnPlayRate = 0.f;
	TurnInPlaceGraphNodeData.bHasReachedMaxTurnAngle = false;
}

void USHFAnimInstance::SetupTurnInPlace(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		TurnInPlaceGraphNodeData.AnimStateTime = 0.f;
		USequenceEvaluatorLibrary::SetSequence(SequenceEvaluator, GetTurnAnimation(false));
		USequenceEvaluatorLibrary::SetExplicitTime(SequenceEvaluator, 0.f);
		TurnInPlaceGraphNodeData.bHasReachedMaxTurnAngle = false;
		UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceNode(TurnInPlaceGraphNodeData, TurnData, GetTurnAnimSet());
	}
}

void USHFAnimInstance::UpdateTurnInPlace(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
	
	if (UAnimSequence* AnimSeq = GetTurnAnimation(false); Result == EAnimNodeReferenceConversionResult::Succeeded )
	{
		float AnimStateTime = UTurnInPlaceStatics::GetUpdatedTurnInPlaceAnimTime_ThreadSafe(AnimSeq, TurnInPlaceGraphNodeData.AnimStateTime,
									Context.GetContext()->GetDeltaTime(), TurnInPlaceGraphNodeData.TurnPlayRate);
		
		TurnInPlaceGraphNodeData.AnimStateTime = AnimStateTime;
		USequenceEvaluatorLibrary::SetExplicitTime(SequenceEvaluator, AnimStateTime);
		UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceNode(TurnInPlaceGraphNodeData, TurnData, GetTurnAnimSet());
	}	
}

void USHFAnimInstance::SetupTurnAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	TurnInPlaceGraphNodeData.bIsTurningRight = TurnData.bTurnRight;
	TurnInPlaceGraphNodeData.StepSize = TurnData.StepSize;
}

void USHFAnimInstance::UpdateTurnInPlaceRecovery(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	if (UAnimSequence* AnimSeq = GetTurnAnimation(true); Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, AnimSeq, 0.2f);
	}
}

void USHFAnimInstance::SetupTurnRecovery(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	TurnInPlaceGraphNodeData.bIsRecoveryTurningRight = TurnInPlaceGraphNodeData.bIsTurningRight;
}

void USHFAnimInstance::GatherVelocityData(FVelocityData& OutVelocityData)
{
	OutVelocityData.Velocity =OwningPawn->GetVelocity();
	OutVelocityData.GroundSpeed = OutVelocityData.Velocity.Size2D();
	OutVelocityData.bIsMoving = OutVelocityData.GroundSpeed > 5.f;
}

void USHFAnimInstance::GatherAccelerationData(FAccelerationData& OutAccelerationData)
{
	OutAccelerationData.Acceleration = MovementCompRef->GetCurrentAcceleration();
	OutAccelerationData.bIsAccelerating = OutAccelerationData.Acceleration.Size2D() > 0.f;
	OutAccelerationData.AccelAmount = OutAccelerationData.Acceleration.Size2D() / MovementCompRef->MaxAcceleration;
}

void USHFAnimInstance::GatherRotationData(FRotationData& OutRotationData)
{
	OutRotationData.LastFrameActorRotation = OutRotationData.ActorRotation;
	OutRotationData.ActorRotation = OwningPawn->GetActorRotation();
	OutRotationData.DeltaActorYaw = UKismetMathLibrary::NormalizeAxis(OutRotationData.ActorRotation.Yaw - OutRotationData.LastFrameActorRotation.Yaw);
	OutRotationData.RootYawOffset += OutRotationData.DeltaActorYaw;
}

void USHFAnimInstance::GatherLocomotionData(FSHFSharedAnimData& OutAnimData, float DeltaSeconds)
{
	OutAnimData.LocomotionData.MovementAngle = UKismetAnimationLibrary::CalculateDirection(OutAnimData.VelocityData.Velocity, OutAnimData.RotationData.ActorRotation);
	OutAnimData.LocomotionData.SmoothedMovementAngle = FMath::FixedTurn(OutAnimData.LocomotionData.SmoothedMovementAngle, OutAnimData.LocomotionData.MovementAngle, DeltaSeconds * 12.f);
}

void USHFAnimInstance::GatherCMCStates(FCMCStates& OutCMCStates)
{
	OutCMCStates.bIsFalling = MovementCompRef->IsFalling();
	OutCMCStates.bIsFlying = MovementCompRef->IsFlying();
	OutCMCStates.bIsCrouching = MovementCompRef->IsCrouching();
	OutCMCStates.bIsStrafing = !MovementCompRef->bOrientRotationToMovement;
	OutCMCStates.bMovingOnGround = MovementCompRef->IsMovingOnGround();
	/*NewData.CMCStates.bTransitionFallToJump = NewData.CMCStates.bIsFalling && NewData.VelocityData.Velocity.Z > 100.0;
	NewData.CMCStates.bShouldMove = NewData.VelocityData.GroundSpeed > 3.f && MovementCompRef->GetCurrentAcceleration().Size() > 0.f;*/
}

void USHFAnimInstance::CalculateMovementDirection(float DeltaSeconds, FSHFSharedAnimData& OutData)
{
	OutData.LocomotionData.MovementAngle = UKismetAnimationLibrary::CalculateDirection(OutData.VelocityData.Velocity, OutData.RotationData.ActorRotation);
	const ESHFMovementDirection NewDir = UAnimFunctionLibrary::CalculateCardinalDirection(OutData.LocomotionData.MovementAngle, LastMovementDirection, 15.f, -50.f, 50.f, -130.f, 130.f);
	OutData.LocomotionData.MovementDirection = NewDir;
	LastMovementDirection = NewDir;
	
	OutData.LocomotionData.AccelerationAngle = UKismetAnimationLibrary::CalculateDirection(OutData.AccelerationData.Acceleration, OutData.RotationData.ActorRotation);
	const ESHFMovementDirection AccelDir = UAnimFunctionLibrary::CalculateCardinalDirection(OutData.LocomotionData.AccelerationAngle, LastAccelerationDirection, 15.f, -50.f, 50.f, -130.f, 130.f);
	OutData.LocomotionData.AccelerationDirection = AccelDir;
	LastAccelerationDirection = AccelDir;
}



/**
 * Gets the addinional references, needs a valid OwningPawn!
 * @return true, if all addinional references are valid
 */
bool USHFAnimInstance::GetReferences()
{
	if (IsValid(CharacterRef) && IsValid(MovementCompRef)) 
		return true;
	
	checkf(IsValid(OwningPawn), TEXT("GetReferences() called on a non-valid Pawn"));
	CharacterRef = Cast<ACharacter>(OwningPawn);
	if (IsValid(CharacterRef))
		MovementCompRef = CharacterRef->GetCharacterMovement();
	return IsValid(CharacterRef) && IsValid(MovementCompRef);
}

void USHFAnimInstance::UpdateJumpFallData(FSHFSharedAnimData& OutData, const FSHFSharedAnimData& OldData, float DeltaSeconds)
{
	if (OutData.CMCStates.bIsFalling)
	{
		OutData.JumpData.TimeToJumpApex = UAnimFunctionLibrary::PredictTimeToJumpApex(OutData.VelocityData.Velocity, MovementCompRef);
		MaxFallSpeed = FMath::Min(MaxFallSpeed, OutData.VelocityData.Velocity.Z);
	} else if (OldData.CMCStates.bIsFalling)
	{
		OutData.JumpData.LandingImpactAlpha = FMath::GetMappedRangeValueClamped(FVector2D(-200.f, -1000.f), FVector2D(0.f, 1.f), MaxFallSpeed);
		MaxFallSpeed = 0.f;
	} else
	{
		OutData.JumpData.LandingImpactAlpha = FMath::FInterpTo(OldData.JumpData.LandingImpactAlpha, 0.0f, DeltaSeconds, 4.0f);
	}
}


void USHFAnimInstance::TraceGroundDistance(FSHFSharedAnimData& OutData) const
{
	if (!OutData.CMCStates.bIsFalling || OutData.VelocityData.Velocity.Z > 50.f) return; //TODO - Maybe only if VelZ > 0 ???
	
	FHitResult OutHit;
	FVector StartLocation = OutData.LocationData.WorldLocation;
	
	if (UCapsuleComponent* Capsule = CharacterRef->GetCapsuleComponent())
	{
		const float CapsuleHeightWithOffset = Capsule->GetScaledCapsuleHalfHeight() + 20.f;
		StartLocation += (-CapsuleHeightWithOffset) * CharacterRef->GetActorUpVector();
	}
	FVector EndLocation = StartLocation + (CharacterRef->GetActorUpVector() * FVector(-1000.f));
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwningActor());
	
	if (GetWorld()->LineTraceSingleByChannel(OutHit, StartLocation, EndLocation, ECC_Visibility, QueryParams))
		OutData.JumpData.GroundDistance = OutHit.Distance;
	
	//DrawDebugLine(GetWorld(), StartLocation, EndLocation, bResult ? FColor::Red : FColor::Green, true, 0.2f);
}

void USHFAnimInstance::CalculateLeaningData(FSHFSharedAnimData& OutData, const FSHFSharedAnimData& OldData,
	float DeltaSeconds) const
{
	float TargetLeanAngle = OutData.RotationData.DeltaActorYaw * 7.0f;
	if (OutData.LocomotionData.MovementDirection == ESHFMovementDirection::Backward)
		TargetLeanAngle *= -1.f;
	
	/*float TargetLeanAngle = FMath::ClampAngle(OutData.RotationData.DeltaActorYaw / DeltaSeconds / 5.f, -90.f, 90.f) * (
		OutData.LocomotionData.MovementDirection == ESHFMovementDirection::Backward ? -1.f : 1.f);*/
	
	float GaitScale = 0.f;
	switch (OutData.Gait)
	{
		case ESHFGait::Walk: GaitScale = 0.2f; break;
		case ESHFGait::Run: GaitScale = 1.0f; break;
		case ESHFGait::Crouch: GaitScale = 0.2f; break;
	}
	OutData.LeaningData.BlendSpaceGait = GaitScale;
	
	// Interpolation (Glättung)
	OutData.LeaningData.Angle = FMath::FInterpTo(OldData.LeaningData.Angle, TargetLeanAngle, DeltaSeconds, 6.0f);		
	
	// --- Luft-Leaning (Air Lean) ---
	FRotator TargetRotation;
	if (OutData.CMCStates.bIsFalling)
	{
		FVector LocalVel = OwningPawn->GetActorRotation().UnrotateVector(OutData.VelocityData.Velocity);
		const float MaxAngle = AnimComp->MaxAirLeanAngle;

		const float TargetRoll = (LocalVel.Y / MovementCompRef->MaxWalkSpeed) * MaxAngle;
		const float TargetPitch = (OutData.VelocityData.Velocity.Z / 1000.f) * -MaxAngle;

		TargetRotation = FRotator(TargetPitch, 0.f, TargetRoll);
	}
	else 
	{
		// Am Boden nutzen wir nur den Yaw-Lean (als Roll-Rotation)
		TargetRotation = FRotator(0.f, 0.f, OutData.LeaningData.Angle);
	}	
	// Finale Rotation glätten
	OutData.LeaningData.Rotation = FMath::RInterpTo(OldData.LeaningData.Rotation, TargetRotation, DeltaSeconds, 5.0f);
}


FTurnInPlaceAnimSet USHFAnimInstance::GetTurnAnimSet() const
{
	return TurnData.AnimSet;
}

UAnimSequence* USHFAnimInstance::GetTurnAnimation(bool bRecovery)
{
	const FTurnInPlaceAnimSet TurnAnimSet = GetTurnAnimSet();
	return UTurnInPlaceStatics::GetTurnInPlaceAnimation(TurnAnimSet, TurnInPlaceGraphNodeData, bRecovery);
}
