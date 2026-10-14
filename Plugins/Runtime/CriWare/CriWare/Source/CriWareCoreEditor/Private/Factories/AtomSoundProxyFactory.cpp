/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundProxyFactory.cpp
 *
 ****************************************************************************/

#include "AtomSoundProxyFactory.h"

#include "CriWareCoreEditorPrivate.h"
#include "Atom/AtomSoundProxy.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomSoundProxyFactory::UAtomSoundProxyFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundProxy::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundProxyFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundProxy* NewSoundProxy = NewObject<UAtomSoundProxy>(InParent, Name, Flags);

	// Add the sound if provided
	if (Sound)
	{
		NewSoundProxy->SetSound(Sound);
	}

	//Atom::Analytics::RecordEvent_Usage("SoundUtilities.ProxySoundCreated");

	return NewSoundProxy;
}

UMySoundFactory::UMySoundFactory(const FObjectInitializer& ObjectInitializer) : UFactory(ObjectInitializer)
{
	SupportedClass = UMySound::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UMySoundFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMySound>(InParent, InName, Flags);
}

#undef LOCTEXT_NAMESPACE
