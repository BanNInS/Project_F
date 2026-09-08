/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLInk plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareAudioLinkEditor module
 * File     : CriWareAudioLinkEditorModule.cpp
 *
 ****************************************************************************/

#include "CriWareAudioLinkEditorModule.h"

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include "CriWareAudioLinkEditorPrivate.h"
#include "Customizations/AtomAudioLinkSettingsDetails.h"

DEFINE_LOG_CATEGORY(LogCriWareAudioLinkEditor);

//class FCriWareAudioLinkEditorModule
//	: public IModuleInterface
//{
//public:

FCriWareAudioLinkEditorModule::FCriWareAudioLinkEditorModule()
	//virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

		// Custom Class Layouts
		auto RegisterCustomClass = [this, InPropertyModule = &PropertyModule](FName Name, FOnGetDetailCustomizationInstance InstanceGetter)
		{
			InPropertyModule->RegisterCustomClassLayout(Name, InstanceGetter);
			CustomClassLayoutNames.Add(Name);
		};

		RegisterCustomClass("AtomAudioLinkSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FAtomAudioLinkSettingsDetails::MakeInstance));
	}

FCriWareAudioLinkEditorModule::~FCriWareAudioLinkEditorModule()
	//virtual void ShutdownModule() override
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

//private:

	/** The collection of registered customized class names. */
	//TArray<FName> CustomClassLayoutNames;
//};

//IMPLEMENT_MODULE(FCriWareAudioLinkEditorModule, CriWareAudioLinkEditor)
