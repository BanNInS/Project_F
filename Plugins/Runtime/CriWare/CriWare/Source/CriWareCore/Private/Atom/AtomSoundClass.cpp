/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundClass.cpp
 *
 ****************************************************************************/

#include "Atom/AtomSoundClass.h"

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Async/Async.h"
#include "Runtime/Launch/Resources/Version.h"
#endif // WITH_EDITOR

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRack.h"

#define LOCTEXT_NAMESPACE "AtomSoundClass"

 /*
  * FAtomSoundClassProperties Implementation
  *****************************************************************************/

FAtomSoundClassProperties::FAtomSoundClassProperties()
	: Volume(1.0f)
	, Pitch(1.0f)
	, LowPassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, HighPassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
	, AttenuationDistanceScale(1.0f)
	//, LFEBleed(0.0f)
	//, VoiceCenterChannelVolume(0.0f)
	, bAlwaysPlay(false)
	, bIsUISound(false)
	, bEnableVoiceLimitScope(false)
	, bEnableCategoryCueLimitScope(false)
	//, bCenterChannelOnly(false)
	, bApplyAmbientVolumes(false)
	//, bReverb(true)
	//, Default2DReverbSendAmount(0.0f)
	//, LoadingBehavior(EAtomSoundWaveLoadingBehavior::Inherited)
	, DefaultRack(nullptr)
{
}

 /*
  * UAtomSoundClass Implementation
  *****************************************************************************/

#if WITH_EDITOR
TSharedPtr<IAtomSoundClassEditor> UAtomSoundClass::SoundClassEditor = nullptr;
#endif

UAtomSoundClass::UAtomSoundClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bOverrideCategory(true)
#if WITH_EDITORONLY_DATA
	, SoundClassGraph(nullptr)
#endif
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomSoundClass::PostLoad()
{
	Super::PostLoad();

	// Use the main/default Atom runtime for storing and retrieving sound class properties
	FAtomRuntimeManager* RuntimeManager = (GCriWare ? GCriWare->GetAtomRuntimeManager() : nullptr);

	// Force the properties to be initialized for this sound category on all active Atom runtimes devices
	if (RuntimeManager)
	{
		RuntimeManager->RegisterSoundClass(this);
	}
}

#if WITH_EDITOR

TArray<UAtomSoundClass*> BackupChildClasses;
//EAtomSoundWaveLoadingBehavior BackupLoadingBehavior;
TArray<UAtomAisacPatch*> BackupAdditionalAisacPatchs;

void UAtomSoundClass::PreEditChange(FProperty* PropertyAboutToChange)
{
	if (PropertyAboutToChange)
	{
		if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomSoundClass, ChildClasses))
		{
			// Take a copy of the current state of child classes
			BackupChildClasses = ChildClasses;
		}
		//else if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(FAtomSoundClassProperties, LoadingBehavior))
		//{
		//	// Copy the current loading behavior in case it does not pass validation.
		//	BackupLoadingBehavior = Properties.LoadingBehavior;
		//}
		else if (PropertyAboutToChange->GetFName() == GET_MEMBER_NAME_CHECKED(FAtomAisacModulationDefaultSettings, AdditionalAisacPatches))
		{
			// Take a copy of the current attached AISAC patches
			BackupAdditionalAisacPatchs = Properties.ModulationSettings.AisacModulations.AdditionalAisacPatches;
		}
	}
}

void UAtomSoundClass::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	TArray<UAtomAisacPatch*> AisacPatchToAttach;
	TArray<UAtomAisacPatch*> AisacPatchToDetach;

	if (PropertyChangedEvent.Property)
	{
		if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UAtomSoundClass, ChildClasses))
		{
			// Find child that was changed/added
			for (int32 ChildIndex = 0; ChildIndex < ChildClasses.Num(); ChildIndex++)
			{
				if (ChildClasses[ChildIndex] && !BackupChildClasses.Contains(ChildClasses[ChildIndex]))
				{
					if (ChildClasses[ChildIndex]->RecurseCheckChild(this))
					{
						// Contains cycle so revert to old layout - launch notification to inform user
						FNotificationInfo Info(NSLOCTEXT("CriWare", "UnableToChangeSoundCategoryChildDueToInfiniteLoopNotification", "Could not change SoundChildCategory child as it would create a loop"));
						Info.ExpireDuration = 5.0f;
						Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
						FSlateNotificationManager::Get().AddNotification(Info);

						ChildClasses = BackupChildClasses;
					}
					else
					{
						// Update parentage
						ChildClasses[ChildIndex]->SetParentClass(this);
					}
					break;
				}
			}

			// Update old child's parent if it has been removed
			for (int32 ChildIndex = 0; ChildIndex < BackupChildClasses.Num(); ChildIndex++)
			{
				if (BackupChildClasses[ChildIndex] && !ChildClasses.Contains(BackupChildClasses[ChildIndex]))
				{
					BackupChildClasses[ChildIndex]->Modify();
					BackupChildClasses[ChildIndex]->ParentClass = nullptr;
				}
			}

			RefreshAllGraphs(false);
		}
		else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UAtomSoundClass, ParentClass))
		{
			// Add this sound class to the parent class if it's not already added
			if (ParentClass)
			{
				bool bIsChildClass = false;
				for (auto ChildClass : ParentClass->ChildClasses)
				{
					if (ChildClass && ChildClass == this)
					{
						bIsChildClass = true;
						break;
					}
				}

				if (!bIsChildClass)
				{
					ParentClass->Modify();
					ParentClass->ChildClasses.Add(this);
				}
			}

			Modify();
			RefreshAllGraphs(false);
		}
		/*else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FAtomSoundClassProperties, AttenuationDistanceScale))
		{
			// Until we can check FAtomSoundClassProperties during USoundWave::Serialize, we can't support ForceInline here.
			if (Properties.LoadingBehavior == EAtomSoundWaveLoadingBehavior::ForceInline)
			{
				FNotificationInfo Info(NSLOCTEXT("CriWare", "ForceInlineUnavailableOnSoundClass", "Using Force Inline on sound classes is currently not supported. Set each Sound Wave to Force Inline individually instead."));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Error"));
				FSlateNotificationManager::Get().AddNotification(Info);

				Properties.LoadingBehavior = BackupLoadingBehavior;
			}
		}
		else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FAtomSoundClassProperties, LoadingBehavior))
		{
			if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
			{
				AtomRuntimeManager->IterateOverAllRuntimes([this](FAtomRuntimeId RuntimeID, FAtomRuntime* InRuntime)
				{
					InRuntime->SetSoundClassDistanceScale(this, Properties.AttenuationDistanceScale, 0.2f);
				});
			}
		}*/
		else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FAtomAisacModulationDefaultSettings, AdditionalAisacPatches))
		{
			for (auto AisacPatch : Properties.ModulationSettings.AisacModulations.AdditionalAisacPatches)
			{
				if (!BackupAdditionalAisacPatchs.Contains(AisacPatch))
				{
					AisacPatchToAttach.Add(AisacPatch);
				}
			}

			for (auto AisacPatch : BackupAdditionalAisacPatchs)
			{
				if (!Properties.ModulationSettings.AisacModulations.AdditionalAisacPatches.Contains(AisacPatch))
				{
					AisacPatchToDetach.Add(AisacPatch);
				}
			}
		}
	}

	// Use the main/default audio device for storing and retrieving sound class properties
	FAtomRuntimeManager* AtomRuntimeManager = (GCriWare ? GCriWare->GetAtomRuntimeManager() : nullptr);

	// Force the properties to be initialized for this SoundClass on all active atom runtimes
	if (AtomRuntimeManager)
	{
		AtomRuntimeManager->RegisterSoundClass(this);

		/*if (!CategoryToOverride.Name.IsNone())
		{
			AtomRuntimeManager->IterateOverAllRuntimes([&](FAtomRuntimeId, FAtomRuntime* InRuntime)
			{
				for (auto AisacPatch : AisacPatchToAttach)
				{
					InRuntime->AttachAisacPatchToCategory(CategoryToOverride, AisacPatch);
				}

				for (auto AisacPatch : AisacPatchToDetach)
				{
					InRuntime->DetachAisacPatchFromCategory(CategoryToOverride, AisacPatch);
				}
			});
		}*/
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH _EDITOR

void UAtomSoundClass::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GExitPurge && GCriWare && GCriWare->GetAtomRuntimeManager())
	{
		GCriWare->GetAtomRuntimeManager()->UnregisterSoundClass(this);
	}
}

FString UAtomSoundClass::GetDesc(void)
{
	return FString::Printf(TEXT("Children: %d"), ChildClasses.Num());
}

//TArray<UAtomSoundBase*> UAtomSoundClass::GetAllSounds()
//{
//	return TArray<TObjectPtr<USoundBase>>();
//}

void UAtomSoundClass::StopAllSounds()
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(GetWorld()))
	{
		FAtomThread::RunCommandOnAtomThread([AtomRuntime, WeakThis = MakeWeakObjectPtr(this)]()
		{
			for (FAtomActiveSound* ActiveSound : AtomRuntime->GetActiveSounds())
			{
				if (ActiveSound->GetSoundClass() == WeakThis.Get())
				{
					const uint64 AtomComponentID = ActiveSound->GetAtomComponentID();
					if (AtomComponentID > 0)
					{
						FAtomThread::RunCommandOnGameThread([AtomComponentID]()
						{
							if (auto AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
							{
								AtomComponent->Stop();
							}
						});
					}
					else
					{
						ActiveSound->Stop();
					}
				}
			}
		});
	}
}

void UAtomSoundClass::PauseAllSounds(bool bPause)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(GetWorld()))
	{
		FAtomThread::RunCommandOnAtomThread([AtomRuntime, WeakThis = MakeWeakObjectPtr(this), bPause]()
		{
			for (FAtomActiveSound* ActiveSound : AtomRuntime->GetActiveSounds())
			{
				if (ActiveSound->GetSoundClass() == WeakThis.Get())
				{
					const uint64 AtomComponentID = ActiveSound->GetAtomComponentID();
					if (AtomComponentID > 0)
					{
						FAtomThread::RunCommandOnGameThread([AtomComponentID, bPause]()
						{
							if (auto AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
							{
								AtomComponent->SetPaused(bPause);
							}
						});
					}
					else
					{
						ActiveSound->bIsPaused = bPause;
					}
				}
			}
		});
	}
}

#if WITH_EDITOR
bool UAtomSoundClass::RecurseCheckChild(UAtomSoundClass* InChildSoundClass)
{
	for (auto ChildSoundClass : ChildClasses)
	{
		if (ChildSoundClass)
		{
			if (ChildSoundClass == InChildSoundClass)
			{
				return true;
			}

			if (ChildSoundClass->RecurseCheckChild(InChildSoundClass))
			{
				return true;
			}
		}
	}

	return false;
}

void UAtomSoundClass::SetParentClass(UAtomSoundClass* InParentClass)
{
	if (ParentClass != InParentClass)
	{
		if (ParentClass)
		{
			ParentClass->Modify();
			ParentClass->ChildClasses.Remove(this);
		}

		Modify();
		ParentClass = InParentClass;
	}
}

void UAtomSoundClass::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UAtomSoundClass* This = CastChecked<UAtomSoundClass>(InThis);

	Collector.AddReferencedObject(This->SoundClassGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void UAtomSoundClass::RefreshAllGraphs(bool bIgnoreThis)
{
	if (SoundClassEditor.IsValid())
	{
		// Update the graph representation of every SoundClass
		for (TObjectIterator<UAtomSoundClass> It; It; ++It)
		{
			UAtomSoundClass* SoundClass = *It;
			if (!bIgnoreThis || SoundClass != this)
			{
				if (SoundClass->SoundClassGraph)
				{
					SoundClassEditor->RefreshGraphLinks(SoundClass->SoundClassGraph);
				}
			}
		}
	}
}

void UAtomSoundClass::SetSoundClassEditor(TSharedPtr<IAtomSoundClassEditor> InAtomSoundClassEditor)
{
	check(!SoundClassEditor.IsValid());
	SoundClassEditor = InAtomSoundClassEditor;
}

TSharedPtr<IAtomSoundClassEditor> UAtomSoundClass::GetSoundClassEditor()
{
	return SoundClassEditor;
}

#endif // WITH _EDITOR

#undef LOCTEXT_NAMESPACE
