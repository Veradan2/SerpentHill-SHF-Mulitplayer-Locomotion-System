// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SHFCharacterBase.generated.h"

class UAnimComponent;

UCLASS()
class SHF_API ASHFCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	ASHFCharacterBase();
	virtual bool ShouldReplicateAcceleration() const override { return true; }
	
	virtual void OnRep_ReplicatedMovement() override;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "SHF|Core")
	TObjectPtr<UAnimComponent> AnimComponent;

};
