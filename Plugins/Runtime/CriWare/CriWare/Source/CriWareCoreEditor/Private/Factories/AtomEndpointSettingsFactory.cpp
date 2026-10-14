/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEndpointSettingsFactory.cpp
 *
 ****************************************************************************/

#include "AtomEndpointSettingsFactory.h"

#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Modules/ModuleManager.h"

#include "CriWareFactoryUtility.h"
#include "Extensions/IAtomEndpoint.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom Endpoint Settings Factory
 *****************************************************************************/

UAtomEndpointSettingsFactory::UAtomEndpointSettingsFactory(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{
   SupportedClass = UAtomEndpointSettingsBase::StaticClass();
   bCreateNew = true;
   bEditorImport = false;
   bEditAfterNew = true;
}

bool UAtomEndpointSettingsFactory::ConfigureProperties()
{
	GeneratorClass = nullptr;

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedPtr<FCriWareAssetClassParentFilter> Filter = MakeShareable(new FCriWareAssetClassParentFilter);
	Options.ClassFilters.Add(Filter.ToSharedRef());

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	Filter->AllowedChildrenOfClasses.Add(UAtomEndpointSettingsBase::StaticClass());

	const FText TitleText = LOCTEXT("CreateAtomEndpointSettingsOptions", "Pick Settings Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UAtomEndpointSettingsBase::StaticClass());

	if (bPressedOk)
	{
		GeneratorClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UAtomEndpointSettingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomEndpointSettingsBase* NewEndpointSettings = nullptr;
	if (GeneratorClass != nullptr)
	{
		NewEndpointSettings = NewObject<UAtomEndpointSettingsBase>(InParent, GeneratorClass, InName, Flags);
	}
	return NewEndpointSettings;
}

bool UAtomEndpointSettingsFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
