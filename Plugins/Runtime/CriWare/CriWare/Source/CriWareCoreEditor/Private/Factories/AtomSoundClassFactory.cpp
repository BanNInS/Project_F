/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundClassFactory.cpp
 *
 ****************************************************************************/

#include "AtomSoundClassFactory.h"

#include "Atom/AtomSoundClass.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

 /*
  * Atom Sound Class Factory
  *****************************************************************************/

UAtomSoundClassFactory::UAtomSoundClassFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundClass::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundClassFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAtomSoundClass>(InParent, InName, Flags);
}

bool UAtomSoundClassFactory::CanCreateNew() const
{
	return true;
}
