// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SHFGlobals.h"
#include "Components/ActorComponent.h"
#include "AnimComponent.generated.h"


class UCharacterMovementComponent;
class USHFAnimInstance;
enum class ESHFTurnState : uint8;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHF_API UAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAnimComponent();
	
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void RegisterMainAnimInstance(USHFAnimInstance* NewAnimInstance);
	
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|Setup")
	ESHFAnimLayerTag InitialLayerTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|Setup")
	ESHFEquipMode InitialEquipMode;
	
	UPROPERTY(EditDefaultsOnly, Category = "SHF|Setup")
	ESHFGait InitialMovementGait;
	
	UFUNCTION(BlueprintCallable, Category = "SHF|Animation")
	void SetAnimLayerTag(ESHFAnimLayerTag NewTag);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|Animation")
	void SetEquipMode(ESHFEquipMode NewEquipMode);
	
	UFUNCTION(BlueprintCallable, Category = "SHF|Animation")
	void SetMovementGait(ESHFGait NewMovementGait);
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "SHF|Animation");
	float RootYawOffset = 0.f;


protected:

	UPROPERTY(EditAnywhere, Category = "SHF|Config")
	TMap<ESHFAnimLayerTag,TSubclassOf<UAnimInstance>> LayerConfig;
	
	UPROPERTY(EditAnywhere, Category = "SHF|Config")
	TMap<ESHFGait, FCMCMovementConfig> GaitConfig;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentLayerTag)
	ESHFAnimLayerTag CurrentLayerTag = ESHFAnimLayerTag::None;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquipMode)
	ESHFEquipMode CurrentEquipMode = ESHFEquipMode::Mode1;
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentMovementGait)
	ESHFGait CurrentMovementGait = ESHFGait::Run;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetAnimLayerTag(ESHFAnimLayerTag NewTag);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetEquipMode(ESHFEquipMode NewEquipMode);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetMovementGait(ESHFGait NewMovementGait);
	
	UFUNCTION()
	void OnRep_CurrentLayerTag();
	
	UFUNCTION()
	void OnRep_CurrentEquipMode();
	
	UFUNCTION()
	void OnRep_CurrentMovementGait();

	
	void ApplyLayer(TSubclassOf<UAnimInstance> LayerClass);
	
	void ApplyMovementConfig (const FCMCMovementConfig& NewConfig);
	
private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter;
	
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComp;
	
	bool bInitialLayerLinked = false;
	bool bFirstLayerLink = true;
	
	UPROPERTY()
	TObjectPtr<USHFAnimInstance> MainAnimInstance;
	
public:
	FORCEINLINE ESHFEquipMode GetCurrentEquipMode() const { return CurrentEquipMode; }
	FORCEINLINE ESHFGait GetCurrentMovementGait() const { return CurrentMovementGait; }
	
	
};
