/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationControlBusFactory.cpp
 *
 ****************************************************************************/

#include "AtomModulationControlBusFactory.h"

#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

 /*
  * Atom Modulation Control Bus Factory
  *****************************************************************************/

UAtomModulationControlBusFactory::UAtomModulationControlBusFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomModulationControlBus::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomModulationControlBusFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAtomModulationControlBus>(InParent, InName, Flags);
}

bool UAtomModulationControlBusFactory::CanCreateNew() const
{
	return true;
}


 /*
  * Atom Modulation Control Bus Mix Factory
  *****************************************************************************/

UAtomModulationControlBusMixFactory::UAtomModulationControlBusMixFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomModulationControlBusMix::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomModulationControlBusMixFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAtomModulationControlBusMix>(InParent, InName, Flags);
}

bool UAtomModulationControlBusMixFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
