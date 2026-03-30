// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"

#include "SequencePlayerLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFAnimInstance.h"
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

void USHFLayerAnimInstance::UpdateFromMain(const FSHFSharedAnimData& NewData)
{
	FSHFLayerAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFLayerAnimInstanceProxy>();
	Proxy.SharedDataProxy = NewData;
	
	if (Proxy.SharedDataProxy.bIsMoving)
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds(); //Reset
	
	OnDataUpdated();
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
		USequencePlayerLibrary::SetSequence(SequencePlayer, IdleAnim);
		USequencePlayerLibrary::SetAccumulatedTime(SequencePlayer, 0.0f);
		// Nutze KEIN Blending beim allerersten Frame (Dauer 0.0f)
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnim, 0.0f);		
	}
}

void USHFLayerAnimInstance::Idle_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;

	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnim, .2f);
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds();
	}
	
}

void USHFLayerAnimInstance::Idle_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (FMath::TruncToInt64(GetWorld()->GetTimeSeconds() - IdleActiveTimeStamp) > 5)
	{
		EAnimNodeReferenceConversionResult Result;
		const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
		if (Result == EAnimNodeReferenceConversionResult::Succeeded)
		{
			UAnimSequence* AnimSeq = IdleBreaks[IdleIndex];
			if (AnimSeq != USequencePlayerLibrary::GetSequencePure(SequencePlayer))
				USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, AnimSeq, .2f);
			IdleActiveTimeStamp = GetWorld()->GetTimeSeconds();
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
		if (FCardinalAnimationSet* Set = MovementAnims.Find(SharedData.Gait))
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

void USHFLayerAnimInstance::CalculateIdleIndex()
{
	const UWorld* World = GetWorld();
	
	if (IdleBreaks.Num() > 0)
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
				
				int32 UniqueSeed = 0;
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
    
				IdleIndex = DeterministicStream.RandRange(0, IdleBreaks.Num() - 1);
				}
		}
	}	
}
