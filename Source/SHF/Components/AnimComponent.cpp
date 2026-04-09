// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimComponent.h"


#include "AnimInstances/SHFAnimInstance.h"
#include "Character/SHFCharacterBase.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "UniversalObjectLocators/AnimInstanceLocatorFragment.h"

UAnimComponent::UAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UAnimComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentLayerTag)
	DOREPLIFETIME(ThisClass, CurrentEquipMode)
	DOREPLIFETIME(ThisClass, CurrentMovementGait)
}

void UAnimComponent::SetAnimLayerTag(ESHFAnimLayerTag NewTag)
{
	if (CurrentLayerTag == NewTag) return;
	
	if (GetOwnerRole() < ROLE_Authority)
	{
		Server_SetAnimLayerTag(NewTag);
	}
	
	CurrentLayerTag = NewTag;
	OnRep_CurrentLayerTag();	//TODO: Check, if this is necessary - Server function also calls this 
}

void UAnimComponent::SetEquipMode(ESHFEquipMode NewEquipMode)
{
	if (CurrentEquipMode == NewEquipMode) return;
	
	if (GetOwnerRole() < ROLE_Authority)
	{
		Server_SetEquipMode(NewEquipMode);
	}
	CurrentEquipMode = NewEquipMode;
	
}

void UAnimComponent::SetMovementGait(ESHFGait NewMovementGait)
{
	if (CurrentMovementGait == NewMovementGait) return;
	
	if (GetOwnerRole() < ROLE_Authority)
	{
		Server_SetMovementGait(NewMovementGait);
	}
	CurrentMovementGait = NewMovementGait;
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

void UAnimComponent::Server_SetEquipMode_Implementation(ESHFEquipMode NewEquipMode)
{
	CurrentEquipMode = NewEquipMode;
	OnRep_CurrentEquipMode();
}

bool UAnimComponent::Server_SetEquipMode_Validate(ESHFEquipMode NewEquipMode)
{
	return true;
}

void UAnimComponent::Server_SetMovementGait_Implementation(ESHFGait NewMovementGait)
{
	CurrentMovementGait = NewMovementGait;
	OnRep_CurrentMovementGait();
}

bool UAnimComponent::Server_SetMovementGait_Validate(ESHFGait NewMovementGait)
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

void UAnimComponent::OnRep_CurrentEquipMode()
{
	if (IsValid(MainAnimInstance))
	{
		if (IAnimInstanceInterface* AnimInstInterface = Cast<IAnimInstanceInterface>(MainAnimInstance))
		{
			AnimInstInterface->ReceiveNewEquipMode(CurrentEquipMode);
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("onRep_CurrentEquipMode"));
		}
	}
}

void UAnimComponent::OnRep_CurrentMovementGait()
{
	if (IsValid(MainAnimInstance))
	{
		if (IAnimInstanceInterface* AnimInstInterface = Cast<IAnimInstanceInterface>(MainAnimInstance))
		{
			AnimInstInterface->ReceiveNewGait(CurrentMovementGait);
		}
	}
}

void UAnimComponent::ApplyLayer(TSubclassOf<UAnimInstance> LayerClass)
{
	if (!OwningCharacter || !LayerClass) return;

	if (bFirstLayerLink)
	{
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
			bFirstLayerLink = false;
		}
	} else if (ASHFCharacterBase* SHFChar = Cast<ASHFCharacterBase>(OwningCharacter))
	{
		SHFChar->LinkAnimLayer(LayerClass);
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
		
		//Do the same for the EquipMode and the Gait
		CurrentEquipMode = InitialEquipMode;
		OnRep_CurrentEquipMode();
		
		CurrentMovementGait = InitialMovementGait;
		OnRep_CurrentMovementGait();
	}	
	
	RootYawOffset = 0.0f;
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

void UAnimComponent::OnUpdateSimulatedProxiesMovement()
{
	if (MainAnimInstance)
		MainAnimInstance->OnUpdateSimulatedProxiesMovement();
}

void UAnimComponent::RegisterMainAnimInstance(USHFAnimInstance* NewAnimInstance)
{
	if (NewAnimInstance)
		MainAnimInstance = NewAnimInstance;
		
	
}

