/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationParameterFactory.cpp
 *
 ****************************************************************************/

#include "AtomModulationParameterFactory.h"

#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

#include "CriWareFactoryUtility.h"
#include "Atom/Modulation/AtomModulationParameter.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom Modulation Parameter Factory
 *****************************************************************************/

UAtomModulationParameterFactory::UAtomModulationParameterFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomModulationParameter::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

bool UAtomModulationParameterFactory::ConfigureProperties()
{
	ParameterClass = nullptr;

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedPtr<FCriWareAssetClassParentFilter> Filter = MakeShareable(new FCriWareAssetClassParentFilter);
	Options.ClassFilters.Add(Filter.ToSharedRef());

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	Filter->AllowedChildrenOfClasses.Add(UAtomModulationParameter::StaticClass());

	const FText TitleText = LOCTEXT("CreateAtomModulationParameterOptions", "Pick Parameter Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UAtomModulationParameter::StaticClass());

	if (bPressedOk)
	{
		ParameterClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UAtomModulationParameterFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomModulationParameter* NewModualtionParameter = nullptr;
	if (ParameterClass != nullptr)
	{
		NewModualtionParameter = NewObject<UAtomModulationParameter>(InParent, ParameterClass, InName, Flags);
	}
	return NewModualtionParameter;
}

bool UAtomModulationParameterFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
