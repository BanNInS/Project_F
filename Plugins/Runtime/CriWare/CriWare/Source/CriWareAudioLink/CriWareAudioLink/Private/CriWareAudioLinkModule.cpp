/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLInk plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareAudioLink module
 * File     : CriWareAudioLinkModule.cpp
 *
 ****************************************************************************/

#include "CriWareAudioLinkModule.h"

//#include "Engine/Engine.h"
//#include "Modules/ModuleManager.h"
//#include "Interfaces/IPluginManager.h"
//#include "Misc/ConfigContext.h"
//#include "Templates/UniquePtr.h"

#include "AtomAudioLinkFactory.h"

DEFINE_LOG_CATEGORY(LogCriWareAudioLink);

/*class FCriWareAudioLinkModule
	: public IModuleInterface
{
	// IModuleInterface implementation
	virtual void StartupModule() override
	{
		// Check if CriWare runtime is enabled
		//bool IsCriWareRuntimeEnabled = false;
		//auto AllPlugins = IPluginManager::Get().GetEnabledPlugins();
		//for (auto& Plugin : AllPlugins)
		//{
		//	if (Plugin->GetName() == "CriWare")
		//	{
		//		IsCriWareRuntimeEnabled = true;
		//	}
		//}

		bool IsCriWareRuntimeEnabled = true;
		if (IsCriWareRuntimeEnabled)
		{
			FConfigContext::ReadIntoGConfig().Load(TEXT("CriWare"), CriWareIni);

			bool bUseAudioLink = false;
			GConfig->GetBool(TEXT("/Script/CriWareCore.CriWareCoreSettings"), TEXT("bUseAudioLink"), bUseAudioLink, CriWareIni);
			if (bUseAudioLink)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				AtomAudioLinkFactory = MakeUnique<FAtomAudioLinkFactory>();
#else
				UE_LOG(LogCriWareAudioLink, Error, TEXT("AudioLink is not available in Unreal versions prior to 5.1. Ingored."));
#endif
			}
		}
		else
		{
			UE_LOG(LogCriWareAudioLink, Error, TEXT("CriWare Plugin is not Enabled."));
		}
	}

	virtual void ShutdownModule() override
	{
		AtomAudioLinkFactory.Reset();
	}

private:

	TUniquePtr<FAtomAudioLinkFactory> AtomAudioLinkFactory;

	FString CriWareIni;
};

IMPLEMENT_MODULE(FCriWareAudioLinkModule, CriWareAudioLink)
*/