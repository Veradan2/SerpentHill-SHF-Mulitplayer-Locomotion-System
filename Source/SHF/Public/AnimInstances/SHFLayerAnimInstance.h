// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define IDLE_BREAK_UPDATE_PERIOD 3.0

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Animation/AnimInstance.h"
#include "SHFLayerAnimInstance.generated.h"

struct FAnimNodeReference;
struct FAnimUpdateContext;
struct FSHFSharedAnimData;
/**
 * The parent layer anim instance with the visual logic
 */
UCLASS()
class SHF_API USHFLayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	void UpdateFromMain(const FSHFSharedAnimData& NewData);
	
protected:
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnInitialUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	
		// Local copy of the data exchange struct (thread safe access possible)
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Data")
	FSHFSharedAnimData SharedData;	
	
	// Eine Funktion, die wir im AnimBP-Eventgraph nutzen können
	UFUNCTION(BlueprintImplementableEvent, Category = "SHF|Events")
	void OnDataUpdated();
	
	/*
	 * Idle and Breaks
	 */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|Idle")
	TObjectPtr<UAnimSequence> IdleAnim;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF||Idle")
	TArray<TObjectPtr<UAnimSequence>> IdleBreaks;
	
	UPROPERTY(BlueprintReadWrite)
	int32 IdleIndex = 0;
	
	// Eine Map, die pro Gangart (Walk, Run) ein Set von 4 Animationen speichert
	UPROPERTY(EditDefaultsOnly, Category = "SHF|Locomotion")
	TMap<ESHFGait, FCardinalAnimationSet> MovementAnims;

	// Die aktuell gewählte Animation (wird in C++ berechnet)
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	TObjectPtr<UAnimSequence> ActiveMovementAnim;
	
private:
	double IdleActiveTimeStamp = 0.;
	
	void CalculateIdleIndex();
	
	UPROPERTY()
	TObjectPtr<AGameStateBase> CachedGameState = nullptr;
	
};
