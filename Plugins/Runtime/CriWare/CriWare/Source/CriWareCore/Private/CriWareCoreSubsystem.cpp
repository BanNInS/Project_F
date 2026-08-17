/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareCoreSubsystem.cpp
 *
 ****************************************************************************/

#include "CriWareCoreSubsystem.h"

#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreModule.h"
#include "CriWareCorePrivate.h"
#if WITH_EDITOR
#include "CriWareEditor.h"
#endif

#define LOCTEXT_NAMESPACE "AtomSubsystem"

bool UCriWareCoreSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UCriWareCoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// This is called at Game Start (before postinit) and at PIE start.

	InitializeCriWare();
}

void UCriWareCoreSubsystem::Deinitialize()
{
	// This is called at Game End (before preexit) and at PIE end.

	//DeinitializeCriWare(); // actully use preexit not pie
}

void UCriWareCoreSubsystem::InitializeCriWare()
{
	ICriWareCoreModule& Module = ICriWareCoreModule::Get();
	TSharedPtr<FCriWareCore> Core = Module.GetCriWareCore();

	if (!GCriWare)
	{
#if WITH_EDITOR
		GCriWare = GCriWareEditor = NewObject<UCriWareEditor>(GetTransientPackage());

		UE_LOG(LogCriWareCore, Log, TEXT("Initializing CriWare Editor..."));
		GCriWareEditor->Init(Core);
		UE_LOG(LogCriWareCore, Log, TEXT("Initializing CriWare Editor Completed"));
#else
		GCriWare = NewObject<UCriWare>(GetTransientPackage());

		UE_LOG(LogCriWareCore, Log, TEXT("Initializing CriWare..."));
		GCriWare->Init(Core);
		UE_LOG(LogCriWareCore, Log, TEXT("Initializing CriWare Completed"));
#endif
		check(GCriWare);
	}
	else if (!GCriWare->IsInitialized())
	{
		GCriWare->Initialize();
	}
}

void UCriWareCoreSubsystem::DeinitializeCriWare()
{
	if (GCriWare)
	{
		GCriWare->Uninitialize();
	}
}

bool UCriWareCoreSubsystem::IsInitialized() const
{ 
	if (GCriWare)
	{
		return GCriWare->IsInitialized();
	}
	
	return false;
}

bool UCriWareCoreSubsystem::SetAtomConfiguration(const UAtomConfig* InAtomConfig)
{
	if (GCriWare)
	{
		return GCriWare->SetAtomConfiguration(InAtomConfig);
	}
	
	return false;
}

void UCriWareCoreSubsystem::ResetAtomConfiguration()
{
	if (GCriWare)
	{
		GCriWare->SetAtomConfiguration(nullptr);
	}
}

const UAtomConfig* UCriWareCoreSubsystem::GetAtomConfiguration() const
{
	if (GCriWare)
	{
		return  GCriWare->GetAtomConfiguration();
	}
	
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
