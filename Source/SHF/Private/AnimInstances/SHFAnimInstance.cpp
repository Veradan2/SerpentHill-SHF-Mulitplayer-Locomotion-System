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
#include "Dataflow/DataflowContent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Libraries/AnimFunctionLibrary.h"
#include "SHF/Components/AnimComponent.h"

void FSHFAnimInstanceProxy::SetRootYawOffset(float NewYawOffset)
{
	SharedData.RootYawOffset = NewYawOffset;
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

	
	// 1. Paket schnüren
	FSHFSharedAnimData NewData;
	NewData.CharacterVelocity = OwningPawn->GetVelocity();
	NewData.GroundSpeed = NewData.CharacterVelocity.Size2D();
	NewData.bIsMoving = NewData.GroundSpeed > 10.f;
	NewData.Gait = AnimComp->GetCurrentMovementGait();
	NewData.EquipMode = AnimComp->GetCurrentEquipMode();
	
	NewData.CharacterAcceleration = MovementCompRef->GetCurrentAcceleration();
	NewData.bIsAccelerating = NewData.CharacterAcceleration.Size() > 0.f;
	
	NewData.LocomotionAngle = UKismetAnimationLibrary::CalculateDirection(NewData.CharacterVelocity, OwningPawn->GetActorRotation());
	NewData.OrientationWarpingLocomotionAngle = -NewData.LocomotionAngle;
	NewData.StrideWarpingLocomotionSpeed = NewData.GroundSpeed;
	NewData.StrideWarpingAlpha = FMath::Clamp(NewData.GroundSpeed / 50.f, 0.f, 1.f);
	
	NewData.LastFrameActorRotation = Proxy.SharedData.ActorRotation;
	NewData.ActorRotation = OwningPawn->GetActorRotation();
	
	NewData.CMCStates.bIsCrouching = MovementCompRef->IsCrouching();
	NewData.CMCStates.bIsFalling = MovementCompRef->IsFalling();
	NewData.CMCStates.bIsStrafing = !MovementCompRef->bOrientRotationToMovement;
	NewData.CMCStates.bTransitionFallToJump = NewData.CMCStates.bIsFalling && NewData.CharacterVelocity.Z > 100.0;
	NewData.CMCStates.bShouldMove = NewData.GroundSpeed > 3.f && MovementCompRef->GetCurrentAcceleration().Size() > 0.f;
	
	NewData.MovementConfig.MaxWalkSpeed = MovementCompRef->MaxWalkSpeed;
	NewData.MovementConfig.MaxAcceleration = MovementCompRef->MaxAcceleration;
	NewData.MovementConfig.GroundFriction = MovementCompRef->GroundFriction;
	NewData.MovementConfig.BrakingFriction = MovementCompRef->BrakingFriction;
	NewData.MovementConfig.BrakingDecelerationWalking = MovementCompRef->BrakingDecelerationWalking;
	NewData.MovementConfig.BrakingFrictionFactor = MovementCompRef->BrakingFrictionFactor;
	NewData.MovementConfig.bUseSeparateBrakingFriction = MovementCompRef->bUseSeparateBrakingFriction;
	
	
	NewData.MovementState.bGateChanged = CurrentMovementGait != PreviousMovementGait;
	NewData.MovementState.bEquipModeChanged = CurrentEquipMode != PreviousEquipMode;
	
	
	const float DeltaActorYaw = UKismetMathLibrary::NormalizeAxis(Proxy.SharedData.ActorRotation.Yaw - NewData.ActorRotation.Yaw);
	NewData.RootYawOffset = Proxy.SharedData.RootYawOffset + DeltaActorYaw;		
	
	CalculateMovementDirection(DeltaSeconds, NewData);
	
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
			Proxy.SharedData.bStartFinished = true;
			break;
		}
	}
	
	SharedData = Proxy.SharedData;
	
	TurnInPlaceCurveValues = UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceCurveValues(this, TurnData);
	
	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlace(TurnData, bCanUpdateTurnInPlace, Proxy.SharedData.CMCStates.bIsStrafing, TurnOutput);
	
	TransitionRules.bCycle2Idle = !Proxy.SharedData.bIsMoving;
	TransitionRules.bIdle2Start = Proxy.SharedData.bIsAccelerating;
	TransitionRules.bStart2Cycle = FMath::IsNearlyEqual(Proxy.SharedData.GroundSpeed, Proxy.SharedData.MovementConfig.MaxWalkSpeed, 5.f);
	TransitionRules.bStart2CycleCond2 = Proxy.SharedData.MovementState.bGateChanged || Proxy.SharedData.MovementState.bEquipModeChanged;
	
	bIdleToMovement = Proxy.SharedData.bIsMoving;
	bMovementToIdle = !Proxy.SharedData.bIsMoving;
}

void USHFAnimInstance::RegisterLayer(USHFLayerAnimInstance* Layer)
{
	// Wir räumen erst auf: Alle Layer entfernen, die nicht mehr "Linked" sind
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

void USHFAnimInstance::CalculateMovementDirection(float DeltaSeconds, FSHFSharedAnimData& OutData)
{
	OutData.LocomotionAngle = UKismetAnimationLibrary::CalculateDirection(OutData.CharacterVelocity, OutData.ActorRotation);
	const ESHFMovementDirection NewDir = UAnimFunctionLibrary::CalculateCardinalDirection(OutData.LocomotionAngle, LastMovementDirection);
	OutData.MovementDirection = NewDir;
	LastMovementDirection = NewDir;
	
	OutData.AccelerationAngle = UKismetAnimationLibrary::CalculateDirection(OutData.CharacterAcceleration, OutData.ActorRotation);
	const ESHFMovementDirection AccelDir = UAnimFunctionLibrary::CalculateCardinalDirection(OutData.AccelerationAngle, LastAccelerationDirection);
	OutData.AccelerationDirection = AccelDir;
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

FTurnInPlaceAnimSet USHFAnimInstance::GetTurnAnimSet() const
{
	return TurnData.AnimSet;
}

UAnimSequence* USHFAnimInstance::GetTurnAnimation(bool bRecovery)
{
	const FTurnInPlaceAnimSet TurnAnimSet = GetTurnAnimSet();
	return UTurnInPlaceStatics::GetTurnInPlaceAnimation(TurnAnimSet, TurnInPlaceGraphNodeData, bRecovery);
}
