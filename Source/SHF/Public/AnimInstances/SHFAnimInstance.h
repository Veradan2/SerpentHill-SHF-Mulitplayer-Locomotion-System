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
	FSHFSharedAnimData SharedData;
	
	void SetRootYawOffset(float NewYawOffset);

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
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	void RegisterLayer(USHFLayerAnimInstance* Layer);
	FORCEINLINE void UnregisterLayer(USHFLayerAnimInstance* OldLayer) { LinkedLayers.Remove(OldLayer); }
	
	void OnUpdateSimulatedProxiesMovement();
	
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<UAnimComponent> AnimComp;	
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<USHFLayerAnimInstance>> LinkedLayers;
	
	// Speichert die letzte Richtung für die Hysterese
	ESHFMovementDirection LastMovementDirection = ESHFMovementDirection::Forward;
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIdleToMovement = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMovementToIdle = false;
	
	UPROPERTY(BlueprintReadOnly)
	FSHFSharedAnimData SharedData;

private:
	void CalculateMovementDirection(float DeltaSeconds, FSHFSharedAnimData& OutData);
	
};
