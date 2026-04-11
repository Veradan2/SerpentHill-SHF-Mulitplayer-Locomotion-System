// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"


#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
#include "AnimDistanceMatchingLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFAnimInstance.h"
#include "Data/AnimSetAsset.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/GameStateBase.h"
#include "Libraries/AnimFunctionLibrary.h"


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
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
    
	// Hier spiegelst du die Daten in die lokale Variable für das AnimBP
	SharedData = Proxy.SharedData;	
}

FTurnInPlaceAnimSet USHFLayerAnimInstance::GetTurnInPlaceAnimSet_Implementation() const
{
	return SharedData.CMCStates.bIsCrouching ? TurnInPlaceAnimSetCrouched_Cached : TurnInPlaceAnimSet_Cached;
}


void USHFLayerAnimInstance::UpdateFromMain(const FSHFSharedAnimData& NewData)
{
	FSHFAnimInstanceProxy& Proxy = GetProxyOnGameThread<FSHFAnimInstanceProxy>();
	Proxy.SharedData = NewData;
	
	if (Proxy.SharedData.bIsMoving)
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds(); //Reset
	
	OnDataUpdated();
	
	SetAnimSet(NewData.EquipMode, false);
	CurrentEquipMode = NewData.EquipMode;
	
	SetAnimSet(CurrentEquipMode, false);	//According to the docs, this is the recommed way...
}

FAnimInstanceProxy* USHFLayerAnimInstance::CreateAnimInstanceProxy()
{
	return new FSHFAnimInstanceProxy(this);
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

void USHFLayerAnimInstance::Movement_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	FSequencePlayerReference Player = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
    
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		// Hole die Ziel-Animation für den IST-Zustand
		if (FCardinalAnimationSet* Set = MovementAnims_Cached.Find(SharedData.Gait))
		{
			UAnimSequence* InitialAnim = Set->SelectAnim(SharedData.MovementDirection);
            
			// WICHTIG: Hier Blending auf 0.0f setzen!
			// Das sorgt dafür, dass der Player SOFORT auf dieser Anim steht,
			// während die State Machine im Main-ABP den eigentlichen 0.2s Blend macht.
			USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, Player, InitialAnim, 0.0f);
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
			UAnimSequenceBase* CurrentAnim = USequencePlayerLibrary::GetSequencePure(Player);
         
			if (CurrentAnim != TargetAnim)
			{
				USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, Player, TargetAnim, 0.2f);
			}
		}

		// 3. PlayRate setzen (Darf jeden Frame passieren, da es die Zeit nicht zurücksetzt)
		// Lyra Walk ist ca. 160 units/s, Run ca. 380 units/s

		float SafePlayRate = FMath::Max(0.1f, SharedData.GroundSpeed / UAnimFunctionLibrary::GetAnimRefSpeed(SharedData.Gait));
		USequencePlayerLibrary::SetPlayRate(Player, SafePlayRate);
	}	
}


void USHFLayerAnimInstance::Start_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	
	ESHFMovementDirection AccelDir = DetermineAccelerationDirection(Proxy); 
	
	Start_MaxDistance = 0.f;
	bStartFinished = false;
	StartAnimDeltaTime = 0.f;
	
	// 3. Animation basierend auf dem Winkel wählen

	LastAccelerationDirection = AccelDir;
	
	if (UAnimSequence* SelectedAnim = StartAnims_Cached.Find(SharedData.Gait)->SelectAnim(AccelDir))
	{
		float AnimLength = SelectedAnim->GetPlayLength();
		Start_MaxDistance = SelectedAnim->EvaluateCurveData(FName("distance"), AnimLength);
		
		EAnimNodeReferenceConversionResult Result;
		const FSequenceEvaluatorReference SequenceEvaluator = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
		if (Result == EAnimNodeReferenceConversionResult::Succeeded)
		{
			USequenceEvaluatorLibrary::SetSequenceWithInertialBlending(Context, SequenceEvaluator, SelectedAnim, 0.1f);
			USequenceEvaluatorLibrary::SetExplicitTime(SequenceEvaluator, 0.f);
		}
	}
	
	StartLocation = Proxy.GetComponentTransform().GetLocation() * FVector(1.f, 1.f, 0.f);
}

void USHFLayerAnimInstance::Start_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Eval = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);

	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		
		DistanceTraveled = FVector::Distance(Proxy.GetComponentTransform().GetLocation() * FVector(1.f, 1.f, 0.f), StartLocation);

		// 2. Sicherheits-Check: Passt die Animation noch zur Eingabe?
		ESHFMovementDirection AccelDir = DetermineAccelerationDirection(Proxy);
		
		UAnimSequenceBase* CurrentAnim = USequenceEvaluatorLibrary::GetSequence(Eval);
		UAnimSequenceBase* NewDesiredAnim = StartAnims_Cached.Find(SharedData.Gait)->SelectAnim(AccelDir);

		if (NewDesiredAnim != CurrentAnim && DistanceTraveled < 15.0f) // Nur am Anfang korrigieren
		{

			USequenceEvaluatorLibrary::SetSequenceWithInertialBlending(Context, Eval, NewDesiredAnim, 0.15f);
            
		}
		
		StartAnimDeltaTime += Context.GetContext()->GetDeltaTime();
		bool bDistReached = (Start_MaxDistance > 0.f && (DistanceTraveled / Start_MaxDistance >= 0.8f));
		bool bTimeReached = (StartAnimDeltaTime > (NewDesiredAnim->GetPlayLength() * 0.8f));
		
		bStartFinished = bDistReached || bTimeReached;
		
		if (!bStartFinished)
			UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching(Context, Eval, DistanceTraveled, FName("distance"));
		else
		{
			float VelocityMagnitude = Proxy.SharedData.GroundSpeed;
			float PlayRateMultiplier = FMath::Clamp(VelocityMagnitude / UAnimFunctionLibrary::GetAnimRefSpeed(SharedData.Gait), 0.8f, 1.5f);

			USequenceEvaluatorLibrary::AdvanceTime(Context, Eval, Context.GetContext()->GetDeltaTime()*PlayRateMultiplier);			
		}
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

ESHFMovementDirection USHFLayerAnimInstance::DetermineAccelerationDirection(const FSHFAnimInstanceProxy& Proxy)
{
	// 1. Wunschrichtung (Welt)
	FVector Acceleration = Proxy.SharedData.CharacterAcceleration;
    
	if (Acceleration.IsNearlyZero()) {
		Acceleration = Proxy.GetActorTransform().GetUnitAxis(EAxis::X);
	}

	FRotator MeshRotation = Proxy.GetComponentTransform().Rotator();

	// 2. Korrektur des Blueprint-Offsets: 
	// Wenn dein Mesh im BP um -90 Grad gedreht ist, addieren wir diese 90 Grad hier wieder drauf,
	// um die "echte" Blickrichtung der Nase des Charakters zu erhalten.
	float ActualVisualYaw = FRotator::NormalizeAxis(MeshRotation.Yaw + 90.0f); 

	// 3. Berechne den relativen Winkel zur Wunschrichtung (Acceleration)
	float RelativeAngle = FRotator::NormalizeAxis(Acceleration.ToOrientationRotator().Yaw - ActualVisualYaw);
	
	ESHFMovementDirection AccelDir;

	if (RelativeAngle >= -45.f && RelativeAngle <= 45.f) {
		AccelDir = ESHFMovementDirection::Forward;
	}
	else if (RelativeAngle > 45.f && RelativeAngle <= 135.f) {
		AccelDir = ESHFMovementDirection::Right;
	}
	else if (RelativeAngle < -45.f && RelativeAngle >= -135.f) {
		AccelDir = ESHFMovementDirection::Left;
	}
	else {
		// Deckt den Bereich von 135 bis 180 und -135 bis -180 ab
		AccelDir = ESHFMovementDirection::Backward;
	}
	return AccelDir;	
}
