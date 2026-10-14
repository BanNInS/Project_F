#include "AtomRuntimeWindows.h"

#include "CriWareApi.h"
#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "CriWareWindowsSettings.h"
#include "Atom/AtomRack.h"

#include "CriWareWindowsSettings.h"

#include "Windows/WindowsHWrapper.h"

THIRD_PARTY_INCLUDES_START
#if CRIWARE_UE_LE
#include <cri_le_file_system.h>
#include <cri_le_atom_wasapi.h>
#else
#include <cri_file_system.h>
#include <cri_atom_wasapi.h>
#endif
THIRD_PARTY_INCLUDES_END

/*
 * FAtomRuntimeWindows Implementation
 *****************************************************************************/

CriAtomSoundRendererType FAtomRuntimeWindows::EndpointMappings[(uint32)EAtomSoundRendererType::Num];

FAtomRuntimeWindows::FAtomRuntimeWindows(ICriWarePlatformAtom* InPlatformAtom)
	: FAtomRuntime(InPlatformAtom)
{
}

FAtomRuntimeWindows::~FAtomRuntimeWindows()
{
}

void FAtomRuntimeWindows::InitializePlatformAtomLibrary()
{
	// enumerate wasapi devices
	Wasapi = MakeUnique<FAtomWasapi>();

	auto Settings = GetDefault<UCriWareCoreSettings>();
	auto WindowsSettings = GetDefault<UCriWareWindowsSettings>();

	// overwrite settings
	// Global Standard Voices
	FAtomVoicesSettings VoicesSettings = Settings->VoicesSettings;
	VoicesSettings.OverridesWith(WindowsSettings->VoicesOverrides);

	// Master Rack
	UAtomRack* MasterAtomRack = GetMasterRack();

	// AtomEx Config overwrites and limitations
	auto SetPlatformAtomConfig = [&](auto& AtomConfig)
	{
		// always out to Default
		CriAtomSoundRendererType DefaultOutRendererType = GetAtomExSoundRendererTypeForWindows(EAtomSoundRendererType::Default);
	
		// select number of channels to output
		int AsrNumChannels = GetAtomExSoundRendererNumChannelsForWindows(EAtomSoundRendererType::Default);
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

	// translate settings to local type - todo: store config as int not fname
	for (int Index = 0; Index < (int)EAtomSoundRendererType::Num; Index++)
	{
		FName HardwareName = WindowsSettings->EndpointMappings[Index];

		if (HardwareName == FName(TEXT("Hardware Device 1")))
		{
			EndpointMappings[Index] = CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_HW1;
		}
		else if (HardwareName == FName(TEXT("Hardware Device 2")))
		{
			EndpointMappings[Index] = CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_HW2;
		}
		else if (HardwareName == FName(TEXT("Hardware Device 3")))
		{
			EndpointMappings[Index] = CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_HW3;
		}
		else if (HardwareName == FName(TEXT("Hardware Device 4")))
		{
			EndpointMappings[Index] = CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_HW4;
		}
	}

	// setup config
	CriAtomExConfig_WASAPI AtomExConfig;
	criAtomEx_SetDefaultConfig_WASAPI(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfig_WASAPI>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// Disable Spatial Audio and use WASAPI instead, by hw - Atom by default uses Spatial Audio (aka Microsoft Spatial Sound).
	Wasapi->SetWasapiSpatialAudio(CRIATOM_SOUND_RENDERER_HW1, !WindowsSettings->WASAPISettings.bEnableHardware1);
	Wasapi->SetWasapiSpatialAudio(CRIATOM_SOUND_RENDERER_HW2, !WindowsSettings->WASAPISettings.bEnableHardware2);
	Wasapi->SetWasapiSpatialAudio(CRIATOM_SOUND_RENDERER_HW3, !WindowsSettings->WASAPISettings.bEnableHardware3);
	Wasapi->SetWasapiSpatialAudio(CRIATOM_SOUND_RENDERER_HW4, !WindowsSettings->WASAPISettings.bEnableHardware4);

	// initialize atom
	FCriWareApi::criAtomEx_Initialize_WASAPI(&AtomExConfig, nullptr, 0);
	if (FCriWareApi::criAtomEx_IsInitialized() != CRI_TRUE)
	{
		return;
	}

	// Wasapi exclusive - (not supported if 2 hardwares uses same device)
	if ((WindowsSettings->WASAPISettings.bEnableHardware1
		|| WindowsSettings->WASAPISettings.bEnableHardware2
		|| WindowsSettings->WASAPISettings.bEnableHardware3
		|| WindowsSettings->WASAPISettings.bEnableHardware4)
		&& WindowsSettings->WASAPISettings.bWASAPI_IsExclusive)
	{
		WAVEFORMATEXTENSIBLE ExclusiveFormat = {};
		WAVEFORMATEX* MixerFormat = (WAVEFORMATEX*)&ExclusiveFormat;
		MixerFormat->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		MixerFormat->nChannels = (uint16)WindowsSettings->WASAPISettings.WASAPI_NumChannels;
		MixerFormat->nSamplesPerSec = (uint32)WindowsSettings->WASAPISettings.WASAPI_SamplingRate;
		MixerFormat->wBitsPerSample = ((WindowsSettings->WASAPISettings.WASAPI_BitsPerSample > 16) ? 32 : 16);
		MixerFormat->nBlockAlign = MixerFormat->wBitsPerSample / 8 * MixerFormat->nChannels;
		MixerFormat->nAvgBytesPerSec = MixerFormat->nSamplesPerSec * MixerFormat->nBlockAlign;
		MixerFormat->cbSize = 22;
		ExclusiveFormat.Samples.wValidBitsPerSample = WindowsSettings->WASAPISettings.WASAPI_BitsPerSample;
		ExclusiveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		FCriWareApi::criAtom_SetAudioClientFormat_WASAPI(MixerFormat);
		FCriWareApi::criAtom_SetAudioClientShareMode_WASAPI(AUDCLNT_SHAREMODE_EXCLUSIVE);
	}

	// Wasapi device assignations
	auto SetEndpoint = [&](CriAtomSoundRendererType Type, const FName& DeviceName)
	{
		const FName* DeviceId = Wasapi->WasapiEndpoints.FindKey(DeviceName);
		if (DeviceId && !(*DeviceId).IsNone())
		{
			Wasapi->SetWasapiEndpoint(Type, *DeviceId);
		}
		else
		{
			// reset to default
			Wasapi->UnsetWasapiEndpoint(Type);
		}
	};

	SetEndpoint(CRIATOM_SOUND_RENDERER_HW1, WindowsSettings->Hardware1);
	SetEndpoint(CRIATOM_SOUND_RENDERER_HW2, WindowsSettings->Hardware2);
	SetEndpoint(CRIATOM_SOUND_RENDERER_HW3, WindowsSettings->Hardware3);
	SetEndpoint(CRIATOM_SOUND_RENDERER_HW4, WindowsSettings->Hardware4);
	
	// Voice pools
	if (IsUsingFixedVoicePools())
	{
		CreateAtomVoicePool(VoicesSettings, EAtomVoicePoolCodecType::Standard);
	}
	
	// D-bas
	SetupAtomDBAS();
}

void FAtomRuntimeWindows::FinalizePlatformAtomLibrary()
{
	// Reset WASAPI Audio Client settings (criAtomEx_Finalize doesn't do it.)
	FCriWareApi::criAtom_SetAudioClientShareMode_WASAPI(AUDCLNT_SHAREMODE_SHARED);

	// finalize atom
	FCriWareApi::criAtomEx_Finalize_WASAPI();
}

FCriAtomExAsrRackId FAtomRuntimeWindows::LoadPlatformAtomRack(const FAtomRackInitParams& InitParams)
{
	CriAtomSoundRendererType OutRendererType = InitParams.RendererType;

	if (InitParams.EndpointSettings)
	{
		// get settings overrides if present
		const FAtomWindowsEndpointSettings& EndpointSettings = Atom::DowncastProxyRef<const FAtomWindowsEndpointSettings>(*InitParams.EndpointSettings);
		if (EndpointSettings.bOverrideHardwareDevice)
		{
			OutRendererType = (CriAtomSoundRendererType)(((int32)EndpointSettings.HardwareDeviceOverrides << 16) | CRIATOM_SOUND_RENDERER_NATIVE);
		}

		// Set output as haptic device if requested
		if (EndpointSettings.bSendToVibrationOutput)
		{
			wchar_t DeviceID[512] = {};
			CriBool OutIsDefaultDevice = CRI_FALSE;

			FCriWareApi::criAtom_GetDeviceId_WASAPI(OutRendererType, DeviceID, 512, &OutIsDefaultDevice);
			FCriWareApi::criAtom_SetHapticDeviceId_WASAPI(DeviceID, CRIATOM_HAPTIC_DEVICE_TYPE_DLS);
		}
	}

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

bool FAtomRuntimeWindows::SetPlatformAtomDSPBusOutput(CriAtomExAsrRackId RackID, const FString& BusName, const FAtomBusOutputParams& OutputParams)
{
	if (OutputParams.EndpointSettings)
	{
		const FAtomWindowsEndpointSettings& EndpointSettings = Atom::DowncastProxyRef<const FAtomWindowsEndpointSettings>(*OutputParams.EndpointSettings);
		if (EndpointSettings.bSendToVibrationOutput)
		{
			wchar_t DeviceID[512] = {};
			CriBool OutIsDefaultDevice = CRI_FALSE;

			// not supported actully 
			CriAtomSoundRendererType OutRendererType = CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_ANY;
			if (EndpointSettings.bOverrideHardwareDevice)
			{
				OutRendererType = (CriAtomSoundRendererType)(((int32)EndpointSettings.HardwareDeviceOverrides << 16) | CRIATOM_SOUND_RENDERER_NATIVE);
			}

			FCriWareApi::criAtom_GetDeviceId_WASAPI(OutRendererType, DeviceID, 512, &OutIsDefaultDevice);
			FCriWareApi::criAtom_SetHapticDeviceId_WASAPI(DeviceID, CRIATOM_HAPTIC_DEVICE_TYPE_DLS);
		}
	}

	return true;
}

CriAtomSoundRendererType FAtomRuntimeWindows::GetAtomExSoundRendererTypeForWindows(EAtomSoundRendererType RendererType)
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
	case EAtomSoundRendererType::Haptic:
	case EAtomSoundRendererType::Aux:
		// the device to output is decided by settings
		return EndpointMappings[(size_t)RendererType];
	case EAtomSoundRendererType::Asr:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_ASR;
	case EAtomSoundRendererType::Muted:
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_PSEUDO;
	default:
		checkNoEntry();
		return CriAtomSoundRendererType::CRIATOM_SOUND_RENDERER_NATIVE;
	}
}

int32 FAtomRuntimeWindows::GetAtomExSoundRendererNumChannelsForWindows(EAtomSoundRendererType RendererType)
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
		return 8; // For compatibilty with Audio + HD haptic devices.
	case EAtomSoundRendererType::Aux:
		return 12;
	case EAtomSoundRendererType::Muted:
		return 0;
	default:
		return 8;
	}

	// should get numchannel from hardware device
}

int32 FAtomRuntimeWindows::GetAtomExSoundRendererNumPortsForWindows(EAtomSoundRendererType RendererType)
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

	// only 1 port by device on windows
	return 1;
}

TMap<FName, FName>& FAtomRuntimeWindows::GetAvailableAudioDevices(bool bRefresh /* = false */) const
{	
	if (bRefresh)
	{
		Wasapi->RefreshWasapiEndpoints();
	}
	return Wasapi->WasapiEndpoints;
}

bool FAtomRuntimeWindows::AssignAudioDeviceToEndpoint(int HardwareId, const FName& DeviceId)
{
	return Wasapi->SetWasapiEndpoint((CriAtomSoundRendererType)((HardwareId << 2) | CRIATOM_SOUND_RENDERER_NATIVE), DeviceId);
}
