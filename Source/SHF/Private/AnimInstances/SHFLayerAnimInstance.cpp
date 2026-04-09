// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"


#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
//#include "DistanceMatching/AnimDistanceMatchingLibrary.h"
#include "AnimDistanceMatchingLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFAnimInstance.h"
#include "Data/AnimSetAsset.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/GameStateBase.h"

/*
 **		------	LAYER ANIM INSTANCE ------
 */	

void USHFLayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (USHFAnimInstance* MainInst = Cast<USHFAnimInstance>(GetSkelMeshComponent()->GetAnimInstance()))
	{
		MainInst->RegisterLayer(this);
	}
}

void USHFLayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!IsValid(CachedGameState))
	{
		CachedGameState = GetWorld()->GetGameState();
	}
	
	CalculateIdleIndex();
}

void USHFLayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	// Worker-Thread holt sich die Daten ab
	const FSHFLayerAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFLayerAnimInstanceProxy>();
    
	// Hier spiegelst du die Daten in die lokale Variable für das AnimBP
	SharedData = Proxy.SharedDataProxy;	
}

FTurnInPlaceAnimSet USHFLayerAnimInstance::GetTurnInPlaceAnimSet_Implementation() const
{
	return SharedData.CMCStates.bIsCrouching ? TurnInPlaceAnimSetCrouched_Cached : TurnInPlaceAnimSet_Cached;
}


void USHFLayerAnimInstance::UpdateFromMain(const FSHFSharedAnimData& NewData)
{
	FSHFLayerAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFLayerAnimInstanceProxy>();
	Proxy.SharedDataProxy = NewData;
	
	if (Proxy.SharedDataProxy.bIsMoving)
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds(); //Reset
	
	OnDataUpdated();
	
	SetAnimSet(NewData.EquipMode, false);
	CurrentEquipMode = NewData.EquipMode;
	
	SetAnimSet(CurrentEquipMode, false);	//According to the docs, this is the recommed way...
}

FAnimInstanceProxy* USHFLayerAnimInstance::CreateAnimInstanceProxy()
{
	return new FSHFLayerAnimInstanceProxy(this);
}

void USHFLayerAnimInstance::Idle_OnInitialUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;

	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequencePlayerLibrary::SetSequence(SequencePlayer, IdleAnim_Cached);
		USequencePlayerLibrary::SetAccumulatedTime(SequencePlayer, 0.0f);
		// Nutze KEIN Blending beim allerersten Frame (Dauer 0.0f)
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnim_Cached, 0.f);		
	}
}

void USHFLayerAnimInstance::Idle_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;

	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnim_Cached, .2f);
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds();
	}
}

void USHFLayerAnimInstance::Idle_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	
	if (Result != EAnimNodeReferenceConversionResult::Succeeded) return;
	
	UAnimSequenceBase* CurrentPlaying = USequencePlayerLibrary::GetSequencePure(SequencePlayer);
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	bool bIsPlayingAnyBreak = IdleBreaks_Cached.Contains(CurrentPlaying);
	
	if (CurrentPlaying != IdleAnim_Cached && !bIsPlayingAnyBreak)
	{
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnim_Cached, 0.2f);
		IdleActiveTimeStamp = CurrentTime;
		return;
	}
	
	if (CurrentTime - IdleActiveTimeStamp > 5.f)
	{
		if (IdleBreaks_Cached.IsValidIndex(IdleIndex))
		{
			UAnimSequence* NextBreak = IdleBreaks_Cached[IdleIndex];
			USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, NextBreak, .2f);
			
			IdleIndex = (IdleIndex + 1) % IdleBreaks_Cached.Num();
			IdleActiveTimeStamp = CurrentTime;
		}
	}
}

void USHFLayerAnimInstance::Movement_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	FSequencePlayerReference Player = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
    
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		// 1. Ziel-Animation basierend auf der Richtung bestimmen
		UAnimSequence* TargetAnim = nullptr;
		if (FCardinalAnimationSet* Set = MovementAnims_Cached.Find(SharedData.Gait))
		{
			TargetAnim = Set->SelectAnim(SharedData.MovementDirection);
		}

		if (TargetAnim)
		{
			// 2. WICHTIGSTER CHECK: Was spielt gerade?
			UAnimSequenceBase* CurrentAnim = USequencePlayerLibrary::GetSequencePure(Player);
            
			// NUR wenn die neue Animation eine ANDERE ist als die aktuelle,
			// rufen wir SetSequence auf. Das verhindert den Dauer-Reset!
			if (CurrentAnim != TargetAnim)
			{
				USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, Player, TargetAnim, 0.2f);
                
				// Debug-Log: Erscheint dieser Log nur beim Richtungswechsel? 
				// Wenn er durchgehend spamt, stimmt der Vergleich != nicht.
				UE_LOG(LogTemp, Log, TEXT("SHF: Richtung gewechselt zu %s"), *TargetAnim->GetName());
			}
		}

		// 3. PlayRate setzen (Darf jeden Frame passieren, da es die Zeit nicht zurücksetzt)
		// Lyra Walk ist ca. 160 units/s, Run ca. 380 units/s
		float RefSpeed = (SharedData.Gait == ESHFGait::Walk) ? 160.f : 480.f;
		
		float SafePlayRate = FMath::Max(0.1f, SharedData.GroundSpeed / RefSpeed);
		USequencePlayerLibrary::SetPlayRate(Player, SafePlayRate);
	}	
}

void USHFLayerAnimInstance::Start_OnInitialUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		if (FCardinalAnimationSet* Set = MovementAnims_Cached.Find(SharedData.Gait))
		{
			UAnimSequenceBase* AnimSeq = Set->SelectAnim(SharedData.MovementDirection);
			USequenceEvaluatorLibrary::SetSequenceWithInertialBlending(Context, SequenceEvaluator, AnimSeq, 0.2f);
		}
		
	}
}

void USHFLayerAnimInstance::Start_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	//Get Start location
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	
	StartLocation = Proxy.GetComponentTransform().GetLocation();
	DistanceTraveled = 0.f;
	
	//Set Start time to 0
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequenceEvaluatorLibrary::SetExplicitTime(SequenceEvaluator, 0.f);
	}
}

void USHFLayerAnimInstance::Start_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	
	FVector CurrentLocation = Proxy.GetComponentTransform().GetLocation();
	
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		DistanceTraveled = FVector::Distance(CurrentLocation, StartLocation);
		UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(
			Context, 
			SequenceEvaluator, 
			DistanceTraveled, 
			FName("Distance") 
		);
	}
	
}

void USHFLayerAnimInstance::CalculateIdleIndex()
{
	const UWorld* World = GetWorld();
	
	if (IdleBreaks_Cached.Num() > 0)
	{
		AActor* Owner = GetOwningActor();
		if (Owner && IsValid(World))
		{
			if (IsValid(World->GetNetDriver()))
			{
				
				// 1. Zeitquelle (Serverzeit bevorzugt, sonst Lokalzeit)
				double ServerTime = World->GetTimeSeconds();
				if (IsValid(CachedGameState))
				{
					ServerTime = CachedGameState->GetServerWorldTimeSeconds();
				}
				
				int32 UniqueSeed;
				if (World->GetNetDriver() && World->GetNetDriver()->GuidCache.IsValid())
				{
					UniqueSeed = World->GetNetDriver()->GuidCache->GetOrAssignNetGUID(Owner).ObjectId;
				}
				else
				{
					// FALLBACK: Wenn kein Netzwerk da ist, nutze die lokale UniqueID des Actors
					UniqueSeed = Owner->GetUniqueID();
				}				
				// 3. Zeitfenster definieren (z.B. alle 10 Sekunden ein möglicher Wechsel)
				
				int32 TimeSlot = FMath::FloorToInt(ServerTime / IDLE_BREAK_UPDATE_PERIOD);
		
    
				// 4. Deterministischer Zufall: Kombiniere Seed und TimeSlot
				// Dadurch würfelt jeder Charakter alle 10s neu, aber für sich individuell
				FRandomStream DeterministicStream(UniqueSeed + TimeSlot);
    
				IdleIndex = DeterministicStream.RandRange(0, IdleBreaks_Cached.Num() - 1);
				}
		}
	}	
}

void USHFLayerAnimInstance::SetAnimSet(ESHFEquipMode EquipMode, bool bEnforce)
{
	if (CurrentEquipMode != EquipMode || bFirstInit || bEnforce)
	{
		if (FMovementAnimSet* MovementAnims = MovementAnimSet->AnimSet.Find(EquipMode))
		{
			IdleAnim_Cached = MovementAnims->IdleAnim;
			IdleBreaks_Cached = MovementAnims->IdleBreakAnims;
			MovementAnims_Cached = MovementAnims->CycleAnims;
			StartAnims_Cached = MovementAnims->StartAnims;
			TurnInPlaceAnimSet_Cached = MovementAnims->TurnInPlaceAnimSet;
			TurnInPlaceAnimSetCrouched_Cached = MovementAnims->TurnInPlaceAnimSetCrouched;
		}
		bFirstInit = false;
	}
}
