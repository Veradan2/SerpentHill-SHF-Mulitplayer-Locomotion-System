// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFAnimInstance.h"

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
	
	
	FSHFSharedAnimData SharedData;
	SharedData.GroundSpeed = OwningPawn->GetVelocity().Size2D();
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
