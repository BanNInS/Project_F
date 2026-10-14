/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWare.cpp
 *
 ****************************************************************************/

#include "CriWare.h"

#include "Misc/CoreMiscDefines.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
//#include "HAL/FileManager.h"

#include "CriWareCoreSettings.h"
#include "CriWareMemory.h"
#include "CriWareFileIo.h"
#include "CriWareCoreSubModules.h"
#include "Atom/Atom.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomPluginUtilities.h"

#include "CriWareCorePrivate.h"

#define LOCTEXT_NAMESPACE "CriWare"

 /* Global variables
  *****************************************************************************/

  /**
   * Global CriWare pointer. Can be 0 so don't use without checking.
   */
CRIWARECORE_API UCriWare* GCriWare = nullptr;

FString GCriWareIni = "CriWare";

 /* UCriWare class
  *****************************************************************************/

  //
  // Enter CriWare.
  //
void UCriWare::Init(TWeakPtr<FCriWareCore> InCriWareCore)
{
	CriWareCore = InCriWareCore;

	// Initializes
	Initialize();

	// Store Active AtomConfig
	FAtomRuntimeHandle AtomRuntime = GetActiveAtomRuntime();
	if (AtomRuntime)
	{
		AtomConfig = AtomRuntime->GetAtomConfiguration();
	}

	// Add to root.
	AddToRoot();
}

//~ UObject interface

// 
// Exit CriWare.
//
void UCriWare::FinishDestroy()
{
	// Remove from root.
	RemoveFromRoot();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// shut down all subsystems.
		GCriWare = nullptr;

		// Uninitializes
		Uninitialize();
	}

	Super::FinishDestroy();
}

#if WITH_EDITOR
void UCriWare::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCriWare, AtomConfig))
	{
		SetAtomConfiguration(AtomConfig);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UCriWare::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);
	UCriWare* This = CastChecked<UCriWare>(InThis);
	
	if (FAtomRuntimeManager* Manager = This->GetAtomRuntimeManager())
	{
		Manager->AddReferencedObjects(Collector);
	}
}

bool UCriWare::Initialize()
{
	if (auto Core = CriWareCore.Pin())
	{
		Core->InitalizeLibraries();
		return true;
	}

	return false;
}

bool UCriWare::Uninitialize()
{
	if (auto Core = CriWareCore.Pin())
	{
		Core->UninitializeLibraries();
		return true;
	}

	return false;
}

bool UCriWare::SetAtomConfiguration(const UAtomConfig* InAtomConfig)
{
	AtomConfig = InAtomConfig;

	// TODO: setup to any atom 
	FAtomRuntimeHandle AtomRuntime = GetActiveAtomRuntime();
	if (AtomRuntime)
	{
		return AtomRuntime->SetAtomConfiguration(InAtomConfig);
	}

	return false;
}

FAtomRuntimeHandle UCriWare::GetActiveAtomRuntime() const
{
	if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin())
	{
		return Core->GetActiveAtomRuntime();
	} 
	
	return FAtomRuntimeHandle();
}

FAtomRuntimeHandle UCriWare::GetMainAtomRuntime() const
{
	if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin())
	{
		return Core->GetMainAtomRuntime();
	}

	return FAtomRuntimeHandle();
}

FAtomRuntimeManager* UCriWare::GetAtomRuntimeManager() const
{
	if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin())
	{
		return Core->GetAtomRuntimeManager();
	}

	return nullptr;
}

FAtomResourceManager* UCriWare::GetAtomResourceManager() const
{
	if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin())
	{
		FAtomRuntimeManager* RuntimeManager = Core->GetAtomRuntimeManager();
		return RuntimeManager ? &RuntimeManager->GetAtomResourceManager() : nullptr;
	}

	return nullptr;
}

TAtomRuntimePluginPtr UCriWare::GetAtomRuntimePlugin(FName PluginName) const
{
	if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin())
	{
		if (FAtomRuntimeManager* RuntimeManager = Core->GetAtomRuntimeManager())
		{
			FAtomRuntimeHandle AtomRuntime = RuntimeManager->GetActiveAtomRuntime();
			if (AtomRuntime.IsValid())
			{
				for (TAtomRuntimePluginPtr Plugin : AtomRuntime->RuntimePluginInterfaces)
				{
					if (Plugin->GetPluginName() == PluginName)
					{
						return Plugin;
					}
				}
			}
		}
	}

	return nullptr;
}

bool UCriWare::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Out /* = *GLog */)
{
	return true;
}

#undef LOCTEXT_NAMESPACE
