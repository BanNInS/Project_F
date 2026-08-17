/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Linux plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareLinux module
 * File     : CriWareLinux.cpp
 *
 ****************************************************************************/

#include "CriWareAtomLinux.h"

#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"


DECLARE_LOG_CATEGORY_EXTERN(LogCriWareLinux, Log, All);
DEFINE_LOG_CATEGORY(LogCriWareLinux);

class FCriWareLinuxModule : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		// Check if CriWare runtime is enabled
		bool IsCriWareRuntimeEnabled = false;
		auto AllPlugins = IPluginManager::Get().GetEnabledPlugins();
		for (auto& Plugin : AllPlugins)
		{
			if (Plugin->GetName() == "CriWare")
			{
				IsCriWareRuntimeEnabled = true;
			}
		}

		if (IsCriWareRuntimeEnabled)
		{
			AtomLinux = MakeShareable(new FCriWareAtomLinux());
		}
		else
		{
			UE_LOG(LogCriWareLinux, Error, TEXT("CriWare Plugin is not Enabled."));
		}
	}

	virtual void ShutdownModule() override
	{
		AtomLinux.Reset();
	}

private:

	TSharedPtr<FCriWareAtomLinux> AtomLinux;
};

IMPLEMENT_MODULE(FCriWareLinuxModule, CriWareLinux)
