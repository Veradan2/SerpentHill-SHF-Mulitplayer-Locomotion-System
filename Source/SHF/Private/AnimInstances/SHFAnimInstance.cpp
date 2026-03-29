// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "AnimInstances/SHFLayerAnimInstance.h"
#include "SHF/Components/AnimComponent.h"

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
        
		if (!AnimComp)
		{
			// Optional: Logge eine Warnung, falls die Komponente fehlt
			UE_LOG(LogTemp, Warning, TEXT("SHFAnimInstance: Cannot find AnimComponent (%s)"), *OwningPawn->GetName());
		}
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
	
	// 1. Paket schnüren
	FSHFSharedAnimData NewData;
	NewData.GroundSpeed = OwningPawn->GetVelocity().Size2D();
	NewData.bIsMoving = NewData.GroundSpeed > 10.f;
	NewData.TurnState = AnimComp->CurrentTurnState;
	NewData.RootYawOffset = AnimComp->RootYawOffset;
	
	CalculateMovementDirection(DeltaSeconds, NewData);
	
	FSHFSharedAnimData SharedData = NewData;
	//SharedData.TurnState = AnimComp->CurrentTurnState;
	//SharedData.RootYawOffset = AnimComp->RootYawOffset;

	// 2. Daten an alle Layer "pushen"
	for (USHFLayerAnimInstance* Layer : LinkedLayers) {
		if (IsValid(Layer)) {
			Layer->UpdateFromMain(SharedData);
		}
	}	
}

void USHFAnimInstance::RegisterLayer(USHFLayerAnimInstance* Layer)
{
	// Wir räumen erst auf: Alle Layer entfernen, die nicht mehr "Linked" sind
	LinkedLayers.RemoveAll([](TObjectPtr<USHFLayerAnimInstance> L) {
		return L == nullptr || !L->GetSkelMeshComponent()->GetLinkedAnimLayerInstanceByClass(L->GetClass());
	});

	LinkedLayers.AddUnique(Layer);	
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
