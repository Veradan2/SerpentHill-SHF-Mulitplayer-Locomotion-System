// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SHFCharacterBase.h"

#include "SHF/Components/AnimComponent.h"

ASHFCharacterBase::ASHFCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	AnimComponent = CreateDefaultSubobject<UAnimComponent>("Animation Component");
	AnimComponent->SetIsReplicated(true);
}


void ASHFCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASHFCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	
}

void ASHFCharacterBase::LinkAnimLayer_Implementation(TSubclassOf<UAnimInstance> NewLayerClass)
{
}
