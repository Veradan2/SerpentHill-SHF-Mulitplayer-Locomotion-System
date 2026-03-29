// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Animation/AnimInstance.h"
#include "SHFLayerAnimInstance.generated.h"

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
	
	void UpdateFromMain(const FSHFSharedAnimData& NewData);
	
protected:
		// Lokale Kopie für das AnimBP (Thread-Safe Zugriff möglich)
	UPROPERTY(BlueprintReadOnly, Category = "SHF|Data")
	FSHFSharedAnimData SharedData;	
	
	// Eine Funktion, die wir im AnimBP-Eventgraph nutzen können
	UFUNCTION(BlueprintImplementableEvent, Category = "SHF|Events")
	void OnDataUpdated();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SHF | AnimSequences")
	TObjectPtr<UAnimSequence> IdleAnim;
	
};
