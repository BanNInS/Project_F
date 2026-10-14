
#include "Atom/Gameplay/AtomGameplayVolumeComponent.h"

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"
#include "Atom/Gameplay/Interfaces/IAtomGameplayVolumeInteraction.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomGameplayVolumeComponent"

UAtomGameplayVolumeComponent::UAtomGameplayVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = true;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomGameplayVolumeComponent::SetProxy(UAtomGameplayVolumeProxy* NewProxy)
{
	RemoveProxy();
	Proxy = NewProxy;

	if (IsActive())
	{
		AddProxy();
	}
}

void UAtomGameplayVolumeComponent::OnComponentDataChanged()
{
	if (IsActive())
	{
		UpdateProxy();
	}
}

void UAtomGameplayVolumeComponent::EnterProxy() const
{
	TInlineComponentArray<UActorComponent*> ActorComponents(GetOwner());
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		if (ActorComponent && ActorComponent->Implements<UAtomGameplayVolumeInteraction>())
		{
			IAtomGameplayVolumeInteraction::Execute_OnListenerEnter(ActorComponent);
		}
	}

	OnProxyEnter.Broadcast();
}

void UAtomGameplayVolumeComponent::ExitProxy() const
{
	TInlineComponentArray<UActorComponent*> ActorComponents(GetOwner());
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		if (ActorComponent && ActorComponent->Implements<UAtomGameplayVolumeInteraction>())
		{
			IAtomGameplayVolumeInteraction::Execute_OnListenerExit(ActorComponent);
		}
	}

	OnProxyExit.Broadcast();
}

#if WITH_EDITOR
void UAtomGameplayVolumeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomGameplayVolumeComponent, Proxy))
	{
		RemoveProxy();

		if (IsActive())
		{
			AddProxy();
		}
	}
}
#endif // WITH_EDITOR

void UAtomGameplayVolumeComponent::OnUnregister()
{
	Super::OnUnregister();
	RemoveProxy();
}

void UAtomGameplayVolumeComponent::Enable()
{
	if (Proxy != nullptr)
	{
		Super::Enable();
		AddProxy();
	}
}

void UAtomGameplayVolumeComponent::Disable()
{
	RemoveProxy();
	Super::Disable();
}

void UAtomGameplayVolumeComponent::AddProxy() const
{
	if (UAtomGameplayVolumeSubsystem* VolumeSubsystem = GetSubsystem())
	{
		VolumeSubsystem->AddVolumeComponent(this);
	}
}

void UAtomGameplayVolumeComponent::RemoveProxy() const
{
	if (UAtomGameplayVolumeSubsystem* VolumeSubsystem = GetSubsystem())
	{
		VolumeSubsystem->RemoveVolumeComponent(this);
	}
}

void UAtomGameplayVolumeComponent::UpdateProxy() const
{
	if (UAtomGameplayVolumeSubsystem* VolumeSubsystem = GetSubsystem())
	{
		VolumeSubsystem->UpdateVolumeComponent(this);
	}
}

UAtomGameplayVolumeSubsystem* UAtomGameplayVolumeComponent::GetSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		return FAtomRuntime::GetSubsystem<UAtomGameplayVolumeSubsystem>(FAtomRuntimeManager::GetAtomRuntimeFromWorld(World));
	}

	return nullptr;
}

UAtomGameplayVolumeComponentBase::UAtomGameplayVolumeComponentBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = true;
}

#undef LOCTEXT_NAMESPACE
