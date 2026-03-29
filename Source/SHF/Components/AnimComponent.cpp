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

	USkeletalMeshComponent* Mesh = OwningCharacter->GetMesh();
	if (Mesh)
	{
		// 1. Den Link physikalisch herstellen
		Mesh->LinkAnimClassLayers(LayerClass);

		// 2. Den Animator zwingen, die neue Struktur sofort zu laden
		if (UAnimInstance* MainInst = Mesh->GetAnimInstance())
		{
			// Wir rufen NativeInitializeAnimation auf der MainInstance auf,
			// damit unsere LinkedLayers-Liste (C++) sofort aktuell ist.
			if (USHFAnimInstance* SHFMain = Cast<USHFAnimInstance>(MainInst))
			{
				SHFMain->NativeInitializeAnimation();
			}

			// DAS ist der Ersatz für RecalcRequiredCurves:
			// Wir triggern ein sofortiges Update der Skelett-Struktur
			MainInst->UpdateAnimation(0.f, false);
		}
		
		// 3. Mesh-Transforms auffrischen, damit die Pose im selben Frame stimmt
		Mesh->RefreshBoneTransforms();
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

	if (!bInitialLayerLinked)
	{
		// Prüfen ob alles bereit ist
		if (OwningCharacter && OwningCharacter->GetMesh() && OwningCharacter->GetMesh()->GetAnimInstance())
		{
			// Wir prüfen, ob bereits ein Layer-Tag gesetzt wurde (z.B. durch OnRep)
			if (CurrentLayerTag != ESHFAnimLayerTag::None) 
			{
				OnRep_CurrentLayerTag();
				bInitialLayerLinked = true;
				UE_LOG(LogTemp, Log, TEXT("SHF: Initialer Layer in Tick verlinkt."));
			}
		}
	}	
}

