/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Editor module for CriWare on Windows.
* File     : CriWareWindowsEditor.cpp
*
****************************************************************************/

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/Class.h"

#include "CriWareWindowsSettings.h"
#include "Customizations/CriWareWindowsSettingsDetails.h"

#define LOCTEXT_NAMESPACE "CriWareWindowsEditorModule"

/**
 * Module for Windows project settings
 */
class FCriWareWindowsEditorModule
	: public IModuleInterface
{
	// IModuleInterface interface

	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

		// Custom Class Layouts
		auto AddCustomClass = [this, InPropertyModule = &PropertyModule](FName Name, FOnGetDetailCustomizationInstance InstanceGetter)
		{
			InPropertyModule->RegisterCustomClassLayout(Name, InstanceGetter);
			CustomClassLayoutNames.Add(Name);
		};
		AddCustomClass("CriWareWindowsSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FCriWareWindowsSettingsDetails::MakeInstance));
	}

	virtual void ShutdownModule() override
	{
		// Unregister Custom Class Layouts
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			for (FName ClassName : CustomClassLayoutNames)
			{
				PropertyModule.UnregisterCustomClassLayout(ClassName);
			}
			CustomClassLayoutNames.Reset();
		}
	}

	/** The collection of registered customized class names. */
	TArray<FName> CustomClassLayoutNames;
};


IMPLEMENT_MODULE(FCriWareWindowsEditorModule, CriWareWindowsEditor);

#undef LOCTEXT_NAMESPACE
