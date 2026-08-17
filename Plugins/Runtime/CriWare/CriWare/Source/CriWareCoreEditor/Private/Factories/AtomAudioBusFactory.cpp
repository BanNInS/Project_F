/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAttenuationFactory.cpp
 *
 ****************************************************************************/

#include "AtomAudioBusFactory.h"

#include "Atom/AtomAudioBus.h"

UAtomAudioBusFactory::UAtomAudioBusFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomAudioBus::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomAudioBusFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomAudioBus* AudioBus = NewObject<UAtomAudioBus>(InParent, InName, Flags);

	return AudioBus;
}

bool UAtomAudioBusFactory::CanCreateNew() const
{
	return true;
}
