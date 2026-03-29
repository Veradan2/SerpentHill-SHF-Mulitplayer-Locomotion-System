// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimComponent.h"

#include "AnimInstances/SHFAnimInstance.h"
#include "AnimInstances/SHFLayerAnimInstance.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UAnimComponent::UAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UAnimComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentLayerTag)
}

void UAnimComponent::SetAnimLayerTag(ESHFAnimLayerTag NewTag)
{
	if (CurrentLayerTag == NewTag) return;
	
	if (GetOwnerRole() < ROLE_Authority)
	{
		Server_SetAnimLayerTag(NewTag);
	}
	
	CurrentLayerTag = NewTag;
	OnRep_CurrentLayerTag();
}


void UAnimComponent::Server_SetAnimLayerTag_Implementation(ESHFAnimLayerTag NewTag)
{
	if (CurrentLayerTag != NewTag)
	{
		CurrentLayerTag = NewTag;
        
		// Da OnRep auf dem Server nicht automatisch feuert, 
		// rufen wir es hier manuell auf.
		OnRep_CurrentLayerTag();
        
		// Durch die REPLIKATION erfahren nun alle anderen Clients davon 
		// und rufen ihre eigene OnRep_CurrentLayerTag() auf.
	}	
}

bool UAnimComponent::Server_SetAnimLayerTag_Validate(ESHFAnimLayerTag NewTag)
{
	return true;
}

void UAnimComponent::OnRep_CurrentLayerTag()
{
	if (TSubclassOf<UAnimInstance>* FoundLayer = LayerConfig.Find(CurrentLayerTag))
	{
		ApplyLayer(*FoundLayer);
	}	
}

void UAnimComponent::ApplyLayer(TSubclassOf<UAnimInstance> LayerClass)
{
	if (!OwningCharacter || !LayerClass) return;

	if (USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh())
	{
		// 1. Den neuen Layer verlinken
		Mesh->LinkAnimClassLayers(LayerClass);
        
		// 2. WICHTIG FÜR DEN WECHSEL: 
		// Wir holen die neue Instanz, die Unreal gerade erstellt hat
		if (UAnimInstance* NewLayerInst = Mesh->GetLinkedAnimLayerInstanceByClass(LayerClass))
		{
			// 3. Registrierung in der MainInstance (damit die Daten dorthin fließen)
			if (USHFAnimInstance* MainInst = Cast<USHFAnimInstance>(Mesh->GetAnimInstance()))
			{
				if (USHFLayerAnimInstance* SHF_Layer = Cast<USHFLayerAnimInstance>(NewLayerInst))
				{
					// Wir müssen die Liste in der MainInstance updaten!
					MainInst->RegisterLayer(SHF_Layer);
				}
			}
		}

		// 4. Visuellen "Push" erzwingen
		Mesh->InitAnim(true); 
	}
}

void UAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	OwningCharacter = Cast<ACharacter>(GetOwner());

	// If we are the server, set Initial Layer Type
	if (GetOwner()->HasAuthority())
	{
		// Set Initial Layer Tag from Brueprint
		CurrentLayerTag = InitialLayerTag; 
        
		// Trigger OnRep on server (important!!!!!!)
		OnRep_CurrentLayerTag();
	}	

}


void UAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// --- INITIALER LAYER-CHECK ---
	if (!bInitialLayerLinked)
	{
		// Prüfen, ob Mesh und AnimInstance schon existieren
		if (OwningCharacter && OwningCharacter->GetMesh() && OwningCharacter->GetMesh()->GetAnimInstance())
		{
			// Wir rufen OnRep manuell auf, um den Initial-Layer zu laden
			OnRep_CurrentLayerTag();
			bInitialLayerLinked = true; 
            
			UE_LOG(LogTemp, Log, TEXT("SHF: Initialer Layer erfolgreich verlinkt in Tick."));
		}
	}	
}

