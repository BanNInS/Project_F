
#include "Atom/Gameplay/AtomComponentGroup.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#include "Atom/Gameplay/AtomComponentGroupDebug.h"
#include "Atom/AtomComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomComponentGroup)

UAtomComponentGroup::UAtomComponentGroup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_LastDemotable;
}

UAtomComponentGroup* UAtomComponentGroup::StaticGetOrCreateComponentGroup(AActor* Actor)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UAtomComponentGroup::StaticGetOrCreateComponentGroup);

	AActor* Owner = Actor;
	AActor* HighestValidOwner = nullptr;
	TInlineComponentArray<TObjectPtr<UAtomComponentGroup>> SoundGroups;

	while (Owner != nullptr)
	{
		Owner->GetComponents(SoundGroups);
		if (!SoundGroups.IsEmpty())
		{
			return SoundGroups[0];
		}

		HighestValidOwner = Owner;

		// prevents sound groups on pawns from spawning on owning playercontrollers
		if (HighestValidOwner->IsA(APawn::StaticClass()))
		{
			break;
		}
		
		Owner = Owner->GetOwner();
	}

	if (HighestValidOwner != nullptr)
	{
		UAtomComponentGroup* SoundGroup = NewObject<UAtomComponentGroup>(HighestValidOwner);
		SoundGroup->RegisterComponent();

		return SoundGroup;
	}
	
	return nullptr;
}

void UAtomComponentGroup::BeginPlay()
{
	Super::BeginPlay();
}

void UAtomComponentGroup::OnRegister()
{
	if (bAddAllActorAtomComponents)
	{
		// todo: better extra list
		TArray<TObjectPtr<UAtomComponent>> AtomComponents;
		GetAllAtomComponents(AtomComponents);

		for (TObjectPtr<UAtomComponent> AtomComponent : AtomComponents)
		{
			if (AtomComponent)
			{
				AddExternalComponent(AtomComponent);
			}
		}
	}

	Super::OnRegister();
}

void UAtomComponentGroup::OnUnregister()
{
	TArray<TObjectPtr<UAtomComponent>> AtomComponents;
	GetAllAtomComponents(AtomComponents);

	for (TObjectPtr<UAtomComponent> AtomComponent : AtomComponents)
	{
		if (AtomComponent)
		{
			RemoveExternalComponent(AtomComponent);
		}
	}

	Super::OnUnregister();
}

#if WITH_EDITOR
void UAtomComponentGroup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	/*if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UAtomComponentGroup, bAddParentActor) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UAtomComponentGroup, bIncludeFromChildActorComponents))
	{
		// todo: better list
		RemoveActorComponents(GetOwner());

		if (bAddParentActor)
		{
			AddActorComponents(GetOwner(), bIncludeFromChildActorComponents);
		}
	}*/

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomComponentGroup::AddActorComponents(AActor* Actor, bool bInIncludeFromChildActors)
{
	if (Actor && !Actors.Contains(Actor))
	{
		Actors.Add(Actor);

		TInlineComponentArray<TObjectPtr<UAtomComponent>> AtomComponents;
		Actor->GetComponents(AtomComponents, bInIncludeFromChildActors);
		for (TObjectPtr<UAtomComponent> AtomComponent : AtomComponents)
		{
			if (AtomComponent)
			{
				AddExternalComponent(AtomComponent);
			}
		}
	}
}

void UAtomComponentGroup::RemoveActorComponents(AActor* Actor)
{
	if (Actor && Actors.Contains(Actor))
	{
		TInlineComponentArray<TObjectPtr<UAtomComponent>> AtomComponents;
		Actor->GetComponents(AtomComponents, true);
		for (TObjectPtr<UAtomComponent> AtomComponent : AtomComponents)
		{
			if (AtomComponent)
			{
				RemoveExternalComponent(AtomComponent);
			}
		}

		Actors.Remove(Actor);
	}
}

void UAtomComponentGroup::StopSound(UAtomSoundBase* Sound, const float FadeTime /*= 0.f*/)
{
	constexpr float TargetVolume = 0.f;

	for (TObjectPtr<UAtomComponent>& Comp : Components)
	{
		if (Comp && Comp->IsPlaying() && Comp->Sound == Sound)
		{
			Comp->FadeOut(FadeTime, TargetVolume);
		}
	}

	for (TWeakObjectPtr<UAtomComponent> Comp : ExternalComponents)
	{
		if (Comp.IsValid() && Comp->IsPlaying() && Comp->Sound == Sound)
		{
			Comp->FadeOut(FadeTime, TargetVolume);
		}
	}
}

bool UAtomComponentGroup::IsPlayingAny() const
{
	return OnStopped.IsBound();
}

void UAtomComponentGroup::BroadcastStopAll()
{
	OnStopped.Broadcast();
}

void UAtomComponentGroup::BroadcastKill()
{
	OnKilled.Broadcast();
}

void UAtomComponentGroup::BroadcastEvent(const FName EventName)
{
	if (TArray<FAtomSoundCallback>* EventCallbacks = EventSubscriptions.Find(EventName))
	{
		for (auto EventIt = EventCallbacks->CreateIterator(); EventIt; ++EventIt)
		{
			if (EventIt->IsBound())
			{
				EventIt->Execute(EventName);
			}
			else
			{
				EventIt.RemoveCurrent();
			}
		}
	}
}

UAtomComponent* UAtomComponentGroup::GetNextAvailableComponent()
{
	UAtomComponent* ReturnComponent = nullptr;

	for (TObjectPtr<UAtomComponent>& Comp : Components)
	{
		if (Comp && !Comp->IsPlaying() && Comp->ActiveCount == 0)
		{
			ReturnComponent = ResetComponent(Comp);
			break;
		}
	}

	if (ReturnComponent == nullptr)
	{
		ReturnComponent = AddComponent();

		for (TScriptInterface<IAtomComponentGroupExtension> Extension : Extensions)
		{
			Extension->OnComponentAdded(ReturnComponent);
		}
	}

	return ReturnComponent;
}

UAtomComponent* UAtomComponentGroup::AddComponent()
{
	UAtomComponent* NewComponent = nullptr;
	if (AActor* Owner = GetOwner())
	{
		NewComponent = NewObject<UAtomComponent>(Owner);
		NewComponent->bAutoActivate = false;
		NewComponent->bAutoManageAttachment = true;

		USceneComponent* RootComponent = Owner->GetRootComponent();
		NewComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		if (RootComponent)
		{
			NewComponent->AutoAttachParent = RootComponent->GetAttachParent();
		}
		
		NewComponent->AutoAttachLocationRule = EAttachmentRule::KeepRelative;
		NewComponent->AutoAttachRotationRule = EAttachmentRule::KeepRelative;
		
		NewComponent->RegisterComponent();

		ApplyParams(NewComponent);

		ApplyModifiers(NewComponent, CachedModifier);

		ApplySoundClass(NewComponent, GroupSoundClass);

		Components.Add(NewComponent);
	}

	return NewComponent;
}

UAtomComponent* UAtomComponentGroup::ResetComponent(UAtomComponent* Component) const
{
	if (Component)
	{
		Component->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		Component->ResetParameters();
		Component->bAllowSpatialization = true;

		ApplyParams(Component);

		ApplyModifiers(Component, CachedModifier);

		ApplySoundClass(Component, GroupSoundClass);
	}

	return Component;
}

void UAtomComponentGroup::RemoveComponent(const UAtomComponent* InComponent)
{
	int32 Index = Components.IndexOfByKey(InComponent);
	if (Index != INDEX_NONE)
	{
		if (UAtomComponent* Component = Components[Index])
		{
			Component->Stop();
		}
		Components.RemoveAt(Index);
	}
}

void UAtomComponentGroup::AddExternalComponent(UAtomComponent* ComponentToAdd)
{
	if (ComponentToAdd != nullptr)
	{
		ApplyParams(ComponentToAdd);

		ApplyModifiers(ComponentToAdd, CachedModifier);

		ApplySoundClass(ComponentToAdd, GroupSoundClass);

		ExternalComponents.Add(TWeakObjectPtr<UAtomComponent>(ComponentToAdd));
	}
}

void UAtomComponentGroup::RemoveExternalComponent(UAtomComponent* ComponentToRemove)
{
	if (ComponentToRemove == nullptr)
	{
		return;
	}

	ExternalComponents.Remove(ComponentToRemove);
}

void UAtomComponentGroup::EnableVirtualization()
{
	if (!bIsVirtualized)
	{
		bIsVirtualized = true;
		OnVirtualized.Broadcast();
	}
}

void UAtomComponentGroup::DisableVirtualization()
{
	if (bIsVirtualized)
	{
		// update ParamsToSet so that all parameters are updated next go-around, but merge any pending values first
		TArray<FAtomAisacParameter> Values = ParamsToSet;
		FAtomAisacParameter::Merge(MoveTemp(Values), PersistentParams);
		
		ParamsToSet = PersistentParams;
		
		bIsVirtualized = false;
		OnUnvirtualized.Broadcast();
	}
}

void UAtomComponentGroup::AddExtension(TScriptInterface<IAtomComponentGroupExtension> NewExtension)
{
	Extensions.AddUnique(NewExtension);
	NewExtension->OnAddedToGroup(this);
}

void UAtomComponentGroup::RemoveExtension(TScriptInterface<IAtomComponentGroupExtension> NewExtension)
{
	Extensions.Remove(NewExtension);
}

void UAtomComponentGroup::UpdateExtensions(const float DeltaTime)
{
	FAtomComponentModifier ExtensionModifier;
	for(TScriptInterface<IAtomComponentGroupExtension> Extension : Extensions)
	{
		Extension->Update(DeltaTime, this, ExtensionModifier);
	}

	ExtensionModifier.Combine(GroupModifier);

	if(bIsVirtualized)
	{
		ExtensionModifier.Volume = 0.f;
	}

	if(ExtensionModifier.IsNearlyEqual(CachedModifier) == false)
	{
		IterateComponents([this, &ExtensionModifier](UAtomComponent* Component)
		{
			ApplyModifiers(Component, ExtensionModifier);
		});
	
		CachedModifier = ExtensionModifier;
	}
}

void UAtomComponentGroup::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UAtomComponentGroup::TickComponent);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateExtensions(DeltaTime);

	UpdateComponentParameters();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	FAtomComponentGroupDebug::DebugPrint(this);
#endif
}

float UAtomComponentGroup::GetAisacParamValue(const FAtomAisacControl& ParamControl) const
{
	if (const FAtomAisacParameter* FoundParam = GetParamInternal(ParamControl))
	{
		return FoundParam->Value;
	}

	return 0.f;
}

void UAtomComponentGroup::SubscribeToEvent(const FName EventName, FAtomSoundCallback Delegate)
{
	if (TArray<FAtomSoundCallback>* EventCallbacks = EventSubscriptions.Find(EventName))
	{
		EventCallbacks->AddUnique(Delegate);
	}
	else
	{
		TArray<FAtomSoundCallback> NewSubscription = { Delegate };
		EventSubscriptions.Add(EventName, NewSubscription);
	}
}

void UAtomComponentGroup::ResetParameters()
{
	PersistentParams.Reset();
}

void UAtomComponentGroup::SetTriggerParameter(const FAtomAisacControl& InControl)
{
	ExecuteEventSubscriptions(InControl.Name);

	IterateComponents([InControl](UAtomComponent* Component)
	{
		if (Component->IsPlaying())
		{
			Component->SetTriggerParameter(InControl);
		}
	});
}

void UAtomComponentGroup::SetAisacParameter(const FAtomAisacControl& InControl, float InValue)
{
	SetParameters({ FAtomAisacParameter(InControl, InValue) });
}

void UAtomComponentGroup::SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InParameters)
{
	TArray<FAtomAisacParameter> Values = InParameters;
	SetParameters(MoveTemp(Values));
}

void UAtomComponentGroup::SetParameter(FAtomAisacParameter&& InValue)
{
	SetParameters({ MoveTemp(InValue) });
}

void UAtomComponentGroup::SetParameters(TArray<FAtomAisacParameter>&& InValues)
{
	InValues.RemoveAll([](FAtomAisacParameter& Param) { return !Param.Control.IsValid(); });

	for (FAtomAisacParameter& Parameter : InValues)
	{
		//ExecuteEventSubscriptions(Parameter.Control.Name);
	}
	
	FAtomAisacParameter::Merge(MoveTemp(InValues), ParamsToSet);
}

void UAtomComponentGroup::ApplyParams(UAtomComponent* Component) const
{
	if (Component)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UAtomComponentGroup::ApplyParams);

		// if the component is playing already, InstanceParameters will be ignored
		if (Component->IsPlaying())
		{
			TArray<FAtomAisacParameter> Params = PersistentParams;
			Component->SetParameters(MoveTemp(Params));
		}
		else
		{
			// if the component hasn't started playing yet, all of its instance parameters will be passed along at once when it starts
			TArray<FAtomAisacParameter>& InstanceParameters = Component->GetInstanceParameters();
			InstanceParameters.Append(PersistentParams);
		}
	}
}

void UAtomComponentGroup::ApplyModifiers(UAtomComponent* Component, const FAtomComponentModifier& Modifier) const
{
	if (Component)
	{
		Component->SetVolumeMultiplier(Modifier.Volume);
		Component->SetPitchMultiplier(Modifier.Pitch);
		Component->SetLowPassFilterFrequency(Modifier.LowPassFrequency);
		Component->SetHighPassFilterFrequency(Modifier.HighPassFrequency);
	}
}

void UAtomComponentGroup::ApplySoundClass(UAtomComponent* Component, UAtomSoundClass* SoundClass) const
{
	if (Component && SoundClass)
	{
		Component->SoundClassOverride = SoundClass;
	}
}

void UAtomComponentGroup::IterateComponents(const TFunction<void(UAtomComponent*)> OnIterate)
{
	Components.RemoveAll([](const UAtomComponent* Component) { return !Component; });
	for (UAtomComponent* Component : Components)
	{
		if (Component->IsActive())
		{
			OnIterate(Component);
		}
	}

	ExternalComponents.RemoveAll([](const TWeakObjectPtr<UAtomComponent> WeakComponent) { return !WeakComponent.IsValid(); });
	for (TWeakObjectPtr<UAtomComponent>& WeakComponent : ExternalComponents)
	{
		OnIterate(WeakComponent.Get());
	}
}

void UAtomComponentGroup::UpdateComponentParameters()
{
	if(ParamsToSet.IsEmpty())
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(UAtomComponentGroup::UpdateComponentParameters);

	if (bIsVirtualized == false)
	{
		IterateComponents([this](UAtomComponent* Component)
    	{
    		// todo: this could be a LOT of tiny allocs... can anything be done about that? -> todo: use TInlineArray
    		// Aisacs
			TArray<FAtomAisacParameter> Values = ParamsToSet;
    		Component->SetParameters(MoveTemp(Values));
    	});
	}
	
	FAtomAisacParameter::Merge(MoveTemp(ParamsToSet), PersistentParams);
	ParamsToSet.Reset();
}

float UAtomComponentGroup::GetComponentVolume() const
{
	if (bIsVirtualized)
	{
		return 0.f;
	}

	return CachedModifier.Volume;
}

void UAtomComponentGroup::ExecuteEventSubscriptions(const FName EventName)
{
	if (EventName.IsNone())
	{
		return;
	}
	
	if (TArray<FAtomSoundCallback>* Callbacks = EventSubscriptions.Find(EventName))
	{
		for (TArray<FAtomSoundCallback>::TIterator CallbackIt = Callbacks->CreateIterator(); CallbackIt; ++CallbackIt)
		{
			if (CallbackIt->IsBound())
			{
				CallbackIt->Execute(EventName);
			}
			else
			{
				CallbackIt.RemoveCurrent();
			}
		}
	}
}

const FAtomAisacParameter* UAtomComponentGroup::GetParamInternal(const FAtomAisacControl& ParamControl) const
{
	return PersistentParams.FindByPredicate([ParamControl](const FAtomAisacParameter& Param) { return Param.Control == ParamControl; });
}

void UAtomComponentGroup::GetAllAtomComponents(TArray<TObjectPtr<UAtomComponent>>& OutComponents) const
{
	AActor* RootActor = GetOwner();
	if (USceneComponent* RootComp = RootActor->GetRootComponent())
	{
		// Walk up to the top-most attach actor in the hierarchy (will just be the RootActor if no attachment)
		RootActor = RootComp->GetAttachmentRootActor();
	}

	// Helper to collect components from an actor
	auto CollectFromActor = [&OutComponents](const AActor* InActor)
		{
			if (InActor)
			{
				TInlineComponentArray<TObjectPtr<UAtomComponent>> TempComponents;
				constexpr bool bIncludeChildActors = false;
				InActor->GetComponents(TempComponents, bIncludeChildActors);
				OutComponents.Append(MoveTemp(TempComponents));
			}
		};

	if (RootActor)
	{
		// Grab any AudioComponents from our owner directly
		CollectFromActor(RootActor);

		// Recursively grab AudioComponents from our Attached Actor's
		TArray<AActor*> AttachedActors;
		constexpr bool bResetArray = false;
		constexpr bool bRecursivelyIncludeAttachedActors = true;
		RootActor->GetAttachedActors(AttachedActors, bResetArray, bRecursivelyIncludeAttachedActors);

		for (AActor* Actor : AttachedActors)
		{
			CollectFromActor(Actor);
		}
	}
}
