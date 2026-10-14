/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundSourceBusFactory.cpp
 *
 ****************************************************************************/

#include "AtomSoundSourceBusFactory.h"

#include "Templates/SubclassOf.h"

#include "Atom/AtomSoundSourceBus.h"

UAtomSoundSourceBusFactory::UAtomSoundSourceBusFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundSourceBus::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundSourceBusFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundSourceBus* SoundSourceBus = NewObject<UAtomSoundSourceBus>(InParent, InName, Flags);

	return SoundSourceBus;
}

bool UAtomSoundSourceBusFactory::CanCreateNew() const
{
	return true;
}
