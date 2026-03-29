// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "SHFAnimInstance.generated.h"


class USHFLayerAnimInstance;
class UAnimComponent;
/**
 *			P R O X Y 
 */
USTRUCT(BlueprintType)
struct FSHFAnimInstanceProxy : public FAnimInstanceProxy {
	GENERATED_BODY()

	FSHFAnimInstanceProxy() : FAnimInstanceProxy() {}
	FSHFAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	// Hier liegen die Kopien der Spieldaten
	UPROPERTY(BlueprintReadOnly)
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly)
	ESHFTurnState CurrentTurnState = ESHFTurnState::None;

	UPROPERTY(BlueprintReadOnly)
	float RootYawOffset = 0.f;

	// Diese Funktion wird vom Anim-Thread aufgerufen
	virtual void Update(float DeltaSeconds) override;
};



/**
 * 
 */
UCLASS()
class SHF_API USHFAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	void RegisterLayer(USHFLayerAnimInstance* Layer);
	FORCEINLINE void UnregisterLayer(USHFLayerAnimInstance* OldLayer) { LinkedLayers.Remove(OldLayer); }
	
protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<UAnimComponent> AnimComp;	
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<USHFLayerAnimInstance>> LinkedLayers;

	
};
