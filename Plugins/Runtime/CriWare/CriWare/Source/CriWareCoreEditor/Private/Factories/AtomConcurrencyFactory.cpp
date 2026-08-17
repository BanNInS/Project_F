/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomConcurrencyFactory.cpp
 *
 ****************************************************************************/

#include "AtomConcurrencyFactory.h"

#include "Atom/AtomRack.h"
#include "Atom/AtomConcurrency.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

 /*
  * Atom Concurrency Factory
  *****************************************************************************/

UAtomConcurrencyFactory::UAtomConcurrencyFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomConcurrency::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomConcurrencyFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAtomConcurrency>(InParent, InName, Flags);
}

bool UAtomConcurrencyFactory::CanCreateNew() const
{
	return true;
}
