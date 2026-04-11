// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Implementation/TurnInPlaceCharacter.h"
#include "SHFCharacterBase.generated.h"

class UAnimComponent;

UCLASS()
class SHF_API ASHFCharacterBase : public ATurnInPlaceCharacter
{
	GENERATED_BODY()

public:
	ASHFCharacterBase();
	virtual void BeginPlay() override;
	
	virtual void Tick( float DeltaSeconds ) override;
	
	UFUNCTION(BlueprintNativeEvent, Category = "SHF|Setup")
	void LinkAnimLayer(TSubclassOf<UAnimInstance> NewLayerClass);


protected:
	virtual bool ShouldReplicateAcceleration() const override { return true; }
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "SHF|Core")
	TObjectPtr<UAnimComponent> AnimComponent;

};
