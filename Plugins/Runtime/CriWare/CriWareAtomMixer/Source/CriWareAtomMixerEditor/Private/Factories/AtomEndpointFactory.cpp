/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEndpointFactory.cpp
 *
 ****************************************************************************/

#include "AtomEndpointFactory.h"

#include "Atom/Atom.h"
#include "Atom/AtomBus.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom Endpoint Factory
 *****************************************************************************/

UAtomEndpointFactory::UAtomEndpointFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomEndpoint::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomEndpointFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomEndpoint* AtomEndpoint = NewObject<UAtomEndpoint>(InParent, Name, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomEndpoint;
}

bool UAtomEndpointFactory::CanCreateNew() const
{
	return true;
}

/*
 * Atom Soundfield  Factory
 *****************************************************************************/

UAtomSoundfieldEndpointFactory::UAtomSoundfieldEndpointFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundfieldEndpoint::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundfieldEndpointFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundfieldEndpoint* AtomEndpoint = NewObject<UAtomSoundfieldEndpoint>(InParent, InName, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomEndpoint;
}

bool UAtomSoundfieldEndpointFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
