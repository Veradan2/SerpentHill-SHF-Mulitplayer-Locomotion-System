// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"

#include "SequencePlayerLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFAnimInstance.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/GameStateBase.h"

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

void USHFLayerAnimInstance::UpdateFromMain(const FSHFSharedAnimData& NewData)
{

	SharedData = NewData;

	if (SharedData.bIsMoving)
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds(); //Reset
	
	OnDataUpdated();
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
