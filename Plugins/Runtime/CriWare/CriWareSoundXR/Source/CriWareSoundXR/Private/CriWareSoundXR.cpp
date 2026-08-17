/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2024 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareSoundXR
 * File     : CriWareSoundXRModule.cpp
 *
 ****************************************************************************/

#include "CriWareSoundXRModule.h"
#include "CriWareSoundXRPrivate.h"

#include "Extensions/IAtomRuntimePlugin.h"

#include "SoundXRAtomRuntimePlugin.h"

 // Log Categories
DEFINE_LOG_CATEGORY(LogCriWareSoundXR);
DEFINE_LOG_CATEGORY(LogCriWareSoundXRDebug);

#define LOCTEXT_NAMESPACE "CriWareSoundXRModule"

// Forward Declarations
class FAtomRuntime;

// For creating our custom runtime plugin
class FSoundXRAtomRuntimePluginFactory
	: public IAtomRuntimePluginFactory
{
public:

	virtual FString GetDisplayName() override
	{
		static FString DisplayName = FString(TEXT("CriWare Sound xR"));
		return DisplayName;
	}

	virtual bool SupportsPlatform(const FString& PlatformName) override
	{
		return !PlatformName.StartsWith(TEXT("Linux")); // Linux and LinuxArm64 are not supported
	}

	virtual UClass* GetCustomRuntimeSettingsClass() const override { return nullptr; }

	virtual TAtomRuntimePluginPtr CreateNewRuntimePlugin(FAtomRuntime* OwningRuntime) override
	{
		return TAtomRuntimePluginPtr(new FSoundXRAtomRuntimePlugin(OwningRuntime));
	}
};

class FCriWareSoundXRModule
	: public ICriWareSoundXRModule
{
	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(FSoundXRAtomRuntimePluginFactory::GetModularFeatureName(), &SoundXRAtomRuntimePluginFactory);
	}

	virtual void ShutdownModule() override
	{
	}

public:

	// Plugins
	IAtomPluginFactory* GetPluginFactory(EAtomPlugin PluginType)
	{
		switch (PluginType)
		{
		case EAtomPlugin::Runtime:
			return &SoundXRAtomRuntimePluginFactory;
			break;
		default:
			return nullptr;
		}
	}

private:

	// Factories
	FSoundXRAtomRuntimePluginFactory SoundXRAtomRuntimePluginFactory;
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCriWareSoundXRModule, CriWareSoundXR)
