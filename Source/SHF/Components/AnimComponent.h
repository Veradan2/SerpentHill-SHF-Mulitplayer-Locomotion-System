// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Components/ActorComponent.h"
#include "AnimComponent.generated.h"


enum class ESHFTurnState : uint8;

UENUM(BlueprintType)
enum class ESHFAnimLayerTag : uint8
{
	Unarmed,
	Pistol,
	Rifle,
	None
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHF_API UAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAnimComponent();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|Setup")
	ESHFAnimLayerTag InitialLayerTag;
	
	UFUNCTION(BlueprintCallable, Category = "SHF|Animation")
	void SetAnimLayerTag(ESHFAnimLayerTag NewTag);
	
	ESHFTurnState CurrentTurnState = ESHFTurnState::None;
	
	float RootYawOffset = 0.f;


protected:
	virtual void BeginPlay() override;
		
	UPROPERTY(EditAnywhere, Category = "SHF|Config")
	TMap<ESHFAnimLayerTag,TSubclassOf<UAnimInstance>> LayerConfig;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentLayerTag)
	ESHFAnimLayerTag CurrentLayerTag = ESHFAnimLayerTag::None;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAnimLayerTag(ESHFAnimLayerTag NewTag);
	
	UFUNCTION()
	void OnRep_CurrentLayerTag();
	
	void ApplyLayer(TSubclassOf<UAnimInstance> LayerClass);
	
private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter;
	
	bool bInitialLayerLinked = false;
	
	
	
	


		
};
