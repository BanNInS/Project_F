
#include "AtomRuntimeAndroid.h"

#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"

#include "CriWareApi.h"
#include "CriWare.h"
#include "CriWareCore.h"
#include "CriWareCoreSettings.h"
#include "CriWareAndroidSettings.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomThread.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomAndroid, Log, All);
DEFINE_LOG_CATEGORY(LogCriWareAtomAndroid);

/*
 * FAtomRuntimeAndroid Implementation
 *****************************************************************************/

FAtomRuntimeAndroid::FAtomRuntimeAndroid(ICriWarePlatformAtom* InPlatformAtom)
	: FAtomRuntime(InPlatformAtom)
{
	FCriWareApi::criAtom_SetJavaVM_ANDROID(GJavaVM);

	jobject Context = FAndroidApplication::GetGameActivityThis();
	CriBool ApplyingSucceeded = FCriWareApi::criAtom_ApplyHardwareProperties_ANDROID(GJavaVM, Context);
	if (ApplyingSucceeded == CRI_TRUE)
	{
		UE_LOG(LogCriWareAtomAndroid, Display, TEXT("Java VM and Activity context setup to Atom."));
	}
	else
	{
		UE_LOG(LogCriWareAtomAndroid, Error, TEXT("Failed to setup Java VM to Atom."));
	}
}

FAtomRuntimeAndroid::~FAtomRuntimeAndroid()
{
}

void FAtomRuntimeAndroid::SuspendContext()
{
	FAtomCommandFence Fence;
	Fence.BeginFence();
	Fence.Wait();

	FScopeLock ScopeLock(&SuspendedCriticalSection);

	if (!bSuspended)
	{
		UE_LOG(LogCriWareAtomAndroid, Display, TEXT("Suspending android Atom sound renderer"));

		FCriWareApi::criAtomEx_StopSound_ANDROID();

		bSuspended = true;
	}
}

void FAtomRuntimeAndroid::ResumeContext()
{
	FScopeLock ScopeLock(&SuspendedCriticalSection);

	if (bSuspended)
	{
		UE_LOG(LogCriWareAtomAndroid, Display, TEXT("Resuming android Atom sound renderer"));

		FCriWareApi::criAtomEx_StartSound_ANDROID();

		bSuspended = false;
	}
}

void FAtomRuntimeAndroid::InitializePlatformAtomLibrary()
{
	auto Settings = GetDefault<UCriWareCoreSettings>();
	auto AndroidSettings = GetDefault<UCriWareAndroidSettings>();

	// overwrite settings
	// Global Standard Voices
	FAtomVoicesSettings VoicesSettings = Settings->VoicesSettings;
	VoicesSettings.OverridesWith(AndroidSettings->VoicesOverrides);

	// Master Rack
	UAtomRack* MasterAtomRack = GetMasterRack();

	// AtomEx Config overwrites and limitations
	auto SetPlatformAtomConfig = [&](auto& AtomConfig)
	{
		// always out to Default
		CriAtomSoundRendererType DefaultOutRendererType = GetAtomExSoundRendererTypeForAndroid(EAtomSoundRendererType::Default);

		// select number of channels to output
		int AsrNumChannels = GetAtomExSoundRendererNumChannelsForAndroid(EAtomSoundRendererType::Default);
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
	CriAtomExConfig_ANDROID AtomExConfig;
	criAtomEx_SetDefaultConfig_ANDROID(&AtomExConfig);
	CriFsConfig FileSystemConfig;
	criFs_SetDefaultConfig(&FileSystemConfig);
	FCriWarePlatformAtomConfig<CriAtomExConfig_ANDROID>::SetCommonAtomConfig(AtomExConfig, FileSystemConfig, ManaRuntimeInterface);
	SetPlatformAtomConfig(AtomExConfig);

	// initialize atom & filesystem
	criAtomEx_Initialize_ANDROID(&AtomExConfig, nullptr, 0);
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

void FAtomRuntimeAndroid::FinalizePlatformAtomLibrary()
{
	// finalize atom
	criAtomEx_Finalize_ANDROID();
}

CriAtomSoundRendererType FAtomRuntimeAndroid::GetAtomExSoundRendererTypeForAndroid(EAtomSoundRendererType RendererType)
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

int32 FAtomRuntimeAndroid::GetAtomExSoundRendererNumChannelsForAndroid(EAtomSoundRendererType RendererType)
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

int32 FAtomRuntimeAndroid::GetAtomExSoundRendererNumPortsForAndroid(EAtomSoundRendererType RendererType)
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

	// only 1 port by device on Android
	return 1;
}
