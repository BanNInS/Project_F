
#include "AtomRuntimeApple.h"

#include "CriWareApi.h"
#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRack.h"

#if defined(XPT_TGT_MACOSX)
#include "CriWareMacSettings.h"
#else
#include "CriWareIOSSettings.h"
#endif

/*
 * FAtomRuntimeApple Implementation
 *****************************************************************************/

FAtomRuntimeApple::FAtomRuntimeApple(ICriWarePlatformAtom* InPlatformAtom)
	: FAtomRuntime(InPlatformAtom)
{
}

FAtomRuntimeApple::~FAtomRuntimeApple()
{
}

void FAtomRuntimeApple::InitializePlatformAtomLibrary()
{
	auto Settings = GetDefault<UCriWareCoreSettings>();

#if defined(XPT_TGT_MACOSX)
	auto AppleSettings = GetDefault<UCriWareMacSettings>();
#else
	auto AppleSettings = GetDefault<UCriWareIOSSettings>();
#endif

	// overwrite settings
	// Global Standard Voices
	FAtomVoicesSettings VoicesSettings = Settings->VoicesSettings;
	VoicesSettings.OverridesWith(AppleSettings->VoicesOverrides);

	// Master Rack
	UAtomRack* MasterAtomRack = GetMasterRack();

	// AtomEx Config overwrites and limitations
	auto SetPlatformAtomConfig = [&](auto& AtomConfig)
	{
		// always out to Default
		CriAtomSoundRendererType DefaultOutRendererType = GetAtomExSoundRendererTypeForApple(EAtomSoundRendererType::Default);

		// select number of channels to output
		int AsrNumChannels = GetAtomExSoundRendererNumChannelsForApple(EAtomSoundRendererType::Default);
		int AsrSamplingRate = CRIATOM_DEFAULT_OUTPUT_SAMPLING_RATE;
		float AsrServerFrequency = 60.0f;

		// todo: move this to common
		if (MasterAtomRack)
		{
			AsrNumChannels = FMath::Min(AsrNumChannels, MasterAtomRack->NumChannels);
			AsrSamplingRate = MasterAtomRack->SamplingRate;
			AsrServerFrequency = MasterAtomRack->ServerFrequency;
		}

		// -- add platform specific overrides here --

		AtomConfig.asr.sound_renderer_type = DefaultOutRendererType;
		AtomConfig.asr.output_channels = AsrNumChannels;
		AtomConfig.asr.output_sampling_rate = AsrSamplingRate;
		AtomConfig.asr.server_frequency = AsrServerFrequency;

		// -- check platform config limitation here --
		// if (max voices is too much...)
	};

	// setup config
#if defined(XPT_TGT_IOS)
	CriAtomExConfig_IOS AtomExConfig;
	criAtomEx_SetDefaultConfig_IOS(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfig_IOS>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// initialize atom & filesystem
	criAtomEx_Initialize_IOS(&AtomExConfig, nullptr, 0);
#else
	CriAtomExConfig_MACOSX AtomExConfig;
	criAtomEx_SetDefaultConfig_MACOSX(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfig_MACOSX>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// initialize atom & filesystem
	FCriWareApi::criAtomEx_Initialize_MACOSX(&AtomExConfig, nullptr, 0);
#endif

	if (FCriWareApi::criAtomEx_IsInitialized() != CRI_TRUE)
	{
		return;
	}

	// Voice pools
	if (IsUsingFixedVoicePools())
	{
		CreateAtomVoicePool(VoicesSettings, EAtomVoicePoolCodecType::Standard);
	}

	// D-bas
	SetupAtomDBAS();
}

void FAtomRuntimeApple::FinalizePlatformAtomLibrary()
{
#if defined(XPT_TGT_IOS)
	// finalize atom
	criAtomEx_Finalize_IOS();
#else
	// finalize atom
	FCriWareApi::criAtomEx_Finalize_MACOSX();
#endif
}

CriAtomSoundRendererType FAtomRuntimeApple::GetAtomExSoundRendererTypeForApple(EAtomSoundRendererType RendererType)
{
	switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		// By default, output to hw1
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	case EAtomSoundRendererType::Main:
	case EAtomSoundRendererType::BGM:
	case EAtomSoundRendererType::Voice:
	case EAtomSoundRendererType::Pad:
	case EAtomSoundRendererType::User:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	case EAtomSoundRendererType::Haptic:
	//	return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_HAPTIC;
	case EAtomSoundRendererType::Aux:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	case EAtomSoundRendererType::Asr:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_ASR;
	case EAtomSoundRendererType::Muted:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_PSEUDO;
	default:
		checkNoEntry();
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	}
}

int32 FAtomRuntimeApple::GetAtomExSoundRendererNumChannelsForApple(EAtomSoundRendererType RendererType)
{
	/*switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		return 8;
	case EAtomSoundRendererType::Main:
		return 8;
	case EAtomSoundRendererType::BGM:
		return 8;
	case EAtomSoundRendererType::Voice:
		return 2;
	case EAtomSoundRendererType::Pad:
		return 1;
	case EAtomSoundRendererType::User:
		return 2;
	case EAtomSoundRendererType::Haptic:
		return 2;
	case EAtomSoundRendererType::Aux:
		return 8;
	case EAtomSoundRendererType::Muted:
		return 0;
	default:
		return 8;
	}*/

	// should get numchannel from hardware device
	return 6;
}

int32 FAtomRuntimeApple::GetAtomExSoundRendererNumPortsForApple(EAtomSoundRendererType RendererType)
{
	/*switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		return 4; // hardware 1~4
	case EAtomSoundRendererType::Main:
		return 1;
	case EAtomSoundRendererType::BGM:
		return 1;
	case EAtomSoundRendererType::Voice:
		return 4;
	case EAtomSoundRendererType::Pad:
		return 4;
	case EAtomSoundRendererType::User:
		return 4;
	case EAtomSoundRendererType::Haptic:
		return 4;
	case EAtomSoundRendererType::Aux:
		return 4; // hardware 1~4
	case EAtomSoundRendererType::Muted:
		return 0;
	default:
		return 1;
	}*/

	// only 1 port by device on Apple
	return 1;
}
