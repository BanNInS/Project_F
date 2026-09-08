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

#include "AtomAttenuationFactory.h"

#include "Atom/AtomRack.h"
#include "Atom/AtomAttenuation.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

 /*
  * Atom Attenuation Factory
  *****************************************************************************/

UAtomAttenuationFactory::UAtomAttenuationFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomAttenuation::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomAttenuationFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAtomAttenuation>(InParent, InName, Flags);
}

bool UAtomAttenuationFactory::CanCreateNew() const
{
	return true;
}
