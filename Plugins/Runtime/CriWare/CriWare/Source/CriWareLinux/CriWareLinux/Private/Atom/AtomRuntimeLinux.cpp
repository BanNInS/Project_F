
#include "AtomRuntimeLinux.h"

#include "CriWareApi.h"
#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "CriWareLinuxSettings.h"
#include "Atom/AtomRack.h"

/*
 * FAtomRuntimeLinux Implementation
 *****************************************************************************/

FAtomRuntimeLinux::FAtomRuntimeLinux(ICriWarePlatformAtom* InPlatformAtom)
	: FAtomRuntime(InPlatformAtom)
{
}

FAtomRuntimeLinux::~FAtomRuntimeLinux()
{
}

void FAtomRuntimeLinux::InitializePlatformAtomLibrary()
{
	auto Settings = GetDefault<UCriWareCoreSettings>();
	auto LinuxSettings = GetDefault<UCriWareLinuxSettings>();

	// overwrite settings
	// Global Standard Voices
	FAtomVoicesSettings VoicesSettings = Settings->VoicesSettings;
	VoicesSettings.OverridesWith(LinuxSettings->VoicesOverrides);

	// Master Rack
	UAtomRack* MasterAtomRack = GetMasterRack();

	// AtomEx Config overwrites and limitations
	auto SetPlatformAtomConfig = [&](auto& AtomConfig)
	{
		// always out to Default
		CriAtomSoundRendererType DefaultOutRendererType = GetAtomExSoundRendererTypeForLinux(EAtomSoundRendererType::Default);
	
		// select number of channels to output
		int AsrNumChannels = GetAtomExSoundRendererNumChannelsForLinux(EAtomSoundRendererType::Default);
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
	CriAtomExConfig_PULSE AtomExConfig;
	criAtomEx_SetDefaultConfig_PULSE(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfig_PULSE>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// initialize atom
	FCriWareApi::criAtomEx_Initialize_PULSE(&AtomExConfig, nullptr, 0);
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

void FAtomRuntimeLinux::FinalizePlatformAtomLibrary()
{
	auto Settings = GetDefault<UCriWareCoreSettings>();

	// finalize atom
	FCriWareApi::criAtomEx_Finalize_PULSE();
}

CriAtomSoundRendererType FAtomRuntimeLinux::GetAtomExSoundRendererTypeForLinux(EAtomSoundRendererType RendererType)
{
	switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	case EAtomSoundRendererType::Main:
	case EAtomSoundRendererType::BGM:
	case EAtomSoundRendererType::Voice:
	case EAtomSoundRendererType::Pad:
	case EAtomSoundRendererType::User:
	case EAtomSoundRendererType::Haptic:
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

int32 FAtomRuntimeLinux::GetAtomExSoundRendererNumChannelsForLinux(EAtomSoundRendererType RendererType)
{
	switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		return 12;
	case EAtomSoundRendererType::Main:
		return 12;
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
		return 12;
	case EAtomSoundRendererType::Muted:
		return 0;
	default:
		return 8;
	}

	// should get numchannel from hardware device
	return 8;
}

int32 FAtomRuntimeLinux::GetAtomExSoundRendererNumPortsForLinux(EAtomSoundRendererType RendererType)
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

	// only 1 port by device on Linux
	return 1;
}
