/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Android plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareAndroid module
 * File     : CriWareAndroid.cpp
 *
 ****************************************************************************/

#include "CriWareAtomAndroid.h"

#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"


DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAndroid, Log, All);
DEFINE_LOG_CATEGORY(LogCriWareAndroid);

class FCriWareAndroidModule
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
			AtomAndroid = MakeShareable(new FCriWareAtomAndroid());
		}
		else
		{
			UE_LOG(LogCriWareAndroid, Error, TEXT("CriWare Plugin is not Enabled."));
		}
	}

	virtual void ShutdownModule() override
	{
		AtomAndroid.Reset();
	}

private:

	TSharedPtr<FCriWareAtomAndroid> AtomAndroid;
};

IMPLEMENT_MODULE(FCriWareAndroidModule, CriWareAndroid)
