// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimInstances/SHFLayerAnimInstance.h"

#include "AnimInstances/SHFAnimInstance.h"

void USHFLayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (USHFAnimInstance* MainInst = Cast<USHFAnimInstance>(GetSkelMeshComponent()->GetAnimInstance()))
	{
		MainInst->RegisterLayer(this);
	}
}

void USHFLayerAnimInstance::UpdateFromMain(const FSHFSharedAnimData& NewData)
{
	SharedData = NewData;
	OnDataUpdated();
	
}