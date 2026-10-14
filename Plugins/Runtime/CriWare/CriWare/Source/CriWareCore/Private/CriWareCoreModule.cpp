/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareCoreModule.cpp
 *
 ****************************************************************************/

#include "CriWareCoreModule.h"

#include "CriWareCorePrivate.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "CriWareDefines.h"
#include "CriWareLLM.h"

#define UE_AUDIO_LINK_AVAILABLE (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1) 

#if UE_AUDIO_LINK_AVAILABLE
#include "Misc/ConfigContext.h"
#include "CriWareAudioLinkModule.h"
#endif

DEFINE_LOG_CATEGORY(LogCriWareCore);

class FCriWareCoreModule : public ICriWareCoreModule
{
public:
	
	virtual void StartupModule() override
	{
#if CRIWARECORE_USE_LLM
		LLM(CriWareLLM::Initialize());
#endif

		CriWareCore = MakeShareable(new FCriWareCore());

#if UE_AUDIO_LINK_AVAILABLE
		// Load AudioLink module
		FString CriWareIni;
		FConfigContext::ReadIntoGConfig().Load(TEXT("CriWare"), CriWareIni);

		bool bUseAudioLink = false;
		GConfig->GetBool(TEXT("/Script/CriWareCore.CriWareCoreSettings"), TEXT("bUseAudioLink"), bUseAudioLink, CriWareIni);
		if (bUseAudioLink)
		{
			UE_LOG(LogCriWareCore, Log, TEXT("FCriWareAudioLinkModule startup"));
			CriWareAudioLinkModule = MakeUnique<FCriWareAudioLinkModule>();
		}
#endif
    }

    virtual void ShutdownModule() override
	{
#if UE_AUDIO_LINK_AVAILABLE
		CriWareAudioLinkModule.Reset();
#endif

		CriWareCore.Reset();
	}

	//~ ICriWareCoreModule Interface

	virtual TSharedPtr<FCriWareCore> GetCriWareCore() override
	{
		return CriWareCore;
	}

	virtual void RegisterCreateAtomSoundBaseRenderer(const FOnCreateAtomSoundBaseRenderer& Delegate) override
	{
		CreateAtomSoundBaseRendererDelegate = Delegate;
	}

	virtual void UnregisterCreateAtomSoundBaseRenderer() override
	{
		CreateAtomSoundBaseRendererDelegate.Unbind();
	}

	virtual UObject* CreateAtomSoundBaseRenderer() override
	{
		UObject* Renderer = nullptr;
		if (CreateAtomSoundBaseRendererDelegate.IsBound())
		{
			Renderer = CreateAtomSoundBaseRendererDelegate.Execute();
		}
		return Renderer;
	}

private:

	TSharedPtr<FCriWareCore> CriWareCore;

	/** Delegate to create an object that implements IAtomSoundBaseRendererInterface. */
	FOnCreateAtomSoundBaseRenderer CreateAtomSoundBaseRendererDelegate;

#if UE_AUDIO_LINK_AVAILABLE
	TUniquePtr<FCriWareAudioLinkModule> CriWareAudioLinkModule;
#endif
};

IMPLEMENT_MODULE(FCriWareCoreModule, CriWareCore);
