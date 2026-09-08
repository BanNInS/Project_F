/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Apple plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareApple module
 * File     : CriWareApple.cpp
 *
 ****************************************************************************/

#include "CriWareAtomApple.h"

#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"


DECLARE_LOG_CATEGORY_EXTERN(LogCriWareApple, Log, All);
DEFINE_LOG_CATEGORY(LogCriWareApple);

class FCriWareAppleModule
	: public IModuleInterface
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
			AtomApple = MakeShareable(new FCriWareAtomApple());
		}
		else
		{
			UE_LOG(LogCriWareApple, Error, TEXT("CriWare Plugin is not Enabled."));
		}
	}

	virtual void ShutdownModule() override
	{
		AtomApple.Reset();
	}

private:

	TSharedPtr<FCriWareAtomApple> AtomApple;
};

IMPLEMENT_MODULE(FCriWareAppleModule, CriWareApple)
