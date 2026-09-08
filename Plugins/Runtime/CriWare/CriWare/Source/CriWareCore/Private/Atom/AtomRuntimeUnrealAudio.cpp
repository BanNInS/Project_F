#include "AtomRuntimeUnrealAudio.h"

#include "CriWareApi.h"
#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRack.h"


/*
 * FAtomRuntimeUnrealAudio Implementation
 *****************************************************************************/

FAtomRuntimeUnrealAudio::FAtomRuntimeUnrealAudio(ICriWarePlatformAtom* InPlatformAtom)
	: FAtomRuntime(InPlatformAtom)
{
}

FAtomRuntimeUnrealAudio::~FAtomRuntimeUnrealAudio()
{
}

void FAtomRuntimeUnrealAudio::InitializePlatformAtomLibrary()
{
	auto Settings = GetDefault<UCriWareCoreSettings>();

	// Standard Voices
	FAtomVoicesSettings VoicesSettings = Settings->VoicesSettings;

	// Master Rack
	UAtomRack* MasterAtomRack = GetMasterRack();

	// AtomEx Config overwrites and limitations
	auto SetPlatformAtomConfig = [&](auto& AtomConfig)
	{
		// always out to Default
		CriAtomSoundRendererType DefaultOutRendererType = GetAtomExSoundRendererTypeForUnrealAudio(EAtomSoundRendererType::Default);
	
		// select number of channels to output
		int AsrNumChannels = GetAtomExSoundRendererNumChannelsForUnrealAudio(EAtomSoundRendererType::Default);
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
	CriAtomExConfigForUserPcmOutput AtomExConfig;
	criAtomEx_SetDefaultConfigForUserPcmOutput(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfigForUserPcmOutput>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// initialize atom
	FCriWareApi::criAtomEx_InitializeForUserPcmOutput(&AtomExConfig, nullptr, 0);

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

void FAtomRuntimeUnrealAudio::FinalizePlatformAtomLibrary()
{
	// finalize atom
	FCriWareApi::criAtomEx_FinalizeForUserPcmOutput();
}

FCriAtomExAsrRackId FAtomRuntimeUnrealAudio::LoadPlatformAtomRack(const FAtomRackInitParams& InitParams)
{
	CriAtomSoundRendererType OutRendererType = InitParams.RendererType;

	// setup rack
	CriAtomExAsrRackConfig asr_rack_config;
	criAtomExAsrRack_SetDefaultConfig(&asr_rack_config);
	asr_rack_config.sound_renderer_type = OutRendererType;
	asr_rack_config.server_frequency = InitParams.ServerFrequency;
	asr_rack_config.num_buses = CRIATOMEXASR_MAX_BUSES;
	asr_rack_config.speaker_mapping = InitParams.SpeakerMapping;
	asr_rack_config.output_channels = FMath::Max(2, InitParams.NumChannels); // a bug in atom makes crash with mono.
	asr_rack_config.output_sampling_rate = InitParams.SamplingRate;
	asr_rack_config.output_rack_id = InitParams.OutputRackId;

	return MakeCriId<FCriAtomExAsrRackId>(FCriWareApi::criAtomExAsrRack_Create(&asr_rack_config, nullptr, 0));
}

CriAtomSoundRendererType FAtomRuntimeUnrealAudio::GetAtomExSoundRendererTypeForUnrealAudio(EAtomSoundRendererType RendererType)
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

int32 FAtomRuntimeUnrealAudio::GetAtomExSoundRendererNumChannelsForUnrealAudio(EAtomSoundRendererType RendererType)
{
	switch (RendererType)
	{
	case EAtomSoundRendererType::Default:
		return 16;
	case EAtomSoundRendererType::Main:
		return 16;
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
		return 16;
	case EAtomSoundRendererType::Muted:
		return 0;
	default:
		return 8;
	}
}

int32 FAtomRuntimeUnrealAudio::GetAtomExSoundRendererNumPortsForUnrealAudio(EAtomSoundRendererType RendererType)
{
	return 1;
}
