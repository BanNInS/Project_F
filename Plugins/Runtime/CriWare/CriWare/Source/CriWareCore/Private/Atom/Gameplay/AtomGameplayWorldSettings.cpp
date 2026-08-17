
#include "Atom/Gameplay/AtomGameplayWorldSettings.h"

#include "CriWareCorePrivate.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomVolume.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Misc/TransactionObjectEvent.h"
#endif

#define LOCTEXT_NAMESPACE "AtomGameplayWorldSettings"

AAtomGameplayWorldSettings::AAtomGameplayWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void AAtomGameplayWorldSettings::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	UWorld* World = GetWorld();

	if (FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World))
	{
		AtomRuntime->SetDefaultAtomSettings(World, FAtomReverbSettings(), DefaultAtomAmbientZoneSettings);
	}
}

#if WITH_EDITOR
void AAtomGameplayWorldSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UWorld* World = GetWorld();

	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;
	if (PropertyThatChanged)
	{
		InternalPostPropertyChanged(PropertyThatChanged->GetFName());
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AAtomGameplayWorldSettings::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	Super::PostTransacted(TransactionEvent);
	if (TransactionEvent.GetEventType() == ETransactionObjectEventType::UndoRedo)
	{
		for (const FName& PropertyName : TransactionEvent.GetChangedProperties())
		{
			InternalPostPropertyChanged(PropertyName);
		}
	}
}

void AAtomGameplayWorldSettings::InternalPostPropertyChanged(FName PropertyName)
{
	if (/*PropertyName == GET_MEMBER_NAME_CHECKED(AWorldSettings, DefaultReverbSettings) || */ PropertyName == GET_MEMBER_NAME_CHECKED(AAtomGameplayWorldSettings, DefaultAtomAmbientZoneSettings))
	{
		UWorld* World = GetWorld();
		if (FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World))
		{
			AtomRuntime->SetDefaultAtomSettings(World, FAtomReverbSettings(), DefaultAtomAmbientZoneSettings);
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
