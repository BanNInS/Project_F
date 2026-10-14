/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackFactory.cpp
 *
 ****************************************************************************/

#include "AtomRackFactory.h"

#include "Atom/AtomRack.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom Rack Factory
 *****************************************************************************/

UAtomRackFactory::UAtomRackFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomRack::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomRackFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomRack* AtomRack = NewObject<UAtomRack>(InParent, InName, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomRack;
}

bool UAtomRackFactory::CanCreateNew() const
{
	return true;
}

/*
 * Atom Soundfield Rack Factory
 *****************************************************************************/

UAtomSoundfieldRackFactory::UAtomSoundfieldRackFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundfieldRack::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundfieldRackFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundfieldRack* AtomRack = NewObject<UAtomSoundfieldRack>(InParent, InName, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomRack;
}

bool UAtomSoundfieldRackFactory::CanCreateNew() const
{
	return true;
}

/*
 * Atom Native Rack Factory
 *****************************************************************************/

UAtomEndpointRackFactory::UAtomEndpointRackFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomEndpointRack::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomEndpointRackFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomEndpointRack* AtomRack = NewObject<UAtomEndpointRack>(InParent, Name, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomRack;
}

bool UAtomEndpointRackFactory::CanCreateNew() const
{
	return true;
}

/*
 * Atom Soundfield Rack Factory
 *****************************************************************************/

UAtomSoundfieldEndpointRackFactory::UAtomSoundfieldEndpointRackFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundfieldEndpointRack::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundfieldEndpointRackFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundfieldEndpointRack* AtomRack = NewObject<UAtomSoundfieldEndpointRack>(InParent, InName, Flags);

	//class FAudioDeviceManager* AudioDeviceManager = GEngine ? GEngine->GetAudioDeviceManager() : nullptr;
	//if (AudioDeviceManager)
	//{
	//	AudioDeviceManager->InitSoundSubmixes();
	//}

	return AtomRack;
}

bool UAtomSoundfieldEndpointRackFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
