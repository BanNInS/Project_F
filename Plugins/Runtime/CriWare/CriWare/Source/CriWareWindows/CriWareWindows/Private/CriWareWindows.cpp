/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Windows plugin for Unreal Engine 4
 * Module   : IModuleInterface Class for CriWareWindows module
 * File     : CriWareWindows.cpp
 *
 ****************************************************************************/

#include "CriWareAtomWindows.h"

#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"


DECLARE_LOG_CATEGORY_EXTERN(LogCriWareWindows, Log, All);
DEFINE_LOG_CATEGORY(LogCriWareWindows);

class FCriWareWindowsModule : public IModuleInterface
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
			AtomWindows = MakeShareable(new FCriWareAtomWindows());
		}
		else
		{
			UE_LOG(LogCriWareWindows, Error, TEXT("CriWare Plugin is not Enabled."));
		}
	}

	virtual void ShutdownModule() override
	{
		AtomWindows.Reset();
	}

private:

	TSharedPtr<FCriWareAtomWindows> AtomWindows;
};

IMPLEMENT_MODULE(FCriWareWindowsModule, CriWareWindows)
