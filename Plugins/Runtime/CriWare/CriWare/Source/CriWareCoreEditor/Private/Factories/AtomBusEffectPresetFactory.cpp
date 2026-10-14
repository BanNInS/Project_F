/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusEffectPresetFactory.cpp
 *
 ****************************************************************************/

#include "AtomBusEffectPresetFactory.h"

#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

#include "CriWareFactoryUtility.h"
#include "Atom/AtomBus.h"
#include "Atom/Atom.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomBusEffectPresetFactory::UAtomBusEffectPresetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, AtomBusEffectPresetClass(nullptr)
{
	SupportedClass = UAtomBusEffectPreset::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

bool UAtomBusEffectPresetFactory::ConfigureProperties()
{
	AtomBusEffectPresetClass = nullptr;

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedPtr<FCriWareAssetClassParentFilter> Filter = MakeShareable(new FCriWareAssetClassParentFilter);
	Options.ClassFilters.Add(Filter.ToSharedRef());

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	Filter->AllowedChildrenOfClasses.Add(UAtomBusEffectPreset::StaticClass());

	const FText TitleText = LOCTEXT("CreateAtomBusEffectOptions", "Pick Bus Effect Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UAtomBusEffectPreset::StaticClass());

	if (bPressedOk)
	{
		AtomBusEffectPresetClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UAtomBusEffectPresetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomBusEffectPreset* NewBusEffectPreset = nullptr;
	if (AtomBusEffectPresetClass != nullptr)
	{
		NewBusEffectPreset = NewObject<UAtomBusEffectPreset>(InParent, AtomBusEffectPresetClass, InName, Flags);
	}
	return NewBusEffectPreset;
}

bool UAtomBusEffectPresetFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
