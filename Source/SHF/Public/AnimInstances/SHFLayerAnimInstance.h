// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define IDLE_BREAK_UPDATE_PERIOD 3.0

#include "CoreMinimal.h"
#include "SHFAnimInstance.h"
#include "SHFGlobals.h"
#include "TurnInPlaceAnimInterface.h"
#include "TurnInPlaceTypes.h"
#include "Animation/AnimInstance.h"
#include "Data/AnimSetAsset.h"
#include "SHFLayerAnimInstance.generated.h"


class UAnimSetAsset;
struct FAnimNodeReference;
struct FAnimUpdateContext;
struct FSHFSharedAnimData;


/**
 * The parent layer anim instance with the visual logic
 */
UCLASS()
class SHF_API USHFLayerAnimInstance : public UAnimInstance, public ITurnInPlaceAnimInterface
{
	GENERATED_BODY()
	
public:
	/**
	 **
	 **/
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Core", meta = (BlueprintThreadSafe))
	bool bStartFinished = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Core", meta = (BlueprintThreadSafe))
	float Stop_DistanceRemaining = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Core", meta = (BlueprintThreadSafe))
	float RelevantAnimTimeRemaining = 1.f;

	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	virtual FTurnInPlaceAnimSet GetTurnInPlaceAnimSet_Implementation() const override;
	
	void UpdateFromMain(const FSHFSharedAnimData& NewData);
	
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	
		
	/*
	 *			Idle Sequence Player
	 */
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnInitialUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Idle_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	/*
	 *			Movement(Cycle) Sequence Player
	 */
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Movement_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Movement_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	/*
	 *			Start Sequence Evaluator
	 */
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Start_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Start_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	/*
	 *			Stop Sequence Evaluator
	 */
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Stop_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|AnimNodeFunctions", meta = (BlueprintThreadSafe))
	void Stop_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	
	/*
	 *			Jump Land Sequence Evaluator
	 */
	
	
	// Local copy of the data exchange struct (thread safe access possible)
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Data")
	FSHFSharedAnimData SharedData;	
	
	// Eine Funktion, die wir im AnimBP-Eventgraph nutzen können
	UFUNCTION(BlueprintImplementableEvent, Category = "SHF|Events")
	void OnDataUpdated();
	
	/*
	 * All anims which the current layer contains
	 */
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF|AnimSet")
	TObjectPtr<UAnimSetAsset> MovementAnimSet;
	
	
	/*
	 * Cached Anims (thread-safe!)
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Idle")
	TMap<ESHFGait, TObjectPtr<UAnimSequence>> IdleAnims_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Idle")
	TMap<ESHFGait, FAnimSequenceArray> IdleBreaks_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|TIP")
	FTurnInPlaceAnimSet TurnInPlaceAnimSet_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|TIP")
	FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Locomotion")
	TMap<ESHFGait, FCardinalAnimationSet> MovementAnims_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Start")
	TMap<ESHFGait, FCardinalAnimationSet> StartAnims_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Stop")
	TMap<ESHFGait, FCardinalAnimationSet> StopAnims_Cached;
	
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Jump", meta = (BlueprintThreadSafe))	
	FJumpAnimSet JumpAnimSet_Cached;
	
	UPROPERTY(BlueprintReadWrite)
	int32 IdleIndex = 0;
	
	
private:
	void Stop_UpdateOrientationAngle(float DeltaSeconds);
	
	
	double IdleActiveTimeStamp = 0.;
	
	void CalculateIdleIndex();
	float CalculateLandingTime(float Distance) const;
	
	UPROPERTY()
	TObjectPtr<AGameStateBase> CachedGameState = nullptr;
	
	void SetAnimSet(ESHFEquipMode EquipMode, bool bEnforce);
	
	ESHFEquipMode CurrentEquipMode;
	bool bFirstInit = true;
	
	float DistanceTraveled;
	FVector StartLocation;
	float Start_MaxDistance = 0.f;
	float StartAnimDeltaTime = 0.f;
	
	FVector StopLocation;
	float Stop_MaxDistance = 0.f;
	float DistanceToStop = 0.f;
	
	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter = nullptr;
	
	ESHFMovementDirection LastAccelerationDirection = ESHFMovementDirection::Forward;
	
	ESHFMovementDirection DetermineAccelerationDirection(const FSHFAnimInstanceProxy& Proxy);
	ESHFMovementDirection DetermineMovementDirection(const FSHFAnimInstanceProxy& Proxy);
};
