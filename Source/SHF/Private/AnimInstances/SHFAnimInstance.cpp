// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "AnimInstances/SHFLayerAnimInstance.h"
#include "Character/SHFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
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
		} else 	UE_LOG(LogTemp, Warning, TEXT("SHFAnimInstance: Cannot find AnimComponent (%s)"), *OwningPawn->GetName());
	}
}

void USHFAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!OwningPawn || !AnimComp)
	{
		NativeInitializeAnimation();
		if (!OwningPawn || !AnimComp) return;
	}
	
	FSHFAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFAnimInstanceProxy>();
	
	// 1. Paket schnüren
	FSHFSharedAnimData NewData;
	NewData.GroundSpeed = OwningPawn->GetVelocity().Size2D();
	NewData.bIsMoving = NewData.GroundSpeed > 10.f;
	NewData.TurnState = AnimComp->CurrentTurnState;
	NewData.Gait = ESHFGait::Run;
	
	NewData.LocomotionAngle = UKismetAnimationLibrary::CalculateDirection(OwningPawn->GetVelocity(), OwningPawn->GetActorRotation());
	NewData.OrientationWarpingLocomotionAngle = -NewData.LocomotionAngle;
	NewData.StrideWarpingLocomotionSpeed = NewData.GroundSpeed;
	NewData.StrideWarpingAlpha = FMath::Clamp(NewData.GroundSpeed / 50.f, 0.f, 1.f);
	
	NewData.LastFrameActorRotation = Proxy.SharedData.ActorRotation;
	NewData.ActorRotation = OwningPawn->GetActorRotation();
	
	if (OwningPawn->GetLocalRole() != ROLE_SimulatedProxy && !NewData.bIsMoving)
	{
		const float DeltaActorYaw = UKismetMathLibrary::NormalizeAxis(Proxy.SharedData.ActorRotation.Yaw - NewData.ActorRotation.Yaw);
		NewData.RootYawOffset = Proxy.SharedData.RootYawOffset + DeltaActorYaw;		
	} else NewData.RootYawOffset = 0;
	
	CalculateMovementDirection(DeltaSeconds, NewData);
	
	/* Daten in den Proxy schreiben */

	Proxy.SharedData = NewData;
	
	// 2. Daten an alle Layer "pushen"
	for (USHFLayerAnimInstance* Layer : LinkedLayers) {
		if (IsValid(Layer)) {
			Layer->UpdateFromMain(NewData);
		}
	}	
	
}

void USHFAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	
	SharedData = Proxy.SharedData;
	
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

void USHFAnimInstance::OnUpdateSimulatedProxiesMovement()
{
	/*FSHFAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFAnimInstanceProxy>();
	
	FSHFSharedAnimData LocalProxyData = Proxy.SharedData;
	
	FRotator ProxyRotation = LocalProxyData.ActorRotation;
	FRotator LastFrameProxyRotation = LocalProxyData.LastFrameActorRotation;
	FRotator DeltaActorYaw = ProxyRotation - LastFrameProxyRotation;
	
	
	
	Proxy.SetRootYawOffset(-UKismetMathLibrary::NormalizeAxis(LocalProxyData.RootYawOffset - DeltaActorYaw.Yaw));
	GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Red, FString::Printf(TEXT("Root Yaw Offset: %f"), Proxy.SharedData.RootYawOffset));*/
}

FAnimInstanceProxy* USHFAnimInstance::CreateAnimInstanceProxy()
{
	return new FSHFAnimInstanceProxy(this);
}

void USHFAnimInstance::CalculateMovementDirection(float DeltaSeconds, FSHFSharedAnimData& OutData)
{
	FVector Velocity = OwningPawn->GetVelocity();
	FRotator Rotation = OwningPawn->GetActorRotation();

	// 1. Roh-Winkel berechnen (-180 bis 180)
	float RawAngle = UKismetAnimationLibrary::CalculateDirection(Velocity, Rotation);
	OutData.LocomotionAngle = RawAngle;

	// 2. Richtungs-Logik mit Hysterese (Buffer)
	// Wir definieren Schwellenwerte, die sich je nach letzter Richtung verschieben
	float Buffer = 10.0f; 
	ESHFMovementDirection NewDir = LastMovementDirection;

	// Vorwärts Check (Standardbereich -45 bis 45)
	if (LastMovementDirection == ESHFMovementDirection::Forward) {
		if (RawAngle > (45.f + Buffer)) NewDir = ESHFMovementDirection::Right;
		else if (RawAngle < (-45.f - Buffer)) NewDir = ESHFMovementDirection::Left;
	}
	// Rechts Check
	else if (LastMovementDirection == ESHFMovementDirection::Right) {
		if (RawAngle < (45.f - Buffer)) NewDir = ESHFMovementDirection::Forward;
		else if (RawAngle > (135.f + Buffer)) NewDir = ESHFMovementDirection::Backward;
	}
	// Links Check
	else if (LastMovementDirection == ESHFMovementDirection::Left) {
		if (RawAngle > (-45.f + Buffer)) NewDir = ESHFMovementDirection::Forward;
		else if (RawAngle < (-135.f - Buffer)) NewDir = ESHFMovementDirection::Backward;
	}
	// Rückwärts Check
	else if (LastMovementDirection == ESHFMovementDirection::Backward) {
		if (RawAngle > 0.f && RawAngle < (135.f - Buffer)) NewDir = ESHFMovementDirection::Right;
		else if (RawAngle < 0.f && RawAngle > (-135.f + Buffer)) NewDir = ESHFMovementDirection::Left;
	}

	OutData.MovementDirection = NewDir;
	LastMovementDirection = NewDir; // State für nächsten Frame speichern
}
