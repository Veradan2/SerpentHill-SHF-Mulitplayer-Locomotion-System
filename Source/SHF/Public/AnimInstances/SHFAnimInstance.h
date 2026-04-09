// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "TurnInPlaceAnimInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Interfaces/AnimInstanceInterface.h"
#include "SHFAnimInstance.generated.h"


struct FAnimNodeReference;
struct FAnimUpdateContext;
class UTurnInPlace;
class UCharacterMovementComponent;
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

protected:
	// Diese Funktion wird vom Anim-Thread aufgerufen
	virtual void Update(float DeltaSeconds) override;
};



/**
 * 
 */
UCLASS()
class SHF_API USHFAnimInstance : public UAnimInstance, public ITurnInPlaceAnimInterface, public IAnimInstanceInterface
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	void RegisterLayer(USHFLayerAnimInstance* Layer);
	FORCEINLINE void UnregisterLayer(USHFLayerAnimInstance* OldLayer) { LinkedLayers.Remove(OldLayer); }
	
	void OnUpdateSimulatedProxiesMovement();
	
	virtual FTurnInPlaceCurveValues GetTurnInPlaceCurveValues_Implementation() const override;
	virtual FTurnInPlaceAnimSet GetTurnInPlaceAnimSet_Implementation() const override;
	
	virtual void ReceiveNewEquipMode(ESHFEquipMode NewEquipMode) override;
	virtual void ReceiveNewGait(ESHFGait NewMovementGait) override;
	
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<APawn> OwningPawn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<UAnimComponent> AnimComp;
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<ACharacter> CharacterRef;
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<UCharacterMovementComponent> MovementCompRef;
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	TObjectPtr<UTurnInPlace> TurnInPlaceComp;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<USHFLayerAnimInstance>> LinkedLayers;
	
	// Speichert die letzte Richtung für die Hysterese
	ESHFMovementDirection LastMovementDirection = ESHFMovementDirection::Forward;
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIdleToMovement = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bMovementToIdle = false;
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "SHF|Core")
	FTransitionRuleContainer TransitionRules;
	
	UPROPERTY(BlueprintReadOnly)
	FSHFSharedAnimData SharedData;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void SetupIdle(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void SetupTurnInPlace(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void UpdateTurnInPlace(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void SetupTurnAnim(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void UpdateTurnInPlaceRecovery(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe), Category = "SHF|Core")
	void SetupTurnRecovery(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	

private:
	void CalculateMovementDirection(float DeltaSeconds, FSHFSharedAnimData& OutData);
	
	bool GetReferences();
	
protected:
	UPROPERTY(BlueprintReadOnly);
	FTurnInPlaceAnimGraphData TurnData = FTurnInPlaceAnimGraphData();
	
	UPROPERTY(BlueprintReadOnly)
	FTurnInPlaceAnimGraphOutput TurnOutput = FTurnInPlaceAnimGraphOutput();
	
	UPROPERTY(BlueprintReadOnly)
	bool bCanUpdateTurnInPlace = false;
	
	UPROPERTY(BlueprintReadOnly)
	FTurnInPlaceCurveValues TurnInPlaceCurveValues;
	
	UPROPERTY(BlueprintReadOnly)
	FTurnInPlaceGraphNodeData TurnInPlaceGraphNodeData;
	
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	FTurnInPlaceAnimSet GetTurnAnimSet() const;
	
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	UAnimSequence* GetTurnAnimation(bool bRecovery);
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|TIP");
	FTurnInPlaceAnimSet TIPAnimSet;
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|TIP");
	FTurnInPlaceAnimSet TIPAnimSetCrouched;
	
	ESHFGait CurrentMovementGait = ESHFGait::Run;
	ESHFEquipMode CurrentEquipMode = ESHFEquipMode::Mode1;
};
