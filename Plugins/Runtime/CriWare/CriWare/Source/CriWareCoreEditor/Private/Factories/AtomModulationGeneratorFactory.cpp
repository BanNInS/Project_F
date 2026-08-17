/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationGeneratorFactory.cpp
 *
 ****************************************************************************/

#include "AtomModulationGeneratorFactory.h"

#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

#include "CriWareFactoryUtility.h"
#include "Atom/Modulation/AtomModulationGenerator.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

 /*
  * Atom Modulation Generator Factory
  *****************************************************************************/

UAtomModulationGeneratorFactory::UAtomModulationGeneratorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomModulationGenerator::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

bool UAtomModulationGeneratorFactory::ConfigureProperties()
{
	GeneratorClass = nullptr;

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedPtr<FCriWareAssetClassParentFilter> Filter = MakeShareable(new FCriWareAssetClassParentFilter);
	Options.ClassFilters.Add(Filter.ToSharedRef());

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	Filter->AllowedChildrenOfClasses.Add(UAtomModulationGenerator::StaticClass());

	const FText TitleText = LOCTEXT("CreateAtomModulationGeneratorOptions", "Pick Generator Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UAtomModulationGenerator::StaticClass());

	if (bPressedOk)
	{
		GeneratorClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UAtomModulationGeneratorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomModulationGenerator* NewModualtionGenerator = nullptr;
	if (GeneratorClass != nullptr)
	{
		NewModualtionGenerator = NewObject<UAtomModulationGenerator>(InParent, GeneratorClass, InName, Flags);
	}
	return NewModualtionGenerator;
}

bool UAtomModulationGeneratorFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
