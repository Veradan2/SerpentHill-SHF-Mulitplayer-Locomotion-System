// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"


#include "AnimCharacterMovementLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "SequencePlayerLibrary.h"
#include "AnimDistanceMatchingLibrary.h"
#include "Animation/AnimNodeReference.h"
#include "AnimInstances/SHFAnimInstance.h"
#include "Data/AnimSetAsset.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Character.h"
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
	if (APawn* Pawn = TryGetPawnOwner())
	{
		OwningCharacter = Cast<ACharacter>(Pawn);
	}
}

void USHFLayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!OwningCharacter)
	{
		if (APawn* Pawn = TryGetPawnOwner())
			OwningCharacter = Cast<ACharacter>(Pawn);
	}
	
	if (!IsValid(CachedGameState))
	{
		CachedGameState = GetWorld()->GetGameState();
	}
	
	CalculateIdleIndex();
}

void USHFLayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
    
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
		USequencePlayerLibrary::SetSequence(SequencePlayer, *IdleAnims_Cached.Find(SharedData.Gait));
		USequencePlayerLibrary::SetAccumulatedTime(SequencePlayer, 0.0f);
		// Nutze KEIN Blending beim allerersten Frame (Dauer 0.0f)
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, *IdleAnims_Cached.Find(SharedData.Gait), 0.f);		
	}
}

void USHFLayerAnimInstance::Idle_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;

	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, *IdleAnims_Cached.Find(SharedData.Gait), .2f);
		IdleActiveTimeStamp = GetWorld()->GetTimeSeconds();
	}
}

void USHFLayerAnimInstance::Idle_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	EAnimNodeReferenceConversionResult Result;
	const FSequencePlayerReference SequencePlayer = USequencePlayerLibrary::ConvertToSequencePlayer(Node, Result);
	
	if (Result != EAnimNodeReferenceConversionResult::Succeeded) return;
	
	if (UAnimSequence* IdleAnimSeq = *IdleAnims_Cached.Find(SharedData.Gait))
	{
		UAnimSequenceBase* CurrentPlaying = USequencePlayerLibrary::GetSequencePure(SequencePlayer);
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		bool bIsPlayingAnyBreak = IdleBreaks_Cached.Find(SharedData.Gait) && IdleBreaks_Cached.Find(SharedData.Gait)->AnimArray.Contains(CurrentPlaying);
	
		if (CurrentPlaying != IdleAnimSeq && !bIsPlayingAnyBreak)
		{
			USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, IdleAnimSeq, 0.2f);
			IdleActiveTimeStamp = CurrentTime;
			return;
		}
	
		if (CurrentTime - IdleActiveTimeStamp > 5.f)
		{
			if (IdleBreaks_Cached.Find(SharedData.Gait) && IdleBreaks_Cached.Find(SharedData.Gait)->AnimArray.IsValidIndex(IdleIndex))
			{
				UAnimSequence* NextBreak = IdleBreaks_Cached.Find(SharedData.Gait)->AnimArray[IdleIndex];
				USequencePlayerLibrary::SetSequenceWithInertialBlending(Context, SequencePlayer, NextBreak, .2f);
			
				IdleIndex = (IdleIndex + 1) % IdleBreaks_Cached.Num();
				IdleActiveTimeStamp = CurrentTime;
			}
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
			// Das sorgt dafür, dass der Player SOFORT auf dieser AnimArray steht,
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
	
	ESHFMovementDirection AccelDir = Proxy.SharedData.AccelerationDirection;/*DetermineAccelerationDirection(Proxy);*/ 
	
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
			USequenceEvaluatorLibrary::SetSequenceWithInertialBlending(Context, SequenceEvaluator, SelectedAnim, 0.0f);
			USequenceEvaluatorLibrary::SetExplicitTime(SequenceEvaluator, 0.f);
		}
	}
	
	StartLocation = Proxy.GetComponentTransform().GetLocation();
}

void USHFLayerAnimInstance::Start_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Eval = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);

	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		
		DistanceTraveled = FVector::Dist2D(Proxy.GetComponentTransform().GetLocation(), StartLocation);

		// 2. Sicherheits-Check: Passt die Animation noch zur Eingabe?
		ESHFMovementDirection AccelDir = Proxy.SharedData.AccelerationDirection;/*DetermineAccelerationDirection(Proxy);*/
		
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

void USHFLayerAnimInstance::Stop_OnBecomeRelevant(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();

	// 1. Stopp-Punkt im Weltraum vorhersagen (UE 5.7 Signatur)
	FVector RelativeStopVector = UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(
		Proxy.SharedData.CharacterVelocity, 
		Proxy.SharedData.MovementConfig.bUseSeparateBrakingFriction, 
		Proxy.SharedData.MovementConfig.BrakingFriction,
		Proxy.SharedData.MovementConfig.GroundFriction,
		Proxy.SharedData.MovementConfig.BrakingFrictionFactor,
		Proxy.SharedData.MovementConfig.BrakingDecelerationWalking
	);
    
	// Zielpunkt fixieren
	StopLocation = Proxy.GetComponentTransform().GetLocation() + RelativeStopVector;

	// 2. Richtung und Animation bestimmen
	ESHFMovementDirection StopDir = SharedData.MovementDirection; //DetermineMovementDirection(Proxy);
    
	if (UAnimSequence* SelectedAnim = StopAnims_Cached.Find(SharedData.Gait)->SelectAnim(StopDir))
	{
		EAnimNodeReferenceConversionResult Result;
		const FSequenceEvaluatorReference Eval = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);
		if (Result == EAnimNodeReferenceConversionResult::Succeeded)
		{
			// Sequenz setzen und Zeit auf 0
			USequenceEvaluatorLibrary::SetSequence(Eval, SelectedAnim);
			USequenceEvaluatorLibrary::SetExplicitTime(Eval, 0.0f);
		}
	}
	
#if !UE_BUILD_SHIPPING
	// Zeichnet eine rote Kugel am vorhergesagten Stopp-Punkt für 2 Sekunden
	DrawDebugSphere(
		GetWorld(), 
		StopLocation, 
		15.0f,          // Radius der Kugel
		12,             // Segmente
		FColor::Red, 
		false,          // Nicht permanent
		2.0f,           // Dauer in Sekunden
		0, 
		2.0f            // Liniendicke
	);
    
	// Zeichnet eine Linie von deiner aktuellen Position zum Stopp-Punkt
	DrawDebugLine(
		GetWorld(),
		Proxy.GetComponentTransform().GetLocation(),
		StopLocation,
		FColor::Yellow,
		false, 2.0f, 0, 1.5f
	);
#endif
}	

		


void USHFLayerAnimInstance::Stop_OnUpdate(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
	EAnimNodeReferenceConversionResult Result;
	const FSequenceEvaluatorReference Eval = USequenceEvaluatorLibrary::ConvertToSequenceEvaluator(Node, Result);

	if (Result == EAnimNodeReferenceConversionResult::Succeeded)
	{
		// 1. Physikalische Restdistanz zum Ziel messen
		float CurrentDistRemaining = FVector::Dist2D(Proxy.GetComponentTransform().GetLocation(), StopLocation);
        
		// Wert für Main-Instance spiegeln (für Transition Rule)
		Stop_DistanceRemaining = CurrentDistRemaining;

		// 2. Playrate dynamisch anpassen (Target Matching)
		UAnimDistanceMatchingLibrary::DistanceMatchToTarget(
			Eval,
			CurrentDistRemaining,
			FName("distance")
		);

		// 3. Zeit manuell voranschreiten lassen
		// Da DistanceMatchToTarget die Playrate im Node setzt, bewirkt AdvanceTime
		// nun eine flüssige Bewegung, die beim Ziel (Distanz 0) am Ende der AnimArray ankommt.
		USequenceEvaluatorLibrary::AdvanceTime(Context, Eval, Context.GetContext()->GetDeltaTime());
		
		Stop_UpdateOrientationAngle(Context.GetContext()->GetDeltaTime());
	}
}


void USHFLayerAnimInstance::Stop_UpdateOrientationAngle(float DeltaSeconds)
{
	const FSHFAnimInstanceProxy& Proxy = GetProxyOnAnyThread<FSHFAnimInstanceProxy>();
    
	FVector Velocity = Proxy.SharedData.CharacterVelocity;
    
	if (Velocity.Size2D() > 5.f)
	{
		// Berechne den Winkel zwischen Mesh-Forward und Bewegungsrichtung
		FRotator MeshRotation = Proxy.GetComponentTransform().Rotator();
        
		// Wir nutzen wieder den +90° Offset, falls dein Mesh im BP gedreht ist
		float MeshYaw = FRotator::NormalizeAxis(MeshRotation.Yaw + 90.0f);
        
		// Der finale Winkel für den Warping-Node (-180 bis 180)
		float TargetAngle =  FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw - MeshYaw);
		SharedData.OrientationWarpingLocomotionAngle = FMath::FInterpTo(SharedData.OrientationWarpingLocomotionAngle, TargetAngle, DeltaSeconds, 12.0f);
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
			IdleAnims_Cached = MovementAnims->IdleAnims;
			IdleBreaks_Cached = MovementAnims->IdleBreakAnims;
			MovementAnims_Cached = MovementAnims->CycleAnims;
			StartAnims_Cached = MovementAnims->StartAnims;
			StopAnims_Cached = MovementAnims->StopAnims;
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

ESHFMovementDirection USHFLayerAnimInstance::DetermineMovementDirection(const FSHFAnimInstanceProxy& Proxy)
{
	FVector Velocity = Proxy.SharedData.CharacterVelocity;
	
	FRotator MeshRotation = Proxy.GetComponentTransform().Rotator();
	float ActualVisualYaw = FRotator::NormalizeAxis(MeshRotation.Yaw + 90.0f); 
	float RelativeAngle = FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw - ActualVisualYaw);
	
	ESHFMovementDirection VelDir;
	if (RelativeAngle >= -45.f && RelativeAngle <= 45.f) {
		VelDir = ESHFMovementDirection::Forward;
	}
	else if (RelativeAngle > 45.f && RelativeAngle <= 135.f) {
		VelDir = ESHFMovementDirection::Right;
	}
	else if (RelativeAngle < -45.f && RelativeAngle >= -135.f) {
		VelDir = ESHFMovementDirection::Left;
	}
	else {
		// Deckt den Bereich von 135 bis 180 und -135 bis -180 ab
		VelDir = ESHFMovementDirection::Backward;
	}
	return VelDir;		
}
