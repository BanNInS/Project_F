
#include "Atom/Mixer/AtomMixerSourceManager.h"

#include "ProfilingDebugging/CsvProfiler.h"
#include "Async/Async.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "Stats/Stats.h"
#include "Trace/Trace.h"
#include "DSP/FloatArrayMath.h"

#include "CriWareDefines.h"
#include "CriWareLLM.h"
#include "Atom/Atom.h"
#include "Atom/AtomThread.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomSourceVoiceEffect.h"
#include "Atom/Mixer/AtomMixerTrace.h"
#include "Atom/Mixer/AtomMixerSourceVoice.h"
#include "Atom/Modulation/AtomModulationDestination.h"
#include "Extensions/IAtomExtensionPlugin.h"

#include "Atom/Mixer/AtomMixerSource.h"

DEFINE_LOG_CATEGORY(LogCriWareAtomMixer);
DEFINE_LOG_CATEGORY(LogCriWareAtomMixerDebug);

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomMixerSourceManager)

#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
	#define ATOM_MIXER_THREAD_COMMAND_STRING(X) (X)
#else
	#define ATOM_MIXER_THREAD_COMMAND_STRING(X) ("")
#endif

#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_CHANNEL_DEFINE(AtomChannel);
UE_TRACE_CHANNEL_DEFINE(AtomMixerChannel);
#endif

// Link to "Audio" profiling category
CSV_DECLARE_CATEGORY_MODULE_EXTERN(CRIWARECORE_API, Atom);
/*static int32 AtomDisableParallelSourceProcessingCvar = 1;
FAutoConsoleVariableRef CVarAtomDisableParallelSourceProcessing(
	TEXT("atom.DisableParallelSourceProcessing"),
	AtomDisableParallelSourceProcessingCvar,
	TEXT("Disables using async tasks for processing sources.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);*/

/*static int32 AtomDisableFilteringCvar = 0;
FAutoConsoleVariableRef CVarDisableFiltering(
	TEXT("au.DisableFiltering"),
	DisableFilteringCvar,
	TEXT("Disables using the per-source lowpass and highpass filter.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);*/

/*static int32 AtomDisableHPFilteringCvar = 0;
FAutoConsoleVariableRef CVarDisableHPFiltering(
	TEXT("au.DisableHPFiltering"),
	DisableHPFilteringCvar,
	TEXT("Disables using the per-source highpass filter.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);*/

static int32 AtomDisableEnvelopeFollowingCvar = 0;
FAutoConsoleVariableRef CVarAtomDisableEnvelopeFollowing(
	TEXT("atom.DisableEnvelopeFollowing"),
	AtomDisableEnvelopeFollowingCvar,
	TEXT("Disables using the envlope follower for source envelope tracking.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);

static int32 AtomDisableSourceEffectsCvar = 0;
FAutoConsoleVariableRef CVarAtomDisableSourceEffects(
	TEXT("atom.DisableSourceEffects"),
	AtomDisableSourceEffectsCvar,
	TEXT("Disables using any source effects.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);

static int32 AtomDisableDistanceAttenuationCvar = 0;
FAutoConsoleVariableRef CVarAtomDisableDistanceAttenuation(
	TEXT("atom.DisableDistanceAttenuation"),
	AtomDisableDistanceAttenuationCvar,
	TEXT("Disables using any Distance Attenuation.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);

/*static int32 AtomBypassAudioPluginsCvar = 0;
FAutoConsoleVariableRef CVarAtomBypassAudioPlugins(
	TEXT("atom.BypassAudioPlugins"),
	AtomBypassAudioPluginsCvar,
	TEXT("Bypasses any audio plugin processing.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);*/

static int32 AtomFlushCommandBufferOnTimeoutCvar = 0;
	FAutoConsoleVariableRef CVarAtomFlushCommandBufferOnTimeout(
		TEXT("atom.FlushCommandBufferOnTimeout"),
		AtomFlushCommandBufferOnTimeoutCvar,
		TEXT("When set to 1, flushes audio render thread synchronously when our fence has timed out.\n")
		TEXT("0: Not Disabled, 1: Disabled"),
		ECVF_Default);

static int32 AtomCommandBufferFlushWaitTimeMsCvar = 1000;
FAutoConsoleVariableRef CVarAtomCommandBufferFlushWaitTimeMs(
	TEXT("atom.CommandBufferFlushWaitTimeMs"),
	AtomCommandBufferFlushWaitTimeMsCvar,
	TEXT("How long to wait for the command buffer flush to complete.\n"),
	ECVF_Default);

static int32 AtomCommandBufferMaxSizeInMbCvar = 10;
FAutoConsoleVariableRef CVarAtomCommandBufferMaxSizeMb(
	TEXT("atom.CommandBufferMaxSizeInMb"),
	AtomCommandBufferMaxSizeInMbCvar,
	TEXT("How big to allow the command buffer to grow before ignoring more commands"),
	ECVF_Default);

static int32 AtomCommandBufferInitialCapacityCvar = 500;
FAutoConsoleVariableRef CVarAtomCommandBufferInitialCapacity(
	TEXT("atom.CommandBufferInitialCapacity"),
	AtomCommandBufferInitialCapacityCvar,
	TEXT("How many elements to initialize the command buffer capacity with"),
	ECVF_Default);

/*static float AtomAudioCommandExecTimeMsWarningThresholdCvar = 500.f;
FAutoConsoleVariableRef CVarAtomAudioCommandExecTimeMsWarningThreshold(
	TEXT("atom.AudioThreadCommand.ExecutionTimeWarningThresholdInMs"),
	AtomAudioCommandExecTimeMsWarningThresholdCvar,
	TEXT("If a command took longer to execute than this number (in milliseconds) then we log a warning"),
	ECVF_Default);*/

// -> use atom monitor
/*static int32 AtomLogEveryAudioThreadCommandCvar = 0;
FAutoConsoleVariableRef AtomLogEveryAudioThreadCommand(
	TEXT("atom.AudioThreadCommand.LogEveryExecution"),
	AtomLogEveryAudioThreadCommandCvar,
	TEXT("Extremely verbose logging of each Audio Thread command caller and it's execution time"),
	ECVF_Default);*/

/*static int32 LogCmdQueueWhenNumberReachedCVar = 0;
FAutoConsoleVariableRef CVarLogCmdQueueWhenNumberReached(
	TEXT("au.debug.LogCmdQueueWhenReached"),
	LogCmdQueueWhenNumberReachedCVar,
	TEXT("When number of commands in queue is reached, it will dump the command list to the log."),
	ECVF_Cheat);*/

static int32 AtomNumCmdsConsideredFullCVar = 1000;
FAutoConsoleVariableRef CVarAtomNumCmdsConsideredFullCVar(
	TEXT("atom.NumCmdsConsideredFullCVar"),
	AtomNumCmdsConsideredFullCVar,
	TEXT("Num of commands in the queue is considered full"),
	ECVF_Default);

static float AtomFloatCompareMinScaleCVar = 0.001;
FAutoConsoleVariableRef CVarAtomFloatCompareMinScale(
	TEXT("atom.FloatCompareMinScale"),
	AtomFloatCompareMinScaleCVar,
	TEXT("Minimum parameter float compare tolerance"),
	ECVF_Default);

static float AtomFloatCompareMaxScaleCVar = 0.1;
FAutoConsoleVariableRef CVarAtomFloatCompareMaxScale(
	TEXT("atom.FloatCompareMaxScale"),
	AtomFloatCompareMaxScaleCVar,
	TEXT("Maximum parameter float compare tolerance"),
	ECVF_Default);

// +/- 4 Octaves (default)
static float AtomMaxModulationPitchRangeFreqCVar = 16.0f;
static float AtomMinModulationPitchRangeFreqCVar = 0.0625f;
static FAutoConsoleCommand GAtomModulationSetMaxPitchRange(
	TEXT("atom.Modulation.SetPitchRange"),
	TEXT("Sets max final modulation range of pitch (in semitones). Default: 96 semitones (+/- 4 octaves)"),
	FConsoleCommandWithArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogCriWareAtom, Error, TEXT("Failed to set max modulation pitch range: Range not provided"));
				return;
			}

			const float Range = FCString::Atof(*Args[0]);
			AtomMaxModulationPitchRangeFreqCVar = Atom::GetFrequencyMultiplier(Range * 0.5f);
			AtomMaxModulationPitchRangeFreqCVar = Atom::GetFrequencyMultiplier(Range * -0.5f);
		}
	)
);

#define ENVELOPE_TAIL_THRESHOLD (1.58489e-5f) // -96 dB

#define VALIDATE_SOURCE_MIXER_STATE 1

#if ATOM_MIXER_ENABLE_DEBUG_MODE

// Macro which checks if the source id is in debug mode, avoids having a bunch of #ifdefs in code
#define ATOM_MIXER_DEBUG_LOG(SourceId, Format, ...)																							\
	if (SourceInfos[SourceId].bIsDebugMode)																													\
	{																																			\
		FString CustomMessage = FString::Printf(Format, ##__VA_ARGS__);																			\
		FString LogMessage = FString::Printf(TEXT("<Debug Sound Log> [Id=%d][Name=%s]: %s"), SourceId, *SourceInfos[SourceId].DebugName, *CustomMessage);	\
		UE_LOG(LogCriWareAtomMixer, Log, TEXT("%s"), *LogMessage);																								\
	}

#else

#define ATOM_MIXER_DEBUG_LOG(SourceId, Message)

#endif

// Disable subframe timing logic
#define ATOM_SUBFRAME_ENABLED 0

// Define profiling for source manager. 
//DEFINE_STAT(STAT_AtomMixerHRTF);
//DEFINE_STAT(STAT_AtomMixerSourceBuffers);
//DEFINE_STAT(STAT_AtomMixerSourceEffectBuffers);
DEFINE_STAT(STAT_AtomMixerSourceManagerUpdate);
//DEFINE_STAT(STAT_AtomMixerSourceOutputBuffers);


#if ATOM_PROFILERTRACE_ENABLED
// Audio Bus messages
UE_TRACE_EVENT_BEGIN(CriWareAtom, AudioBusStart)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, AudioBusID)
UE_TRACE_EVENT_FIELD(uint64, Timestamp)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_FIELD(int32, NumChannels)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, AudioBusStop)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, AudioBusID)
UE_TRACE_EVENT_FIELD(uint64, Timestamp)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, AudioBusHasActivity)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, AudioBusID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(bool, HasActivity)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, AudioBusEnvelopeFollowerEnabled)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, AudioBusID)
UE_TRACE_EVENT_FIELD(uint64, Timestamp)
UE_TRACE_EVENT_FIELD(bool, EnvelopeFollowerEnabled)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, AudioBusEnvelopeValues)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, AudioBusID)
UE_TRACE_EVENT_FIELD(uint64, Timestamp)
UE_TRACE_EVENT_FIELD(float[], EnvelopeValues)
UE_TRACE_EVENT_END()
#endif // ATOM_PROFILERTRACE_ENABLED*/

#ifndef CASE_ENUM_TO_TEXT
	#define CASE_ENUM_TO_TEXT(TXT) case TXT: return TEXT(#TXT);
#endif

const TCHAR* LexToString(EAtomSourceManagerRenderThreadPhase InPhase)
{
	switch (InPhase)
	{
		FOREACH_ENUM_EATOMSOURCEMANAGERRENDERTHREADPHASE(CASE_ENUM_TO_TEXT)
	}
	return TEXT("Unknown");
}

namespace Atom
{
	int32 GetCommandBufferInitialCapacity()
	{
		return FMath::Clamp(AtomCommandBufferInitialCapacityCvar, 0, 10000);
	}

	bool IsAudioBufferSilent(const float* AudioBuffer, const int32 NumSamples)
	{
		bool bIsSilent = true;

		int32 Index = 0;

#if PLATFORM_ENABLE_VECTORINTRINSICS
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		const int32 SimdNum = NumSamples & 0xFFFFFFF0;
		for (; Index < SimdNum; Index += 16)
		{
			const VectorRegister4x4Float Samples = VectorLoad16(&AudioBuffer[Index]);

			if (VectorAnyGreaterThan(VectorAbs(Samples.val[0]), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples.val[1]), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples.val[2]), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples.val[3]), GlobalVectorConstants::SmallNumber))
			{
				bIsSilent = false;
				Index = INT_MAX;
				break;
			}
		}
#else
		const int32 SimdNum = NumSamples & 0xFFFFFFF0;
		for (; Index < SimdNum; Index += 16)
		{
			const VectorRegister4Float Samples0 = VectorLoad(&AudioBuffer[Index]);
			const VectorRegister4Float Samples1 = VectorLoad(&AudioBuffer[Index + 4]);
			const VectorRegister4Float Samples2 = VectorLoad(&AudioBuffer[Index + 8]);
			const VectorRegister4Float Samples3 = VectorLoad(&AudioBuffer[Index + 12]);

			if (VectorAnyGreaterThan(VectorAbs(Samples0), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples1), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples2), GlobalVectorConstants::SmallNumber) ||
				VectorAnyGreaterThan(VectorAbs(Samples3), GlobalVectorConstants::SmallNumber))
			{
				bIsSilent = false;
				Index = INT_MAX;
				break;
			}
		}
#endif
#endif
		// Finish to the end of the buffer or check each sample if vector intrinsics are disabled
		for (; Index < NumSamples; ++Index)
		{
			// As soon as we hit a non-silent sample, we're not silent
			if (FMath::Abs(AudioBuffer[Index]) > SMALL_NUMBER)
			{
				bIsSilent = false;
				break;
			}
		}

		return bIsSilent;
	}

#if ATOM_PROFILERTRACE_ENABLED
	void TraceAudioBusStartMessage(const uint32 InRuntimeID, const uint32 InObjectID, const FString& InAudioBusName, const int32 InNumChannels)
	{
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
		if (bChannelEnabled)
		{
			UE_TRACE_LOG(CriWareAtom, AudioBusStart, AtomChannel)
				<< AudioBusStart.RuntimeID(InRuntimeID)
				<< AudioBusStart.AudioBusID(InObjectID)
				<< AudioBusStart.Timestamp(FPlatformTime::Cycles64())
				<< AudioBusStart.Name(InAudioBusName.IsEmpty() ? *FString::Printf(TEXT("%d"), InObjectID) : *InAudioBusName)
				<< AudioBusStart.NumChannels(InNumChannels);
		}
	}

	void TraceAudioBusStopMessage(const uint32 InRuntimeID, const uint32 InObjectID)
	{
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
		if (bChannelEnabled)
		{
			UE_TRACE_LOG(CriWareAtom, AudioBusStop, AtomChannel)
				<< AudioBusStop.RuntimeID(InRuntimeID)
				<< AudioBusStop.AudioBusID(InObjectID)
				<< AudioBusStop.Timestamp(FPlatformTime::Cycles64());
		}
	}
#endif // ATOM_PROFILERTRACE_ENABLED

	/*************************************************************************
	* FAtomSourceManager
	**************************************************************************/

	FMixerSourceManager::FMixerSourceManager(FAtomRuntime* InAtomRuntime)
		: AtomRuntime(InAtomRuntime)
		, NumActiveSources(0)
		, NumTotalSources(0)
		, NumOutputFrames(0)
		, NumOutputSamples(0)
		//, NumSourceWorkers(4)
		, bInitialized(false)
		//, bUsingSpatializationPlugin(false)
		, bUsingSourceDataOverridePlugin(false)
	{
		// Get a manual resetable event
		const bool bIsManualReset = true;
		CommandsProcessedEvent = FPlatformProcess::GetSynchEventFromPool(bIsManualReset);
		check(CommandsProcessedEvent != nullptr);

		// Immediately trigger the command processed in case a flush happens before the audio thread swaps command buffers
		CommandsProcessedEvent->Trigger();
	}

	FMixerSourceManager::~FMixerSourceManager()
	{
#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.RemoveAll(this);
		UnregisterAudioBusEnvelopeFollowerCVars();
#endif // ATOM_PROFILERTRACE_ENABLED

		FPlatformProcess::ReturnSynchEventToPool(CommandsProcessedEvent);
	}

#if ATOM_PROFILERTRACE_ENABLED
	void FMixerSourceManager::OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination)
	{
		for (const auto& [AudioBusKey, AudioBus] : AudioBuses)
		{
			if (AudioBus.IsValid())
			{
				TraceAudioBusStartMessage(AtomRuntime->GetAtomRuntimeID(), AudioBusKey.ObjectId, AudioBus->BusName, AudioBus->GetNumChannels());
			}
		}
	}

	void FMixerSourceManager::RegisterAudioBusEnvelopeFollowerCVars()
	{
		// StartAudioBusEnvelopeFollowerCVar
		StartAudioBusEnvelopeFollowerCVar = IConsoleManager::Get().RegisterConsoleVariable(
			*FString::Printf(TEXT("atom.StartAudioBusEnvelopeFollower_AD%d"), AtomRuntime->GetAtomRuntimeID()),
			FString(),
			TEXT("Starts an audio bus envelope follower (args= audio bus id)\n"),
			ECVF_Default
		);

		if (StartAudioBusEnvelopeFollowerCVar)
		{
			StartAudioBusEnvelopeFollowerCVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FMixerSourceManager::OnStartAudioBusEnvelopeFollowerCVarChanged));
		}

		// StopAudioBusEnvelopeFollowerCVar
		StopAudioBusEnvelopeFollowerCVar = IConsoleManager::Get().RegisterConsoleVariable(
			*FString::Printf(TEXT("atom.StopAudioBusEnvelopeFollower_AD%d"), AtomRuntime->GetAtomRuntimeID()),
			FString(),
			TEXT("Stops an audio bus envelope follower (args= audio bus id)\n"),
			ECVF_Default
		);

		if (StopAudioBusEnvelopeFollowerCVar)
		{
			StopAudioBusEnvelopeFollowerCVar->SetOnChangedCallback(FConsoleVariableDelegate::CreateRaw(this, &FMixerSourceManager::OnStopAudioBusEnvelopeFollowerCVarChanged));
		}
	}

	void FMixerSourceManager::UnregisterAudioBusEnvelopeFollowerCVars()
	{
		IConsoleManager::Get().UnregisterConsoleObject(StartAudioBusEnvelopeFollowerCVar);
		IConsoleManager::Get().UnregisterConsoleObject(StopAudioBusEnvelopeFollowerCVar);
	}

	void FMixerSourceManager::OnStartAudioBusEnvelopeFollowerCVarChanged(IConsoleVariable* InStartAudioBusEnvelopeFollowerCVar)
	{
		ensure(IsInGameThread());

		if (InStartAudioBusEnvelopeFollowerCVar)
		{
			uint32 AudioBusId = INDEX_NONE;
			LexFromString(AudioBusId, *InStartAudioBusEnvelopeFollowerCVar->GetString());

			FAtomThread::RunCommandOnAtomThread([this, AudioBusId]()
			{
				StartAudioBusEnvelopeFollowerFromCVar(AudioBusId);
			});
		}
	}

	void FMixerSourceManager::OnStopAudioBusEnvelopeFollowerCVarChanged(IConsoleVariable* InStopAudioBusEnvelopeFollowerCVar)
	{
		ensure(IsInGameThread());

		if (InStopAudioBusEnvelopeFollowerCVar)
		{
			uint32 AudioBusId = INDEX_NONE;
			LexFromString(AudioBusId, *InStopAudioBusEnvelopeFollowerCVar->GetString());

			FAtomThread::RunCommandOnAtomThread([this, AudioBusId]()
			{
				StopAudioBusEnvelopeFollowerFromCVar(AudioBusId);
			});
		}
	}

	void FMixerSourceManager::StartAudioBusEnvelopeFollowerFromCVar(const uint32 InAudioBusId)
	{
		ensure(IsInAudioThread());

		constexpr float AttackTime = 10.0f;
		constexpr float ReleaseTime = 500.0f;

		for (auto& [AudioBusKey, AudioBus] : AudioBuses)
		{
			if (AudioBusKey.ObjectId == InAudioBusId && AudioBus.IsValid())
			{
				AudioBus->StartEnvelopeFollower(AttackTime, ReleaseTime);

				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
				if (bChannelEnabled && AudioBus->bIsEnvelopeFollowing)
				{
					UE_TRACE_LOG(CriWareAtom, AudioBusEnvelopeFollowerEnabled, AtomChannel)
						<< AudioBusEnvelopeFollowerEnabled.RuntimeID(AtomRuntime->GetAtomRuntimeID())
						<< AudioBusEnvelopeFollowerEnabled.AudioBusID(AudioBusKey.ObjectId)
						<< AudioBusEnvelopeFollowerEnabled.Timestamp(FPlatformTime::Cycles64())
						<< AudioBusEnvelopeFollowerEnabled.EnvelopeFollowerEnabled(true);
				}

				break;
			}
		}
	}

	void FMixerSourceManager::StopAudioBusEnvelopeFollowerFromCVar(const uint32 InAudioBusId)
	{
		ensure(IsInAudioThread());

		for (auto& [AudioBusKey, AudioBus] : AudioBuses)
		{
			if (AudioBusKey.ObjectId == InAudioBusId && AudioBus.IsValid())
			{
				AudioBus->StopEnvelopeFollower();

				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
				if (bChannelEnabled && !AudioBus->bIsEnvelopeFollowing)
				{
					UE_TRACE_LOG(CriWareAtom, AudioBusEnvelopeFollowerEnabled, AtomChannel)
						<< AudioBusEnvelopeFollowerEnabled.RuntimeID(AtomRuntime->GetAtomRuntimeID())
						<< AudioBusEnvelopeFollowerEnabled.AudioBusID(AudioBusKey.ObjectId)
						<< AudioBusEnvelopeFollowerEnabled.Timestamp(FPlatformTime::Cycles64())
						<< AudioBusEnvelopeFollowerEnabled.EnvelopeFollowerEnabled(false);
				}

				break;
			}
		}
	}
#endif // ATOM_PROFILERTRACE_ENABLED

	void FMixerSourceManager::Init(const FSourceManagerInitParams& InitParams)
	{
		ATOM_MIXER_CHECK(InitParams.NumSources > 0);

		if (bInitialized || !AtomRuntime)
		{
			return;
		}

		//ATOM_MIXER_CHECK(AtomRuntime->GetSampleRate() > 0);

		NumTotalSources = InitParams.NumSources;
		NumOutputFrames = AtomRuntime->GetRuntimeNumOutputFrames(); // use the sound's rack !!!
		NumOutputSamples = NumOutputFrames * AtomRuntime->GetRuntimeNumOutputChannels(); // use the sound's rack !!!

		MixerSources.Init(nullptr, NumTotalSources); // atom voices

		// Populate output sources array with default data
		/*SourceSubmixOutputBuffers.Reset();
		for (int32 Index = 0; Index < NumTotalSources; Index++)
		{
			SourceSubmixOutputBuffers.Emplace(AtomRuntime, 2, MixerDevice->GetNumDeviceChannels(), NumOutputFrames);
		}*/

		SourceInfos.AddDefaulted(NumTotalSources);

		for (int32 i = 0; i < NumTotalSources; ++i)
		{
			FSourceInfo& SourceInfo = SourceInfos[i];

			SourceInfo.MixerSourcePlayer = {};

			//SourceInfo.VolumeSourceStart = -1.0f;
			//SourceInfo.VolumeSourceDestination = -1.0f;
			//SourceInfo.VolumeFadeSlope = 0.0f;
			//SourceInfo.VolumeFadeStart = 0.0f;
			//SourceInfo.VolumeFadeFramePosition = 0;
			//SourceInfo.VolumeFadeNumFrames = 0;

			//SourceInfo.DistanceAttenuationSourceStart = -1.0f;
			//SourceInfo.DistanceAttenuationSourceDestination = -1.0f;

			//SourceInfo.LowPassFreq = MAX_FILTER_FREQUENCY;
			//SourceInfo.HighPassFreq = MIN_FILTER_FREQUENCY;

			SourceInfo.SourceListener = nullptr;
			SourceInfo.CurrentPCMBuffer = nullptr;
			SourceInfo.CurrentAudioChunkNumFrames = 0;
			//SourceInfo.CurrentFrameAlpha = 0.0f;
			//SourceInfo.CurrentFrameIndex = 0;
			SourceInfo.NumFramesPlayed = 0;
			//SourceInfo.StartTime = 0.0;
			//SourceInfo.SubmixSends.Reset();
			SourceInfo.AudioBusID = INDEX_NONE;
			SourceInfo.SourceBusDurationFrames = INDEX_NONE;
			SourceInfo.SourceBusPatch = nullptr;
		
			SourceInfo.AudioBusSends[(int32)EAtomBusSendStage::PreEffect].Reset();
			SourceInfo.AudioBusSends[(int32)EAtomBusSendStage::PostEffect].Reset();

			//SourceInfo.SourceEffectChainId = INDEX_NONE;

			Audio::FInlineEnvelopeFollowerInitParams EnvelopeFollowerInitParams;
			EnvelopeFollowerInitParams.SampleRate = AtomRuntime->GetRuntimeSampleRate();
			EnvelopeFollowerInitParams.AttackTimeMsec = 10.f;
			EnvelopeFollowerInitParams.ReleaseTimeMsec = 100.f;
			EnvelopeFollowerInitParams.Mode = Audio::EPeakMode::Peak;
			SourceInfo.SourceEnvelopeFollower = Audio::FInlineEnvelopeFollower(EnvelopeFollowerInitParams);

			SourceInfo.SourceEnvelopeValue = 0.0f;
			//SourceInfo.bEffectTailsDone = false;
		
			SourceInfo.ResetModulators(AtomRuntime->GetAtomRuntimeID());

			SourceInfo.bIs3D = false;
			SourceInfo.bIsCenterChannelOnly = false;
			SourceInfo.bIsActive = false;
			SourceInfo.bIsPlaying = false;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsPausedForQuantization = false;
			//SourceInfo.bDelayLineSet = false;
			SourceInfo.bIsStopping = false;
			SourceInfo.bIsDone = false;
			//SourceInfo.bIsLastBuffer = false;
			//SourceInfo.bIsBusy = false;
			SourceInfo.bUseHRTFSpatializer = false;
			//SourceInfo.bUseOcclusionPlugin = false;
			//SourceInfo.bUseReverbPlugin = false;
			SourceInfo.bHasStarted = false;
			SourceInfo.bEnableBusSends = false;
			//SourceInfo.bEnableBaseSubmix = false;
			//SourceInfo.bEnableSubmixSends = false;
			//SourceInfo.bIsVorbis = false;
			//SourceInfo.bHasPreDistanceAttenuationSend = false;
			SourceInfo.bModFiltersUpdated = false;

#if ATOM_MIXER_ENABLE_DEBUG_MODE
			SourceInfo.bIsDebugMode = false;
#endif // ATOM_MIXER_ENABLE_DEBUG_MODE

			SourceInfo.NumInputChannels = 0;
			//SourceInfo.NumPostEffectChannels = 0;
			SourceInfo.NumInputFrames = 0;
		}
		
		GameThreadInfo.bIsBusy.AddDefaulted(NumTotalSources);
		GameThreadInfo.bNeedsSpeakerMap.AddDefaulted(NumTotalSources);
		GameThreadInfo.bIsDebugMode.AddDefaulted(NumTotalSources);
		GameThreadInfo.bIsUsingHRTFSpatializer.AddDefaulted(NumTotalSources);
		GameThreadInfo.FreeSourceIndices.Reset(NumTotalSources);
		for (int32 Index = NumTotalSources - 1; Index >= 0; --Index)
		{
			GameThreadInfo.FreeSourceIndices.Add(Index);
		}

#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		// Initialize the source buffer memory usage to max source scratch buffers (num frames times max source channels)
		for (int32 SourceID = 0; SourceID < NumTotalSources; ++SourceID)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceID];

			//SourceInfo.SourceBuffer.Reset(NumOutputFrames * ATOM_MAX_DSP_CHANNELS);
			//SourceInfo.PreDistanceAttenuationBuffer.Reset(NumOutputFrames * ATOM_MAX_DSP_CHANNELS);
			//SourceInfo.SourceEffectScratchBuffer.Reset(NumOutputFrames * ATOM_MAX_DSP_CHANNELS);
			//SourceInfo.AudioPluginOutputData.AudioBuffer.Reset(NumOutputFrames * ATOM_MAX_DSP_CHANNELS);

			SourceInfo.SendsBuffer.SetCapacity((NumOutputFrames + 1) * ATOM_MAX_DSP_CHANNELS);
		}
#endif

		// Setup the source workers
		/*SourceWorkers.Reset();
		if (NumSourceWorkers > 0)
		{
			const int32 NumSourcesPerWorker = FMath::Max(NumTotalSources / NumSourceWorkers, 1);
			int32 StartId = 0;
			int32 EndId = 0;
			while (EndId < NumTotalSources)
			{
				EndId = FMath::Min(StartId + NumSourcesPerWorker, NumTotalSources);
				SourceWorkers.Add(new FAsyncTask<FAudioMixerSourceWorker>(this, StartId, EndId));
				StartId = EndId;
			}
		}
		NumSourceWorkers = SourceWorkers.Num();*/

		// Cache the spatialization plugin
		/*bUsingSpatializationPlugin = false;
		SpatialInterfaceInfo = MixerDevice->GetCurrentSpatializationPluginInterfaceInfo();
		const auto& SpatializationPlugin = SpatialInterfaceInfo.SpatializationPlugin;
		if (SpatialInterfaceInfo.SpatializationPlugin.IsValid())
		{
			bUsingSpatializationPlugin = true;
		}*/
		// Cache the source data override plugin
		SourceDataOverridePlugin = AtomRuntime->SourceDataOverridePluginInterface;
		if (SourceDataOverridePlugin.IsValid())
		{
			bUsingSourceDataOverridePlugin = true;
		}

		// Spam command queue with nops.
		/*static FAutoConsoleCommand SpamNopsCmd(
			TEXT("atom.AudioThreadCommand.SpamCommandQueue"),
			TEXT(""),
			FConsoleCommandDelegate::CreateLambda([this]() 
			{				
				struct FSpamPayload
				{
					uint8 JunkBytes[1024];
				} Payload;
				for (int32 i = 0; i < 65536; ++i)
				{
					AtomMixerThreadCommand([Payload] {});
				}
			})
		);*/


		// submit a command that has an endless loop
		/*static FAutoConsoleCommand SpamEndlessCmd(
			TEXT("atom.AudioThreadCommand.ChokeCommandQueue"),
			TEXT(""),
			FConsoleCommandDelegate::CreateLambda([this]()
			{
				struct FSpamPayload
				{
					uint8 JunkBytes[1024];
				} Payload;

				AtomMixerThreadCommand([Payload] {while(true){}});
			})
		);*/

		bInitialized = true;
		//bPumpQueue = false;
	}

	void FMixerSourceManager::Update(bool bTimedOut)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

#if VALIDATE_SOURCE_MIXER_STATE
		for (int32 Index = 0; Index < NumTotalSources; ++Index)
		{
			if (!GameThreadInfo.bIsBusy[Index])
			{
				// Make sure that our bIsFree and FreeSourceIndices are correct
				ATOM_MIXER_CHECK(GameThreadInfo.FreeSourceIndices.Contains(Index) == true);
			}
		}
#endif

		if (FPlatformProcess::SupportsMultithreading())
		{
			// If the command was triggered, then we want to do a swap of command buffers
			if (CommandsProcessedEvent->Wait(0))
			{
				int32 CurrentGameIndex = !RenderThreadCommandBufferIndex.GetValue();

				// This flags the audio render thread to be able to pump the next batch of commands
				// And will allow the audio thread to write to a new command slot
				const int32 NextIndex = (CurrentGameIndex + 1) & 1;

				FCommands& NextCommandBuffer = CommandBuffers[NextIndex];

				// Make sure we've actually emptied the command queue from the render thread before writing to it
				if (AtomFlushCommandBufferOnTimeoutCvar && NextCommandBuffer.SourceCommandQueue.Num() != 0)
				{
					UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Audio render callback stopped. Flushing %d commands."), NextCommandBuffer.SourceCommandQueue.Num());

					// Pop and execute all the commands that came since last update tick
					for (int32 ID = 0; ID < NextCommandBuffer.SourceCommandQueue.Num(); ++ID)
					{
						FAtomMixerThreadCommand AudioCommand = NextCommandBuffer.SourceCommandQueue[ID];

						AudioCommand();
						NumCommands.Decrement();
					}

					NextCommandBuffer.SourceCommandQueue.Reset();
				}

				// Here we ensure that we block for any pending calls to AtomMixerThreadCommand.
				FScopeLock ScopeLock(&CommandBufferIndexCriticalSection);
				RenderThreadCommandBufferIndex.Set(CurrentGameIndex);

				CommandsProcessedEvent->Reset();
			}
		}
		else
		{
			int32 CurrentRenderIndex = RenderThreadCommandBufferIndex.GetValue();
			int32 CurrentGameIndex = !RenderThreadCommandBufferIndex.GetValue();
			check(CurrentGameIndex == 0 || CurrentGameIndex == 1);
			check(CurrentRenderIndex == 0 || CurrentRenderIndex == 1);

			// If these values are the same, that means the audio render thread has finished the last buffer queue so is ready for the next block
			if (CurrentRenderIndex == CurrentGameIndex)
			{
				// This flags the audio render thread to be able to pump the next batch of commands
				// And will allow the audio thread to write to a new command slot
				const int32 NextIndex = !CurrentGameIndex;

				// Make sure we've actually emptied the command queue from the render thread before writing to it
				if (CommandBuffers[NextIndex].SourceCommandQueue.Num() != 0)
				{
					UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Source command queue not empty: %d"), CommandBuffers[NextIndex].SourceCommandQueue.Num());
				}
				bPumpQueue = true;
			}
		}
	}

	void FMixerSourceManager::ReleaseSource(const int32 SourceID)
	{
		//ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(AtomRuntime);

		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(bInitialized);

		if (MixerSources[SourceID] == nullptr)
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Ignoring double release of SourceId: %i"), SourceID);
			return;
		}

		ATOM_MIXER_DEBUG_LOG(SourceID, TEXT("Is releasing"));

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

#if ATOM_MIXER_ENABLE_DEBUG_MODE
		if (SourceInfo.bIsDebugMode)
		{
			//DebugSoloSources.Remove(SourceID);
		}
#endif
		// Remove from list of active bus or source ids depending on what type of source this is
		if (SourceInfo.AudioBusID != INDEX_NONE)
		{
			// Remove this bus from the registry of bus instances
			TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(SourceInfo.AudioBusID);
			if (AudioBusPtr.IsValid())
			{
				// If this audio bus was automatically created via source bus playback, this this audio bus can be removed
				if (AudioBusPtr->RemoveInstanceID(SourceID, SourceInfo.MixerSourcePlayer.PlaybackID))
				{
#if ATOM_PROFILERTRACE_ENABLED
					TraceAudioBusStopMessage(AtomRuntime->GetAtomRuntimeID(), SourceInfo.AudioBusID);
#endif // ATOM_PROFILERTRACE_ENABLED

					// Only automatic buses will be getting removed here. Otherwise they need to be manually removed from the source manager.
					ensure(AudioBusPtr->IsAutomatic());
					AudioBuses.Remove(SourceInfo.AudioBusID);
				}
			}
		}

		// Remove this source's send list from the bus data registry
		for (int32 AudioBusSendStage = 0; AudioBusSendStage < (int32)EAtomBusSendStage::Count; ++AudioBusSendStage)
		{
			for (uint32 AudioBusID : SourceInfo.AudioBusSends[AudioBusSendStage])
			{
				// we should have a bus registration entry still since the send hasn't been cleaned up yet
				TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(AudioBusID);
				if (AudioBusPtr.IsValid())
				{
					if (AudioBusPtr->RemoveSend((EAtomBusSendStage)AudioBusSendStage, SourceID))
					{
#if ATOM_PROFILERTRACE_ENABLED
						TraceAudioBusStopMessage(AtomRuntime->GetAtomRuntimeID(), AudioBusID);
#endif // ATOM_PROFILERTRACE_ENABLED

						ensure(AudioBusPtr->IsAutomatic());
						AudioBuses.Remove(AudioBusID);
					}
				}
			}

			SourceInfo.AudioBusSends[AudioBusSendStage].Reset();
		}

		SourceInfo.AudioBusID = INDEX_NONE;
		SourceInfo.SourceBusDurationFrames = INDEX_NONE;
		SourceInfo.SourceBusPatch = nullptr;

		// Free the mixer source buffer data
		if (SourceInfo.MixerSourcePlayer.Player.IsValid())
		{
			//PendingSourcePlayers.Add(SourceInfo.MixerSourcePlayer);
			SourceInfo.MixerSourcePlayer.Player = nullptr;
		}
		SourceInfo.MixerSourcePlayer.PlaybackID = INDEX_NONE;
		SourceInfo.SourceListener = nullptr;

		// CRI: ExPlayer -> setrack/bus sends
		// Remove the mixer source from its submix sends
		/*for (FMixerSourceSubmixSend& SubmixSendItem : SourceInfo.SubmixSends)
		{
			FMixerSubmixPtr SubmixPtr = SubmixSendItem.Submix.Pin();
			if (SubmixPtr.IsValid())
			{
				SubmixPtr->RemoveSourceVoice(MixerSources[SourceId]);
			}
		}
		SourceInfo.SubmixSends.Reset();*/

		// CRI: ExPlayer -> ?? how enable HRTF effect ? 
		// Notify plugin effects
		/*if (SourceInfo.bUseHRTFSpatializer)
		{
			ATOM_MIXER_CHECK(bUsingSpatializationPlugin);
			LLM_SCOPE(ELLMTag::AudioMixerPlugins);
			SpatialInterfaceInfo.SpatializationPlugin->OnReleaseSource(SourceId);
		}*/

		/*if (SourceInfo.bUseOcclusionPlugin)
		{
			MixerDevice->OcclusionInterface->OnReleaseSource(SourceId);
		}*/

		/*if (SourceInfo.bUseReverbPlugin)
		{
			MixerDevice->ReverbPluginInterface->OnReleaseSource(SourceId);
		}*/

		/*if (SourceInfo.AudioLink)
		{
			SourceInfo.AudioLink->OnSourceReleased(SourceId);
			SourceInfo.AudioLink.Reset();
		}*/

		// Delete the source effects
		/*SourceInfo.SourceEffectChainId = INDEX_NONE;
		ResetSourceEffectChain(SourceId);*/

		SourceInfo.SourceEnvelopeFollower.Reset();
		//SourceInfo.bEffectTailsDone = true;

		// Release the source voice back to the mixer device. This is pooled.

		// actally we need to release the resource in Atom too
		// todo: reuse created voices in atom.
		MixerSources[SourceID]->VoicePool = nullptr;

		AtomRuntime->ReleaseMixerSourceVoice(MixerSources[SourceID]);
		MixerSources[SourceID] = nullptr;

		// Reset all state and data
		//SourceInfo.PitchSourceParam.Init();
		//SourceInfo.VolumeSourceStart = -1.0f;
		//SourceInfo.VolumeSourceDestination = -1.0f;
		//SourceInfo.VolumeFadeSlope = 0.0f;
		//SourceInfo.VolumeFadeStart = 0.0f;
		//SourceInfo.VolumeFadeFramePosition = 0;
		//SourceInfo.VolumeFadeNumFrames = 0;

		//SourceInfo.DistanceAttenuationSourceStart = -1.0f;
		//SourceInfo.DistanceAttenuationSourceDestination = -1.0f;

		//SourceInfo.LowPassFreq = MAX_FILTER_FREQUENCY;
		//SourceInfo.HighPassFreq = MIN_FILTER_FREQUENCY;

		//if (SourceInfo.SourceBufferListener)
		//{
		//	SourceInfo.SourceBufferListener->OnSourceReleased(SourceId);
		//	SourceInfo.SourceBufferListener.Reset();
		//}

		SourceInfo.ResetModulators(AtomRuntime->GetAtomRuntimeID());

		//SourceInfo.LowPassFilter.Reset();
		//SourceInfo.HighPassFilter.Reset();
		SourceInfo.CurrentPCMBuffer = nullptr;
		SourceInfo.CurrentAudioChunkNumFrames = 0;
		//SourceInfo.SourceBuffer.Reset();
		//SourceInfo.PreDistanceAttenuationBuffer.Reset();
		//SourceInfo.SourceEffectScratchBuffer.Reset();
		//SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
		//SourceInfo.CurrentFrameValues.Reset();
		//SourceInfo.NextFrameValues.Reset();
		//SourceInfo.CurrentFrameAlpha = 0.0f;
		//SourceInfo.CurrentFrameIndex = 0;
		SourceInfo.NumFramesPlayed = 0;
		//SourceInfo.StartTime = 0.0;
		SourceInfo.bIs3D = false;
		SourceInfo.bIsCenterChannelOnly = false;
		SourceInfo.bIsActive = false;
		SourceInfo.bIsPlaying = false;
		SourceInfo.bIsDone = true;
		//SourceInfo.bIsLastBuffer = false;
		SourceInfo.bIsPaused = false;
		SourceInfo.bIsPausedForQuantization = false;
		//SourceInfo.bDelayLineSet = false;
		SourceInfo.bIsStopping = false;
		//SourceInfo.bIsBusy = false;
		SourceInfo.bUseHRTFSpatializer = false;
		SourceInfo.bIsExternalSend = false;
		//SourceInfo.bUseOcclusionPlugin = false;
		//SourceInfo.bUseReverbPlugin = false;
		SourceInfo.bHasStarted = false;
		SourceInfo.bEnableBusSends = false;
		//SourceInfo.bEnableBaseSubmix = false;
		//SourceInfo.bEnableSubmixSends = false;
		SourceInfo.bIsVorbis = false;
		//SourceInfo.bHasPreDistanceAttenuationSend = false;
		SourceInfo.bModFiltersUpdated = false;

		SourceInfo.AtomComponentID = 0;
		SourceInfo.PlayOrder = INDEX_NONE;
		SourceInfo.ActiveSoundPlayOrder = INDEX_NONE;

		SourceInfo.QuantizedCommandHandle.Reset();

#if ATOM_MIXER_ENABLE_DEBUG_MODE
		SourceInfo.bIsDebugMode = false;
		SourceInfo.DebugName = FString();
#endif //ATOM_MIXER_ENABLE_DEBUG_MODE

		SourceInfo.NumInputChannels = 0;
		//SourceInfo.NumPostEffectChannels = 0;
		SourceInfo.NumInputFrames = 0;

		GameThreadInfo.bNeedsSpeakerMap[SourceID] = false;
	}

	/*void FAtomSourceManager::BuildSourceEffectChain(const int32 SourceId, FSoundEffectSourceInitData& InitData, const TArray<FSourceEffectChainEntry>& InSourceEffectChain, TArray<TSoundEffectSourcePtr>& OutSourceEffects)
	{
		// Create new source effects. The memory will be owned by the source manager.
		FScopeLock ScopeLock(&EffectChainMutationCriticalSection);
		for (const FSourceEffectChainEntry& ChainEntry : InSourceEffectChain)
		{
			// Presets can have null entries
			if (!ChainEntry.Preset)
			{
				continue;
			}

			// Get this source effect presets unique id so instances can identify their originating preset object
			const uint32 PresetUniqueId = ChainEntry.Preset->GetUniqueID();
			InitData.ParentPresetUniqueId = PresetUniqueId;

			TSoundEffectSourcePtr NewEffect = USoundEffectPreset::CreateInstance<FSoundEffectSourceInitData, FSoundEffectSource>(InitData, *ChainEntry.Preset);
			NewEffect->SetEnabled(!ChainEntry.bBypass);

			OutSourceEffects.Add(NewEffect);
		}
	}*/

	/*void FAtomSourceManager::ResetSourceEffectChain(const int32 SourceId)
	{
		FScopeLock ScopeLock(&EffectChainMutationCriticalSection);
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Unregister these source effect instances from their owning USoundEffectInstance on the audio thread.
			// Have to pass to Game Thread prior to processing on AudioThread to avoid race condition with GC.
			// (RunCommandOnAudioThread is not safe to call from any thread other than the GameThread).
			if (!SourceInfo.SourceEffects.IsEmpty())
			{
				AsyncTask(ENamedThreads::GameThread, [GTSourceEffects = MoveTemp(SourceInfo.SourceEffects)]() mutable
				{
					FAudioThread::RunCommandOnAudioThread([ATSourceEffects = MoveTemp(GTSourceEffects)]() mutable
					{
						for (const TSoundEffectSourcePtr& EffectPtr : ATSourceEffects)
						{
							USoundEffectPreset::UnregisterInstance(EffectPtr);
						}
					});
				});

				SourceInfo.SourceEffects.Reset();
			}
			SourceInfo.SourceEffectPresets.Reset();
		}
	}*/

	bool FMixerSourceManager::GetFreeSourceID(int32& OutSourceId)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (GameThreadInfo.FreeSourceIndices.Num())
		{
			OutSourceId = GameThreadInfo.FreeSourceIndices.Pop();

			ATOM_MIXER_CHECK(OutSourceId < NumTotalSources);
			ATOM_MIXER_CHECK(!GameThreadInfo.bIsBusy[OutSourceId]);

			ATOM_MIXER_CHECK(!GameThreadInfo.bIsDebugMode[OutSourceId]);
			ATOM_MIXER_CHECK(NumActiveSources < NumTotalSources);
			++NumActiveSources;

			GameThreadInfo.bIsBusy[OutSourceId] = true;
			return true;
		}
		ATOM_MIXER_CHECK(false);
		return false;
	}

	int32 FMixerSourceManager::GetNumActiveSources() const
	{
		return NumActiveSources;
	}

	int32 FMixerSourceManager::GetNumActiveAudioBuses() const
	{
		return AudioBuses.Num();
	}

	void FMixerSourceManager::InitSource(const int32 SourceID, const FMixerSourceVoiceInitParams& InitParams)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK(!GameThreadInfo.bIsDebugMode[SourceID]);
		ATOM_MIXER_CHECK(InitParams.SourceListener != nullptr);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

#if ATOM_MIXER_ENABLE_DEBUG_MODE
		GameThreadInfo.bIsDebugMode[SourceID] = InitParams.bIsDebugMode;
#endif 

		// Make sure we flag that this source needs a speaker map to at least get one
		GameThreadInfo.bNeedsSpeakerMap[SourceID] = true;

		GameThreadInfo.bIsUsingHRTFSpatializer[SourceID] = InitParams.bUseHRTFSpatialization;

		// Need to build source effect instances on the audio thread
		/*FSoundEffectSourceInitData InitData;
		InitData.SampleRate = MixerDevice->SampleRate;
		InitData.NumSourceChannels = InitParams.NumInputChannels;
		InitData.AudioClock = MixerDevice->GetAudioTime();
		InitData.AudioDeviceId = MixerDevice->DeviceID;

		TArray<TSoundEffectSourcePtr> SourceEffectChain;
		BuildSourceEffectChain(SourceId, InitData, InitParams.SourceEffectChain, SourceEffectChain);*/

		FModulationDestination VolumeMod;
		VolumeMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
		VolumeMod.UpdateModulators(InitParams.ModulationSettings.VolumeModulation.Modulators);

		FModulationDestination PitchMod;
		PitchMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Pitch"), false /* bInIsBuffered */);
		PitchMod.UpdateModulators(InitParams.ModulationSettings.PitchModulation.Modulators);

		FModulationDestination HighpassMod;
		HighpassMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("HPFCutoffFrequency"), false /* bInIsBuffered */);
		HighpassMod.UpdateModulators(InitParams.ModulationSettings.HighpassModulation.Modulators);

		FModulationDestination LowpassMod;
		LowpassMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("LPFCutoffFrequency"), false /* bInIsBuffered */);
		LowpassMod.UpdateModulators(InitParams.ModulationSettings.LowpassModulation.Modulators);

		// Aisac modulations
		TMap<FAtomAisacControl, FModulationDestination> ControlMods;
		for (auto& ControlDefaultSetting : InitParams.ModulationSettings.AisacModulations.AisacControlModulations)
		{
			const FName ControlName = ControlDefaultSetting.Control.Name;

			const FAtomAisacControlSettings* ControlSettings = InitParams.ModulationSettings.AisacModulations.AisacControlModulations.FindByPredicate([&](FAtomAisacControlSettings& Settings)
			{
				return Settings.Control == ControlDefaultSetting.Control;
			});

			FModulationDestination AisacMod;
			if (ControlSettings)
			{
				AisacMod.Init(AtomRuntime->GetAtomRuntimeID(), "AISAC", false /* bInIsBuffered */, true /* bInValueLinear */);
				AisacMod.UpdateModulators(ControlSettings->Modulators);
			}

			ControlMods.Add(ControlDefaultSetting.Control, MoveTemp(AisacMod));
		}

		/*AtomMixerThreadCommand([
			this,
				SourceId,
				InitParams,
				VolumeModulation = MoveTemp(VolumeMod),
				HighpassModulation = MoveTemp(HighpassMod),
				LowpassModulation = MoveTemp(LowpassMod),
				PitchModulation = MoveTemp(PitchMod),
				SourceEffectChain
		]() mutable*/
		{
			ATOM_MIXER_CHECK(InitParams.SourceVoice != nullptr);

			if (!InitParams.SourceVoice)
			{
				return;
			}

			FSourceInfo& SourceInfo = SourceInfos[SourceID];
			
			// Initialize the mixer source buffer decoder with the given mixer buffer
			SourceInfo.MixerSourcePlayer = InitParams.MixerSourcePlayer;
			ATOM_MIXER_CHECK(SourceInfo.MixerSourcePlayer.Player.IsValid());
			SourceInfo.MixerSourcePlayer.Player->EnablePcmFilter(true); // temp
			SourceInfo.CurrentPCMBuffer = MakeShared<FMixerSourceVoiceBuffer, ESPMode::ThreadSafe>(); // temp move this system to atomplayer by plbkID
			//SourceInfo.MixerSourceBuffer->Init();
			//SourceInfo.MixerSourceBuffer->OnBeginGenerate();

			SourceInfo.bIsPlaying = false;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsPausedForQuantization = false;
			//SourceInfo.bDelayLineSet = false;
			SourceInfo.bIsStopping = false;
			SourceInfo.bIsActive = true;
			//SourceInfo.bIsBusy = true;
			SourceInfo.bIsDone = false;
			//SourceInfo.bIsLastBuffer = false;
			SourceInfo.bUseHRTFSpatializer = InitParams.bUseHRTFSpatialization;
			SourceInfo.bIsExternalSend = InitParams.bIsExternalSend;
			SourceInfo.bIsVorbis = InitParams.bIsVorbis;
			SourceInfo.PlayOrder = InitParams.PlayOrder;
			SourceInfo.ActiveSoundPlayOrder = InitParams.ActiveSoundPlayOrder;
			SourceInfo.AtomComponentID = InitParams.AtomComponentID;
			SourceInfo.bIsSoundfield = InitParams.bIsSoundfield;

			SourceInfo.TimeStretch = InitParams.TimeStretch;

			SourceInfo.NumInputChannels = InitParams.NumInputChannels;
			SourceInfo.NumInputFrames = InitParams.NumInputFrames;

#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
			// init and zero-out buffers
			const int32 BufferSize = NumOutputFrames * InitParams.NumInputChannels;
			SourceInfo.SendsBuffer.SetCapacity(BufferSize + InitParams.NumInputChannels);
			SourceInfo.PreEffectBuffer.Reset();
			SourceInfo.PreEffectBuffer.AddZeroed(BufferSize);
			//SourceInfo.PreDistanceAttenuationBuffer.Reset();
			//SourceInfo.PreDistanceAttenuationBuffer.AddZeroed(BufferSize);
#endif

			Audio::FInlineEnvelopeFollowerInitParams EnvelopeFollowerInitParams;
			EnvelopeFollowerInitParams.SampleRate = AtomRuntime->GetRuntimeSampleRate() / NumOutputFrames;
			EnvelopeFollowerInitParams.AttackTimeMsec = (float)InitParams.EnvelopeFollowerAttackTime;
			EnvelopeFollowerInitParams.ReleaseTimeMsec = (float)InitParams.EnvelopeFollowerReleaseTime;
			EnvelopeFollowerInitParams.Mode = Audio::EPeakMode::Peak;
			SourceInfo.SourceEnvelopeFollower = Audio::FInlineEnvelopeFollower(EnvelopeFollowerInitParams);

			SourceInfo.VolumeModulation = MoveTemp(VolumeMod);
			SourceInfo.PitchModulation = MoveTemp(PitchMod);
			SourceInfo.LowpassModulation = MoveTemp(LowpassMod);
			SourceInfo.HighpassModulation = MoveTemp(HighpassMod);
			SourceInfo.AisacControlModulations.Reset();
			for (auto& ControlMod : ControlMods)
			{
				FSourceInfo::FSourceAisacModulation AisacControlMod = {};
				AisacControlMod.Modulation = MoveTemp(ControlMod.Value);

				SourceInfo.AisacControlModulations.Add(ControlMod.Key, AisacControlMod);
			}

			// Pass required info to clock manager
			const FQuartzQuantizedRequestData& QuantData = InitParams.QuantizedRequestData;
			if (QuantData.QuantizedCommandPtr)
			{
				if (false == AtomRuntime->QuantizedEventClockManager.DoesClockExist(QuantData.ClockName))
				{
					UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Quantization Clock: '%s' Does not exist."), *QuantData.ClockName.ToString());
					QuantData.QuantizedCommandPtr->Cancel();
				}
				else
				{
					FQuartzQuantizedCommandInitInfo QuantCommandInitInfo(QuantData, AtomRuntime->GetRuntimeSampleRate(), SourceID);
					SourceInfo.QuantizedCommandHandle = AtomRuntime->QuantizedEventClockManager.AddCommandToClock(QuantCommandInitInfo);
				}
			}

			// If this is going to be a source bus, add this source id to the list of active bus ids
			if (InitParams.AudioBusID != INDEX_NONE)
			{
				// Setting this BusId will flag this source as a bus. It doesn't try to generate 
				// audio in the normal way but instead will render in a second stage, after normal source rendering (InputPort).
				SourceInfo.AudioBusID = InitParams.AudioBusID;

				// Source bus duration allows us to stop a bus after a given time
				if (InitParams.SourceBusDuration != 0.0f)
				{
					SourceInfo.SourceBusDurationFrames = InitParams.SourceBusDuration * AtomRuntime->GetRuntimeSampleRate(); // ? use bus framerate ?
				}

				// Register this bus as an instance
				bool bAlreadyExisted = false;
				TSharedRef<FMixerAudioBus> AudioBusPtr = FindOrAddAudioBus(SourceInfo.AudioBusID, InitParams.AudioBusChannels, true, &bAlreadyExisted);
				AudioBusPtr->AddInstanceID(SourceID, SourceInfo.MixerSourcePlayer.PlaybackID, InitParams.NumInputChannels);
			
#if ATOM_PROFILERTRACE_ENABLED
				if (!bAlreadyExisted)
				{
					AudioBusPtr->BusName = InitParams.AudioBusName;
					TraceAudioBusStartMessage(AtomRuntime->GetAtomRuntimeID(), InitParams.AudioBusID, InitParams.AudioBusName, InitParams.AudioBusChannels);
				}
#endif // ATOM_PROFILERTRACE_ENABLED

				// Add a patch output to patch splitter of the source bus to keep a single read buffer for this particular source.
				// SonicSync system may use a different thread depending device and/or input sources.
				SourceInfo.SourceBusPatch = AtomRuntime->MakePatch(InitParams.NumInputFrames, InitParams.NumInputChannels, 1.0f);
				AddPendingAudioBusConnection(InitParams.AudioBusID, InitParams.NumInputChannels, false, SourceInfo.SourceBusPatch);
			}

			// Iterate through source's bus sends and add this source to the bus send list
			// Note: buses can also send their audio to other buses.
			for (int32 BusSendStage = 0; BusSendStage < (int32)EAtomBusSendStage::Count; ++BusSendStage)
			{
				for (const FInitAudioBusSend& AudioBusSend : InitParams.AudioBusSends[BusSendStage])
				{
					// New struct to map which source (SourceId) is sending to the bus
					FAudioBusSend NewAudioBusSend;
					NewAudioBusSend.SourceID = SourceID;
					NewAudioBusSend.TransmitterID = SourceInfo.MixerSourcePlayer.PlaybackID;
					NewAudioBusSend.SendLevel = AudioBusSend.SendLevel;

					// Get existing BusId and add the send, or create new bus registration. A newly created bus won't output audio, but 
					// we register the send anyway in the event that this bus does play, we'll know to send this
					// source's audio to it.
					bool bAlreadyExisted = false;
					TSharedRef<FMixerAudioBus> AudioBusPtr = FindOrAddAudioBus(AudioBusSend.AudioBusID, AudioBusSend.BusChannels, true, &bAlreadyExisted);
					AudioBusPtr->AddSend((EAtomBusSendStage)BusSendStage, NewAudioBusSend);	
#if ATOM_PROFILERTRACE_ENABLED
					if (!bAlreadyExisted)
					{
						AudioBusPtr->BusName = AudioBusSend.AudioBusName;
						TraceAudioBusStartMessage(AtomRuntime->GetAtomRuntimeID(), AudioBusSend.AudioBusID, AudioBusSend.AudioBusName, AudioBusSend.BusChannels);
					}
#endif // ATOM_PROFILERTRACE_ENABLED

					// Store on this source, which buses its sending its audio to
					SourceInfo.AudioBusSends[BusSendStage].Add(AudioBusSend.AudioBusID);
				}
			}

			//SourceInfo.CurrentFrameValues.Init(0.0f, InitParams.NumInputChannels);
			//SourceInfo.NextFrameValues.Init(0.0f, InitParams.NumInputChannels);

			// Setup Atom voice
			if (InitParams.ExternalVoicePool.IsValid())
			{
				// create a new ref to external voice pool
				InitParams.SourceVoice->VoicePool = ReplicateExternalCriHandle(InitParams.ExternalVoicePool);
				InitParams.SourceVoice->PoolID = SourceID;
				InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::External;
				InitParams.SourceVoice->StreamingType = EMixerSourceVoiceStreamingType::Mixed;

				ATOM_MIXER_CHECK(MixerSources[SourceID] == nullptr);
				MixerSources[SourceID] = InitParams.SourceVoice;
			}
			else
			// create voice pool on demand
			{
				float MaxPitchScale = AtomRuntime->GetGlobalPitchRange().GetUpperBoundValue();
				const bool bIsStreamingOnly = InitParams.StreamingType == EMixerSourceVoiceStreamingType::StreamOnly;
				const bool bIsStreaming = InitParams.StreamingType != EMixerSourceVoiceStreamingType::MemoryOnly;

				switch (InitParams.InputFormat)
				{
				case EAtomFormat::HCA:
				case EAtomFormat::ADX:
				{
					if (InitParams.bUseFixedVoicePools)
					{
						// by default a cue use a user defined metrics
						InitParams.SourceVoice->VoicePool = nullptr;
						InitParams.SourceVoice->PoolID = FAtomRuntime::GetAtomDefaultVoicePoolIdentifier();
						InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::UserDefined; // Tag for Global
						InitParams.SourceVoice->MaxInputVoices = 128; // max - cue sub voices - limited by pools...
						InitParams.SourceVoice->MaxInputChannels = 128; // max - from pools
						InitParams.SourceVoice->MaxInputSampleRate = 128; // max - from pools
					}
					else // if no user voice pool if wave or cue with no fixed voice pool.
					{
						CriAtomExStandardVoicePoolConfig Config;
						criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&Config);
						Config.identifier = SourceID;
						Config.is_streaming_only = bIsStreamingOnly;
						Config.num_voices = InitParams.InputVoices;
						Config.player_config.max_channels = InitParams.NumInputChannels;
						Config.player_config.max_sampling_rate = InitParams.InputSampleRate * MaxPitchScale;
						Config.player_config.streaming_flag = bIsStreaming;

						InitParams.SourceVoice->VoicePool = FCriWareApi::criAtomExVoicePool_AllocateStandardVoicePool(&Config, nullptr, 0);
						InitParams.SourceVoice->PoolID = SourceID;
						InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::Standard;
						InitParams.SourceVoice->StreamingType = InitParams.StreamingType;
						InitParams.SourceVoice->MaxInputVoices = InitParams.InputVoices; // sub voices (1 for wave and more for cues)
						InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
						InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate * MaxPitchScale;
					}
				}
				break;
				case EAtomFormat::HCAMX:
				{
					CriAtomExHcaMxVoicePoolConfig Config;
					criAtomExVoicePool_SetDefaultConfigForHcaMxVoicePool(&Config);
					Config.identifier = SourceID;
					Config.is_streaming_only = bIsStreamingOnly;
					Config.num_voices = 1; // if a cue get max voices by cue. other (wave) have only one voice
					Config.player_config.max_channels = InitParams.NumInputChannels;
					Config.player_config.max_sampling_rate = InitParams.InputSampleRate * MaxPitchScale;
					Config.player_config.streaming_flag = bIsStreaming;

					InitParams.SourceVoice->VoicePool = FCriWareApi::criAtomExVoicePool_AllocateHcaMxVoicePool(&Config, nullptr, 0);
					InitParams.SourceVoice->PoolID = SourceID;
					InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::HcaMx;
					InitParams.SourceVoice->StreamingType = InitParams.StreamingType;
					InitParams.SourceVoice->MaxInputVoices = 1;
					InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
					InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate * MaxPitchScale;
				}
				break;
				case EAtomFormat::Wave: // wave file data
				{
					CriAtomExWaveVoicePoolConfig Config;
					criAtomExVoicePool_SetDefaultConfigForWaveVoicePool(&Config);
					Config.identifier = SourceID;
					Config.is_streaming_only = bIsStreamingOnly;
					Config.num_voices = 1; // only one
					Config.player_config.max_channels = InitParams.NumInputChannels;
					Config.player_config.max_sampling_rate = InitParams.InputSampleRate * MaxPitchScale;
					Config.player_config.streaming_flag = bIsStreaming;

					InitParams.SourceVoice->VoicePool = FCriWareApi::criAtomExVoicePool_AllocateWaveVoicePool(&Config, nullptr, 0);
					InitParams.SourceVoice->PoolID = SourceID;
					InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::Wave;
					InitParams.SourceVoice->StreamingType = InitParams.StreamingType;
					InitParams.SourceVoice->MaxInputVoices = 1;
					InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
					InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate * MaxPitchScale;
				}
				break;
				case EAtomFormat::AIFF: // aiff file data
				{
					CriAtomExAiffVoicePoolConfig Config;
					criAtomExVoicePool_SetDefaultConfigForAiffVoicePool(&Config);
					Config.identifier = SourceID;
					Config.is_streaming_only = bIsStreamingOnly;
					Config.num_voices = 1; // only one
					Config.player_config.max_channels = InitParams.NumInputChannels;
					Config.player_config.max_sampling_rate = InitParams.InputSampleRate * MaxPitchScale;
					Config.player_config.streaming_flag = bIsStreaming;

					InitParams.SourceVoice->VoicePool = FCriWareApi::criAtomExVoicePool_AllocateAiffVoicePool(&Config, nullptr, 0);
					InitParams.SourceVoice->PoolID = SourceID;
					InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::Aiff;
					InitParams.SourceVoice->StreamingType = InitParams.StreamingType;
					InitParams.SourceVoice->MaxInputVoices = 1;
					InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
					InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate * MaxPitchScale;
				}
				break;
				case EAtomFormat::RawPCM: // procedural sound data to voice buffer
				{
					CriAtomExRawPcmVoicePoolConfig Config;
					criAtomExVoicePool_SetDefaultConfigForRawPcmVoicePool(&Config)
						Config.identifier = SourceID;
					Config.num_voices = 1; // only one
					Config.player_config.max_channels = InitParams.NumInputChannels;
					Config.player_config.max_sampling_rate = InitParams.InputSampleRate;
					Config.player_config.pcm_format = CRIATOM_PCM_FORMAT_FLOAT32;

					InitParams.SourceVoice->VoicePool = FCriWareApi::criAtomExVoicePool_AllocateRawPcmVoicePool(&Config, nullptr, 0);
					InitParams.SourceVoice->PoolID = SourceID;
					InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::RawPcm;
					InitParams.SourceVoice->StreamingType = EMixerSourceVoiceStreamingType::MemoryOnly;
					InitParams.SourceVoice->MaxInputVoices = 1;
					InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
					InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate;
				}
				break;
				case EAtomFormat::AudioBuffer: // procedural buffer to voice (aka InputPort)
				{
					CriAtomExInputPortConfig Config;

					Config.type = CRIATOMEX_INPUT_PORT_TYPE_AUX;
					Config.num_channels = InitParams.NumInputChannels;
					Config.num_grains = InitParams.NumInputFrames; // number of samples per channel per callback
					Config.sampling_rate = InitParams.InputSampleRate;
					//Config.buffering_time; // todo: from audio link / buffer settings ?
					InitParams.SourceVoice->VoicePool = MakeCriHandle((CriAtomExVoicePoolHn)FCriWareApi::criAtomExInputPort_Create(&Config, nullptr, 0));
					InitParams.SourceVoice->CodecType = EMixerSourceVoiceCodecType::InputPort;
					InitParams.SourceVoice->StreamingType = EMixerSourceVoiceStreamingType::MemoryOnly;
					InitParams.SourceVoice->MaxInputVoices = 1; // only one
					InitParams.SourceVoice->MaxInputChannels = InitParams.NumInputChannels;
					InitParams.SourceVoice->MaxInputSampleRate = InitParams.InputSampleRate;
				}
				break;
				case EAtomFormat::Vibration: // replaced by hd-haptic using normal pcm data
				case EAtomFormat::Instrument: // reserved
					// unsupported
					break;
				}

				// todo: call platform specific voice type here

				ATOM_MIXER_CHECK(MixerSources[SourceID] == nullptr);
				MixerSources[SourceID] = InitParams.SourceVoice;
			}

#if ATOM_MIXER_ENABLE_DEBUG_MODE
			ATOM_MIXER_CHECK(!SourceInfo.bIsDebugMode);
			SourceInfo.bIsDebugMode = InitParams.bIsDebugMode;

			ATOM_MIXER_CHECK(SourceInfo.DebugName.IsEmpty());
			SourceInfo.DebugName = InitParams.DebugName;

			ATOM_MIXER_DEBUG_LOG(SourceID, TEXT("Is initializing"));
#endif
		}
	}

	// init the ADX mixer player (AtomExPlayer) and get ADX playbackid
	FAtomPlaybackId FMixerSourceManager::InitSourcePlayer(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		// initialized
		FAtomPlaybackId PlaybackID = SourceInfo.MixerSourcePlayer.PlaybackID;
		if (PlaybackID == 0)  // uninitialized
		{
			PlaybackID = SourceInfo.MixerSourcePlayer.Player->Prepare();
			SourceInfo.MixerSourcePlayer.PlaybackID = PlaybackID;
		}
		else
		{
			SourceInfo.MixerSourcePlayer.Player->UpdateParameters(PlaybackID);
		}

		return PlaybackID;
	}

	// release a voice pool
	void FMixerSourceManager::ReleaseSourceID(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		ATOM_MIXER_CHECK(NumActiveSources > 0);
		--NumActiveSources;

		GameThreadInfo.bIsBusy[SourceID] = false;

#if ATOM_MIXER_ENABLE_DEBUG_MODE
		GameThreadInfo.bIsDebugMode[SourceID] = false;
#endif

		GameThreadInfo.FreeSourceIndices.Push(SourceID);

		ATOM_MIXER_CHECK(GameThreadInfo.FreeSourceIndices.Contains(SourceID));

		{
			ReleaseSource(SourceID);
		}

		/*AtomMixerThreadCommand([this, SourceID]()
		{
			ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

			ReleaseSource(SourceID);
		});*/
	}

	// attach explayer - audio thread no render thread

	void FMixerSourceManager::StartAudioPlayer(FAtomExPlayerKey InAudioPlayerKey)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (AudioPlayers.Contains(InAudioPlayerKey))
		{
			return;
		}

		// If the player is not registered, make a new entry.
		FAtomExPlayerArgs PlayerArgs;
		PlayerArgs.AtomRuntime = AtomRuntime;
		PlayerArgs.ActiveSound = nullptr;
		PlayerArgs.SoundData = nullptr;

		TSharedPtr<FAtomExPlayer> NewExPlayer(new FAtomExPlayer(PlayerArgs));

		AudioPlayers.Add(InAudioPlayerKey, NewExPlayer);
	}

	void FMixerSourceManager::StopAudioPlayer(FAtomExPlayerKey InAudioPlayerKey)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (AudioPlayers.Contains(InAudioPlayerKey))
		{
			return;
		}

		AudioPlayers.Remove(InAudioPlayerKey);
	}

	bool FMixerSourceManager::IsAudioPlayerActive(FAtomExPlayerKey InAudioPlayerKey) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return AudioPlayers.Contains(InAudioPlayerKey);
	}

	// attach bus - audio thread no render thread

	void FMixerSourceManager::StartAudioBus(FAudioBusKey InAudioBusKey, const FString& InAudioBusName, int32 InNumChannels, bool bInIsAutomatic)
	{
		if (AudioBusKeys_AudioThread.Contains(InAudioBusKey))
		{
			return;
		}

		AudioBusKeys_AudioThread.Add(InAudioBusKey);

#if ATOM_PROFILERTRACE_ENABLED
		TraceAudioBusStartMessage(AtomRuntime->GetAtomRuntimeID(), InAudioBusKey.ObjectId, InAudioBusName, InNumChannels);
#endif

		AtomMixerThreadCommand([this, InAudioBusKey, InNumChannels, bInIsAutomatic, InAudioBusName]()
		{
			// If this audio bus id already exists, set it to not be automatic and return it
			bool bAlreadyExisted = false;
			TSharedRef<FMixerAudioBus> AudioBusPtr = FindOrAddAudioBus(InAudioBusKey, InNumChannels, bInIsAutomatic, &bAlreadyExisted);
			if (bAlreadyExisted)
			{
				// If this audio bus already existed, make sure the num channels lines up
				ensure(AudioBusPtr->GetNumChannels() == InNumChannels);
				AudioBusPtr->SetAutomatic(bInIsAutomatic);
			}
			else
			{
#if ATOM_PROFILERTRACE_ENABLED
				AudioBusPtr->BusName = InAudioBusName;
#endif
			}
		}, ATOM_MIXER_THREAD_COMMAND_STRING("StartAudioBus()"));
	}

	// detach bus- audio thread no render thread

	void FMixerSourceManager::StopAudioBus(FAudioBusKey InAudioBusKey)
	{
		if (!AudioBusKeys_AudioThread.Contains(InAudioBusKey))
		{
			return;
		}

		AudioBusKeys_AudioThread.Remove(InAudioBusKey);

#if ATOM_PROFILERTRACE_ENABLED
		TraceAudioBusStopMessage(AtomRuntime->GetAtomRuntimeID(), InAudioBusKey.ObjectId);
#endif

		AtomMixerThreadCommand([this, InAudioBusKey]()
		{
			TSharedPtr<FMixerAudioBus>* AudioBusPtr = AudioBuses.Find(InAudioBusKey);
			if (AudioBusPtr)
			{
				if (!(*AudioBusPtr)->IsAutomatic())
				{
					// Immediately stop all sources which were source buses
					for (FSourceInfo& SourceInfo : SourceInfos)
					{
						if (SourceInfo.AudioBusID == InAudioBusKey.ObjectId)
						{
							SourceInfo.bIsPlaying = false;
							SourceInfo.bIsPaused = false;
							SourceInfo.bIsActive = false;
							SourceInfo.bIsStopping = false;
						}
					}
					AudioBuses.Remove(InAudioBusKey);
				}
			}
		}, ATOM_MIXER_THREAD_COMMAND_STRING("StopAudioBus()"));
	}

	bool FMixerSourceManager::IsAudioBusActive(FAudioBusKey InAudioBusKey) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return AudioBusKeys_AudioThread.Contains(InAudioBusKey);
	}

	// must return the number of channel of the used voice

	int32 FMixerSourceManager::GetAudioBusNumChannels(FAudioBusKey InAudioBusKey) const
	{
		ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);
		
		TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(InAudioBusKey);
		if (AudioBusPtr.IsValid())
		{
			return AudioBusPtr->GetNumChannels();
		}

		return 0;
	}

	void FMixerSourceManager::AddPatchOutputForAudioBus(FAudioBusKey InAudioBusKey, const Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr)
	{
		ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);
		if (AtomRuntime->IsAudioRenderingThread())
		{
			TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(InAudioBusKey);
			if (AudioBusPtr.IsValid())
			{
				AudioBusPtr->AddNewPatchOutput(InPatchOutputStrongPtr);
			}
		}
		else
		{
			// Queue up the command via MPSC command queue
			AtomMixerThreadMPSCCommand([this, InAudioBusKey, InPatchOutputStrongPtr]()
			{
				AddPatchOutputForAudioBus(InAudioBusKey, InPatchOutputStrongPtr);
			}, ATOM_MIXER_THREAD_COMMAND_STRING("AddPatchOutputForAudioBus()"));
		}
	}

	void FMixerSourceManager::AddPatchOutputForAudioBus_AudioThread(FAudioBusKey InAudioBusKey, const Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr)
	{
		AtomMixerThreadCommand([this, InAudioBusKey, NewPatchPtr = InPatchOutputStrongPtr]() mutable
		{
			AddPatchOutputForAudioBus(InAudioBusKey, NewPatchPtr);
		}, ATOM_MIXER_THREAD_COMMAND_STRING("AddPatchOutputForAudioBus_AtomThread()"));
	}

	void FMixerSourceManager::AddPatchInputForAudioBus(FAudioBusKey InAudioBusKey, const Audio::FPatchInput& InPatchInput)
	{
		ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);
		if (AtomRuntime->IsAudioRenderingThread())
		{
			TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(InAudioBusKey);
			if (AudioBusPtr.IsValid())
			{
				AudioBusPtr->AddNewPatchInput(InPatchInput);
			}
		}
		else
		{
			// Queue up the command via MPSC command queue
			AtomMixerThreadMPSCCommand([this, InAudioBusKey, InPatchInput]()
			{
				AddPatchInputForAudioBus(InAudioBusKey, InPatchInput);
			}, ATOM_MIXER_THREAD_COMMAND_STRING("AddPatchInputForAudioBus()"));
		}
	}

	void FMixerSourceManager::AddPatchInputForAudioBus_AudioThread(FAudioBusKey InAudioBusKey, const Audio::FPatchInput& InPatchInput)
	{
		AtomMixerThreadCommand([this, InAudioBusKey, InPatchInput]()
		{
			AddPatchInputForAudioBus(InAudioBusKey, InPatchInput);
		}, ATOM_MIXER_THREAD_COMMAND_STRING("AddPatchInputForAudioBus_AtomThread()"));
	}

	// this is controlled by MixerPlayer and its voices
	// function here just set falgs and handle quantization for Unreal side features
	void FMixerSourceManager::Play(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// Compute the frame within which to start the sound based on the current "thread faction" on the audio thread
		//double StartTime = AtomRuntime->GetAudioThreadTime();

		//AtomMixerThreadCommand([this, SourceID/*, StartTime*/]()
		//{
		//	ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

			FSourceInfo& SourceInfo = SourceInfos[SourceID];
			
			SourceInfo.bIsPlaying = true;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsActive = true;

			//SourceInfo.StartTime = StartTime;

			// ADX
			if (SourceInfo.MixerSourcePlayer.Player.IsValid() && !SourceInfo.bIsPausedForQuantization)
			{
				SourceInfo.MixerSourcePlayer.Player->Resume(SourceInfo.MixerSourcePlayer.PlaybackID);
			}

			ATOM_MIXER_DEBUG_LOG(SourceID, TEXT("Is playing"));
		//});
	}

	void FMixerSourceManager::CancelQuantizedSound(const int32 SourceID)
	{
		if (!AtomRuntime)
		{
			return;
		}

		// If we are in the audio rendering thread, this is being called either before
		// or after source generation, so it is safe (and preffered) to call StopInternal()
		// synchronously. 
		if (AtomRuntime->IsAudioRenderingThread())
		{
			StopInternal(SourceID);

			// Verify we have a reasonable Source
			ATOM_MIXER_CHECK(SourceID < NumTotalSources);
			FSourceInfo& SourceInfo = SourceInfos[SourceID];

			//Update game thread state
			SourceInfo.bIsDone = true;

			// Notify that we're now done with this source
			//if (SourceInfo.SourceListener)
			//{
			//	SourceInfo.SourceListener->OnDone();
			//}

			// ADX - need to call command on AtomThread safely 
			AsyncTask(ENamedThreads::GameThread, [&SourceInfo]() mutable
			{
				FAtomThread::RunCommandOnAtomThread([&SourceInfo]() mutable
				{
					if (SourceInfo.MixerSourcePlayer.Player.IsValid())
					{
						SourceInfo.MixerSourcePlayer.Player->StopWithoutReleaseTime(SourceInfo.MixerSourcePlayer.PlaybackID);
					}
				});
			});
		}
	}
	
	void FMixerSourceManager::Stop(const int32 SourceID)
	{
		if (!AtomRuntime)
		{
			return;
		}

		ATOM_MIXER_CHECK(SourceID < NumTotalSources);

		//Assert that we are being called from the GameThread and the
		//source isn't busy.  Then call StopInternal() in a thread command
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// ADX
		FSourceInfo& SourceInfo = SourceInfos[SourceID];
		if (SourceInfo.MixerSourcePlayer.Player.IsValid())
		{
			SourceInfo.MixerSourcePlayer.Player->StopWithoutReleaseTime(SourceInfo.MixerSourcePlayer.PlaybackID);
		}

		//AtomMixerThreadCommand([this, SourceID]()
		//{
			StopInternal(SourceID);
		//});
	}

	void FMixerSourceManager::StopInternal(const int32 SourceID)
	{
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		SourceInfo.bIsPlaying = false;
		SourceInfo.bIsPaused = false;
		SourceInfo.bIsActive = false;
		SourceInfo.bIsStopping = false;

		if (SourceInfo.bIsPausedForQuantization)
		{
			UE_LOG(LogCriWareAtomMixer, Display, TEXT("StopInternal() cancelling command [%s]"), *SourceInfo.QuantizedCommandHandle.CommandPtr->GetCommandName().ToString());
			SourceInfo.QuantizedCommandHandle.Cancel();
			SourceInfo.bIsPausedForQuantization = false;
		}

		ATOM_MIXER_DEBUG_LOG(SourceID, TEXT("Is immediately stopping"));
	}

	void FMixerSourceManager::StopFade(const int32 SourceID/*, const int32 NumFrames*/)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		//ATOM_MIXER_CHECK(NumFrames > 0);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// ADX -- release time ??
		FSourceInfo& SourceInfo = SourceInfos[SourceID];
		if (SourceInfo.MixerSourcePlayer.Player.IsValid())
		{
			SourceInfo.MixerSourcePlayer.Player->Stop(SourceInfo.MixerSourcePlayer.PlaybackID);
		}

		//AtomMixerThreadCommand([this, SourceID/*, NumFrames*/]()
		//{
			//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

			//FSourceInfo& SourceInfo = SourceInfos[SourceID];

			SourceInfo.bIsPaused = false;
			SourceInfo.bIsStopping = true;

			if (SourceInfo.bIsPausedForQuantization)
			{
				// no need to fade, we haven't actually started playing
				StopInternal(SourceID);
				return;
			}
			
			// Only allow multiple of 4 fade frames and positive
			//int32 NumFadeFrames = AlignArbitrary(NumFrames, 4);
			//if (NumFadeFrames <= 0)
			{
				// Stop immediately if we've been given no fade frames
				SourceInfo.bIsPlaying = false;
				SourceInfo.bIsPaused = false;
				SourceInfo.bIsActive = false;
				SourceInfo.bIsStopping = false;
			}
			//else
			//{
				// compute the fade slope
			//	SourceInfo.VolumeFadeStart = SourceInfo.VolumeSourceStart;
			//	SourceInfo.VolumeFadeNumFrames = NumFadeFrames;
			//	SourceInfo.VolumeFadeSlope = -SourceInfo.VolumeSourceStart / SourceInfo.VolumeFadeNumFrames;
			//	SourceInfo.VolumeFadeFramePosition = 0;
			//}

			ATOM_MIXER_DEBUG_LOG(SourceID, TEXT("Is stopping with fade"));
		//});
	}
	
	void FMixerSourceManager::Pause(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// ADX
		FSourceInfo& SourceInfo = SourceInfos[SourceID];
		if (SourceInfo.MixerSourcePlayer.Player.IsValid())
		{
			SourceInfo.MixerSourcePlayer.Player->Pause(SourceInfo.MixerSourcePlayer.PlaybackID);
		}

		//AtomMixerThreadCommand([this, SourceID]()
		//{
		//	ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

			//FSourceInfo& SourceInfo = SourceInfos[SourceID];

			SourceInfo.bIsPaused = true;
			SourceInfo.bIsActive = false;
		//});
	}

	/*void FMixerSourceManager::SetPitch(const int32 SourceId, const float Pitch)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);

		AtomMixerThreadCommand([this, SourceId, Pitch]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			check(NumOutputFrames > 0);

			SourceInfos[SourceId].PitchSourceParam.SetValue(Pitch, NumOutputFrames);
		});
	}

	void FMixerSourceManager::SetVolume(const int32 SourceId, const float Volume)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, Volume]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			check(NumOutputFrames > 0);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Only set the volume if we're not stopping. Stopping sources are setting their volume to 0.0.
			if (!SourceInfo.bIsStopping)
			{
				// If we've not yet set a volume, we need to immediately set the start and destination to be the same value (to avoid an initial fade in)
				if (SourceInfos[SourceId].VolumeSourceDestination < 0.0f)
				{
					SourceInfos[SourceId].VolumeSourceStart = Volume;
				}

				SourceInfos[SourceId].VolumeSourceDestination = Volume;
			}
		});
	}

	void FMixerSourceManager::SetDistanceAttenuation(const int32 SourceId, const float DistanceAttenuation)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, DistanceAttenuation]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			check(NumOutputFrames > 0);

			// If we've not yet set a distance attenuation, we need to immediately set the start and destination to be the same value (to avoid an initial fade in)
			if (SourceInfos[SourceId].DistanceAttenuationSourceDestination < 0.0f)
			{
				SourceInfos[SourceId].DistanceAttenuationSourceStart = DistanceAttenuation;
			}

			SourceInfos[SourceId].DistanceAttenuationSourceDestination = DistanceAttenuation;
		});
	}

	void FMixerSourceManager::SetSpatializationParams(const int32 SourceId, const FSpatializationParams& InParams)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InParams]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			SourceInfos[SourceId].SpatParams = InParams;
		});
	}

	void FMixerSourceManager::SetChannelMap(const int32 SourceId, const uint32 NumInputChannels, const Audio::FAlignedFloatBuffer& ChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, NumInputChannels, ChannelMap, bInIs3D, bInIsCenterChannelOnly]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			check(NumOutputFrames > 0);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			FMixerSourceSubmixOutputBuffer& SourceSubmixOutput = SourceSubmixOutputBuffers[SourceId];

			if (SourceSubmixOutput.GetNumSourceChannels() != NumInputChannels && !SourceInfo.bUseHRTFSpatializer)
			{
				// This means that this source has been reinitialized as a different source while this command was in flight,
				// In which case it is of no use to us. Exit.
				return;
			}

			// Set whether or not this is a 3d channel map and if its center channel only. Used for reseting channel maps on device change.
			SourceInfo.bIs3D = bInIs3D;
			SourceInfo.bIsCenterChannelOnly = bInIsCenterChannelOnly;

			bool bNeedsSpeakerMap = SourceSubmixOutput.SetChannelMap(ChannelMap, bInIsCenterChannelOnly);
			GameThreadInfo.bNeedsSpeakerMap[SourceId] = bNeedsSpeakerMap;
		});
	}

	void FMixerSourceManager::SetLPFFrequency(const int32 SourceId, const float InLPFFrequency)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InLPFFrequency]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// LowPassFreq is cached off as the version set by this setter as well as that internal to the LPF.
			// There is a second cutoff frequency cached in SourceInfo.LowpassModulation updated per buffer callback.
			// On callback, the client version may be overridden with the modulation LPF value depending on which is more aggressive.  
			SourceInfo.LowPassFreq = InLPFFrequency;
			SourceInfo.LowPassFilter.StartFrequencyInterpolation(InLPFFrequency, NumOutputFrames);
		});
	}

	void FAtomSourFMixerSourceManagerceManager::SetHPFFrequency(const int32 SourceId, const float InHPFFrequency)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InHPFFrequency]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// HighPassFreq is cached off as the version set by this setter as well as that internal to the HPF.
			// There is a second cutoff frequency cached in SourceInfo.HighpassModulation updated per buffer callback.
			// On callback, the client version may be overridden with the modulation HPF value depending on which is more aggressive.  
			SourceInfo.HighPassFreq = InHPFFrequency;
			SourceInfo.HighPassFilter.StartFrequencyInterpolation(InHPFFrequency, NumOutputFrames);
		});
	}*/

	void FMixerSourceManager::SetModVolume(const int32 SourceId, const float InModVolume)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		//AtomMixerThreadCommand([this, SourceId, InModVolume]()
		//{
		//	ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.VolumeModulationBase = InModVolume;
		//});
	}

	void FMixerSourceManager::SetModPitch(const int32 SourceId, const float InModPitch)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		//AtomMixerThreadCommand([this, SourceId, InModPitch]()
		//{
		//	ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.PitchModulationBase = InModPitch;
		//});
	}

	void FMixerSourceManager::SetModLPFFrequency(const int32 SourceId, const float InLPFFrequency)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		//AtomMixerThreadCommand([this, SourceId, InLPFFrequency]()
		//{
			//ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.LowpassModulationBase = InLPFFrequency;
			SourceInfo.bModFiltersUpdated = true;
		//});
	}

	void FMixerSourceManager::SetModHPFFrequency(const int32 SourceId, const float InHPFFrequency)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		//AtomMixerThreadCommand([this, SourceId, InHPFFrequency]()
		//{
			//ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.HighpassModulationBase = InHPFFrequency;
			SourceInfo.bModFiltersUpdated = true;
		//});
	}

	void FMixerSourceManager::SetModulationRouting(const int32 SourceId, FAtomSoundModulationDefaultSettings& ModulationSettings)
	{
		FSourceInfo& SourceInfo = SourceInfos[SourceId];

		FModulationDestination VolumeMod;
		VolumeMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
		VolumeMod.UpdateModulators(ModulationSettings.VolumeModulation.Modulators);

		FModulationDestination PitchMod;
		PitchMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Pitch"), false /* bInIsBuffered */);
		PitchMod.UpdateModulators(ModulationSettings.PitchModulation.Modulators);

		FModulationDestination HighpassMod;
		HighpassMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("HPFCutoffFrequency"), false /* bInIsBuffered */);
		HighpassMod.UpdateModulators(ModulationSettings.HighpassModulation.Modulators);

		FModulationDestination LowpassMod;
		LowpassMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("LPFCutoffFrequency"), false /* bInIsBuffered */);
		LowpassMod.UpdateModulators(ModulationSettings.LowpassModulation.Modulators);

		// Aisac modulations
		TMap<FAtomAisacControl, FModulationDestination> ControlMods;
		for (auto& ControlDefaultSetting : ModulationSettings.AisacModulations.AisacControlModulations)
		{
			const FName ControlName = ControlDefaultSetting.Control.Name;

			FAtomAisacControlSettings* ControlSettings = ModulationSettings.AisacModulations.AisacControlModulations.FindByPredicate([&](FAtomAisacControlSettings& Settings)
			{
				return Settings.Control == ControlDefaultSetting.Control;
			});

			FModulationDestination AisacMod;
			if (ControlSettings)
			{
				AisacMod.Init(AtomRuntime->GetAtomRuntimeID(), "AISAC", false /* bInIsBuffered */, true /* bInValueLinear */);
				AisacMod.UpdateModulators(ControlSettings->Modulators);
			}

			ControlMods.Add(ControlDefaultSetting.Control, MoveTemp(AisacMod));
		}

	//	AtomMixerThreadCommand([
	//		this,
	//			SourceId,
	//			VolumeModulation = MoveTemp(VolumeMod),
	//			HighpassModulation = MoveTemp(HighpassMod),
	//			LowpassModulation = MoveTemp(LowpassMod),
	//			PitchModulation = MoveTemp(PitchMod)
	//			ControlModulations = MoveTemp(ControlMods)
	//	]() mutable
	//		{
				//FSourceInfo& SourceInfo = SourceInfos[SourceId];

				SourceInfo.VolumeModulation = MoveTemp(VolumeMod);
				SourceInfo.PitchModulation = MoveTemp(PitchMod);
				SourceInfo.LowpassModulation = MoveTemp(LowpassMod);
				SourceInfo.HighpassModulation = MoveTemp(HighpassMod);

				SourceInfo.AisacControlModulations.Reset();
				for (auto& ControlMod : ControlMods)
				{
					FSourceInfo::FSourceAisacModulation AisacControlMod = {};
					AisacControlMod.Modulation = MoveTemp(ControlMod.Value);

					SourceInfo.AisacControlModulations.Add(ControlMod.Key, AisacControlMod);
				}
	//		});
	}

	/*void FMixerSourceManager::SetSourceBufferListener(const int32 SourceId, FSharedISourceBufferListenerPtr& InSourceBufferListener, bool InShouldSourceBufferListenerZeroBuffer)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InSourceBufferListener, InShouldSourceBufferListenerZeroBuffer]()
		{
			ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.SourceBufferListener = InSourceBufferListener;
			SourceInfo.bShouldSourceBufferListenerZeroBuffer = InShouldSourceBufferListenerZeroBuffer;
		});
	}*/

	/*void FMixerSourceManager::SetSubmixSendInfo(const int32 SourceId, const FMixerSourceSubmixSend& InSubmixSend)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InSubmixSend]()
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			FMixerSubmixPtr InSubmixPtr = InSubmixSend.Submix.Pin();
			if (InSubmixPtr.IsValid())
			{
				bool bIsNew = true;
				
				SourceInfo.bHasPreDistanceAttenuationSend = false;
				for (FMixerSourceSubmixSend& SubmixSend : SourceInfo.SubmixSends)
				{
					FMixerSubmixPtr SubmixPtr = SubmixSend.Submix.Pin();
					if (SubmixPtr.IsValid())
					{
						if (SubmixSend.SubmixSendStage == EMixerSourceSubmixSendStage::PreDistanceAttenuation)
						{
							SourceInfo.bHasPreDistanceAttenuationSend = true;
						}
					
						if (SubmixPtr->GetId() == InSubmixPtr->GetId())
						{
							SubmixSend.SendLevel = InSubmixSend.SendLevel;
							SubmixSend.SubmixSendStage = InSubmixSend.SubmixSendStage;
							bIsNew = false;
							if (SourceInfo.bHasPreDistanceAttenuationSend)
							{
								break;
							}
						}
					}
				}

				if (bIsNew)
				{
					SourceInfo.SubmixSends.Add(InSubmixSend);
				}
				
				// If we don't have a pre-distance attenuation send, lets zero out the buffer so the output buffer stops doing math with it.
				if (!SourceInfo.bHasPreDistanceAttenuationSend)
				{
					SourceSubmixOutputBuffers[SourceId].SetPreAttenuationSourceBuffer(nullptr);
				}

				FMixerSourceVoice* SourceVoice = MixerSources[SourceId];
				if (ensureAlways(nullptr != SourceVoice))
				{
					InSubmixPtr->AddOrSetSourceVoice(SourceVoice, InSubmixSend.SendLevel, InSubmixSend.SubmixSendStage);
				}
			}
		});
	}

	void FMixerSourceManager::ClearSubmixSendInfo(const int32 SourceId, const FMixerSourceSubmixSend& InSubmixSend)
	{
		ATOM_MIXER_CHECK(SourceId < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		ATOM_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AtomMixerThreadCommand([this, SourceId, InSubmixSend]()
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			FMixerSubmixPtr InSubmixPtr = InSubmixSend.Submix.Pin();
			if (InSubmixPtr.IsValid())
			{
				for (int32 i = SourceInfo.SubmixSends.Num() - 1; i >= 0; --i)
				{
					if (SourceInfo.SubmixSends[i].Submix == InSubmixSend.Submix)
					{
						SourceInfo.SubmixSends.RemoveAtSwap(i, 1, false);
					}
				}

				// Update the has predist attenuation send state
				SourceInfo.bHasPreDistanceAttenuationSend = false;
				for (FMixerSourceSubmixSend& SubmixSend : SourceInfo.SubmixSends)
				{
					FMixerSubmixPtr SubmixPtr = SubmixSend.Submix.Pin();
					if (SubmixPtr.IsValid())
					{
						if (SubmixSend.SubmixSendStage == EMixerSourceSubmixSendStage::PreDistanceAttenuation)
						{
							SourceInfo.bHasPreDistanceAttenuationSend = true;
							break;
						}
					}
				}

				// If we don't have a pre-distance attenuation send, lets zero out the buffer so the output buffer stops doing math with it.
				if (!SourceInfo.bHasPreDistanceAttenuationSend)
				{
					SourceSubmixOutputBuffers[SourceId].SetPreAttenuationSourceBuffer(nullptr);
				}

				// Now remove the source voice from the submix send list
				InSubmixPtr->RemoveSourceVoice(MixerSources[SourceId]);
			}
		});
	}*/

	void FMixerSourceManager::SetBusSendInfo(const int32 SourceID, EAtomBusSendStage InAudioBusSendStage, uint32 AudioBusID, float BusSendLevel, const FString& InBusName)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		AtomMixerThreadCommand([this, SourceID, InAudioBusSendStage, AudioBusID, BusSendLevel, InBusName]()
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceID];

			// Create mapping of source id to bus send level
			FAudioBusSend BusSend;
			BusSend.SourceID = SourceID;
			BusSend.TransmitterID = SourceInfo.MixerSourcePlayer.PlaybackID;
			BusSend.SendLevel = BusSendLevel;

			// Retrieve the bus we want to send audio to. A newly created bus won't output audio, but 
			// we register the send anyway in the event that this bus does play, we'll know to send this
			// source's audio to it.
			bool bAlreadyExisted = false;
			TSharedRef<FMixerAudioBus> AudioBusPtr = FindOrAddAudioBus(AudioBusID, SourceInfo.NumInputChannels, true, &bAlreadyExisted);
			AudioBusPtr->AddSend(InAudioBusSendStage, BusSend);

#if ATOM_PROFILERTRACE_ENABLED
			if (!bAlreadyExisted)
			{
				AudioBusPtr->BusName = InBusName;
				TraceAudioBusStartMessage(AtomRuntime->GetAtomRuntimeID(), AudioBusID, InBusName, SourceInfo.NumInputChannels);
			}
#endif // ATOM_PROFILERTRACE_ENABLED

			// Check to see if we need to create new bus data. If we are not playing a bus with this id, then we
			// need to create a slot for it such that when a bus does play, it'll start rendering audio from this source
			bool bExisted = false;
			for (uint32 BusID : SourceInfo.AudioBusSends[(int32)InAudioBusSendStage])
			{
				if (BusID == AudioBusID)
				{
					bExisted = true;
					break;
				}
			}

			if (!bExisted)
			{
				SourceInfo.AudioBusSends[(int32)InAudioBusSendStage].Add(AudioBusID);
			}
		});
	}

	/*void FMixerSourceManager::SetListenerTransforms(const TArray<FTransform>& InListenerTransforms)
	{
		AtomMixerThreadCommand([this, InListenerTransforms]()
		{
			ListenerTransforms = InListenerTransforms;
		});
	}

	const TArray<FTransform>* FMixerSourceManager::GetListenerTransforms() const
	{
		ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
		return &ListenerTransforms;
	}*/

	int64 FMixerSourceManager::GetNumFramesPlayed(const int32 SourceID) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return SourceInfos[SourceID].NumFramesPlayed;
	}

	float FMixerSourceManager::GetEnvelopeValue(const int32 SourceID) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return SourceInfos[SourceID].SourceEnvelopeValue;
	}

	void FMixerSourceManager::SetVoiceEffect(const int32 SourceID, const FAtomSourceVoiceEffect& Params)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		ATOM_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceID]);

		// ... actualy handled in explayer mixer source
	}

	bool FMixerSourceManager::IsUsingHRTFSpatializer(const int32 SourceID) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return GameThreadInfo.bIsUsingHRTFSpatializer[SourceID];
	}
	/*
	bool FMixerSourceManager::NeedsSpeakerMap(const int32 SourceID) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);
		return GameThreadInfo.bNeedsSpeakerMap[SourceID];
	}*/

	// called by asr thread from AtomExPlayer (MixerSourcePlayer)
	void FMixerSourceManager::SubmitBuffer(const int32 SourceID, float** Data, int32 NumChannels, int32 NumFrames)
	{
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

#if ATOM_MIXER_ENABLE_PCM_TRACE
		UE_LOG(LogTemp, Warning, TEXT("[%d bus:%d] IN RAW %d [%f %f %f %f]"), SourceID, SourceInfo.AudioBusID, NumFrames
			, Data[0][0]
			, Data[0][1]
			, Data[0][2]
			, Data[0][3]);
#endif
		
		//check(NumChannels == SourceInfo.NumInputChannels); action cue change over frame ~~
		SourceInfo.CurrentAudioChunk = TArrayView<float>((float*)Data, NumChannels * NumFrames);
		SourceInfo.CurrentAudioChunkNumFrames = NumFrames;

		if (NumFrames > 0 && SourceInfo.CurrentPCMBuffer.IsValid())
		{
			FAlignedFloatBuffer& AudioData = SourceInfo.CurrentPCMBuffer->AudioData;

			// First, copy Atom buffer to interleave buffer for DSP.
			Interleave((const float**)Data, AudioData, NumChannels, NumFrames);

			// unreal call it true then false on all sources .... here we work By source from Atom
			// todo remove or use pcm caches
			bool bGenerateBuses = true;
			// Buses generate their input buffers independently
			// Get the next block of frames from the source buffers
			ComputeSourceBuffer(bGenerateBuses, SourceID);

			// Compute the audio source buffers after their individual effect chain processing
			//ComputePostSourceEffectBuffer(bGenerateBuses, SourceID);

			// Get the audio for the output buffers
			ComputeOutputBuffer(bGenerateBuses, SourceID);

			// De-interleave and ovrride result buffer for Atom ASR.
			Deinterleave(AudioData, Data, NumChannels, NumFrames);
		}
	}

	void FMixerSourceManager::ComputeSourceBuffer(const bool bGenerateBuses, const int32 SourceID)
	{
		CSV_SCOPED_TIMING_STAT(Atom, SourceBuffer);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		const bool bIsSourceBus = SourceInfo.AudioBusID != INDEX_NONE;
		//if((bGenerateBuses && !bIsSourceBus) || (!bGenerateBuses && bIsSourceBus))
		//{
		//	return;
		//}

		const int32 NumSamples = SourceInfo.CurrentAudioChunkNumFrames * SourceInfo.NumInputChannels;

		//float* PreDistanceAttenBufferPtr = SourceInfo.CurrentPCMBuffer->AudioData.GetData();

		//if (SourceInfo.SubCallbackDelayLengthInFrames && !SourceInfo.bDelayLineSet && !PerSourceResampling)
		//{
		//	SourceInfo.SourceBufferDelayLine.SetCapacity(SourceInfo.SubCallbackDelayLengthInFrames * SourceInfo.NumInputChannels + SourceInfo.NumInputChannels);
		//	SourceInfo.SourceBufferDelayLine.PushZeros(SourceInfo.SubCallbackDelayLengthInFrames * SourceInfo.NumInputChannels);
		//	SourceInfo.bDelayLineSet = true;
		//}

		// if this is a bus, we just want to copy the bus audio to this source's output audio 
		// Note a bus use an Atom inputport as source voice.
		if (bIsSourceBus)
		{
			// Get the source's rendered and mixed audio bus data
			const TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(SourceInfo.AudioBusID);
			if (AudioBusPtr.IsValid())
			{
				int32 NumFramesPlayed = SourceInfo.CurrentAudioChunkNumFrames; // dynamic
				if (SourceInfo.SourceBusDurationFrames != INDEX_NONE)
				{
					// If we're now finishing, only copy over the real data
					if ((SourceInfo.NumFramesPlayed + NumOutputFrames) >= SourceInfo.SourceBusDurationFrames)
					{
						NumFramesPlayed = SourceInfo.SourceBusDurationFrames - SourceInfo.NumFramesPlayed;
						//SourceInfo.bIsLastBuffer = true;
					}
				}

				SourceInfo.NumFramesPlayed += NumFramesPlayed;

#if ATOM_MIXER_ENABLE_PCM_TRACE
				int32 NumSamplesAvalaible = SourceInfo.SourceBusPatch->GetNumSamplesAvailable();
#endif

				// Retrieve the channel map of going from the audio bus channel count to the source channel count since they may not match
				int32 NumAudioBusChannels = AudioBusPtr->GetNumChannels();
				if (NumAudioBusChannels != SourceInfo.NumInputChannels)
				{
					int32 NumAudioBusSamples = SourceInfo.CurrentAudioChunkNumFrames * NumAudioBusChannels;

					Atom::FAlignedFloatBuffer ChannelMap;
					AtomRuntime->Get2DChannelMap(SourceInfo.bIsVorbis, AudioBusPtr->GetNumChannels(), SourceInfo.NumInputChannels, SourceInfo.bIsCenterChannelOnly, ChannelMap);
					
					Atom::FAlignedFloatBuffer BusOutBuffer; // intermedate buffer
					BusOutBuffer.SetNumUninitialized(NumAudioBusSamples);
					SourceInfo.SourceBusPatch->PopAudio(BusOutBuffer.GetData(), NumAudioBusSamples, false);
					Audio::DownmixBuffer(NumAudioBusChannels, SourceInfo.NumInputChannels, BusOutBuffer, SourceInfo.CurrentPCMBuffer->AudioData, ChannelMap.GetData());
				}
				else
				{
					// Get data from source bus patch splitter to avoid lag with Atom SonicSync
					SourceInfo.SourceBusPatch->PopAudio(SourceInfo.CurrentPCMBuffer->AudioData.GetData(), NumSamples, false);
				}

#if ATOM_MIXER_ENABLE_PCM_TRACE
				UE_LOG(LogTemp, Warning, TEXT("[%d] BUS OUT %d (%d) %d==%d [%f %f] [%f %f]"), SourceID, NumFramesPlayed, NumSamplesAvalaible, NumSamples, SourceInfo.CurrentPCMBuffer->AudioData.Num()
					, SourceInfo.CurrentPCMBuffer->AudioData[0]
					, SourceInfo.CurrentPCMBuffer->AudioData[1]
					, SourceInfo.CurrentPCMBuffer->AudioData[2]
					, SourceInfo.CurrentPCMBuffer->AudioData[3]);


				/*FString PCMDump;
				for (float Data : SourceInfo.CurrentPCMBuffer->AudioData)
				{
					PCMDump.Appendf(TEXT(" %f"), Data);
				}

				UE_LOG(LogTemp, Warning, TEXT("[%d] BUS OUT %d %d\n%s"), SourceID, NumFramesPlayed, SourceInfo.CurrentPCMBuffer->AudioData.Num(), *PCMDump);*/
#endif
			}
		}
		else
		{
			int32 NumFramesPlayed = SourceInfo.CurrentAudioChunkNumFrames; // dynamic

			// Bump up the frames played -- this is tracking the total frames in source file played
			SourceInfo.NumFramesPlayed += NumFramesPlayed;
		}
	}

	void FMixerSourceManager::ComputeOutputBuffer(const bool bGenerateBuses, const int32 SourceID)
	{
		CSV_SCOPED_TIMING_STAT(Atom, SourceOutputBuffer);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		float* PCMBufferPtr = SourceInfo.CurrentPCMBuffer->AudioData.GetData();
		const int32 NumSamples = SourceInfo.CurrentPCMBuffer->AudioData.Num();

		// This is used by pre-effect bus sends
#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		for (int AudioBusID : SourceInfo.AudioBusSends[(int32)EAtomBusSendStage::PreEffect])
		{
			TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(AudioBusID);
			if (AudioBusPtr.IsValid())
			{
				AudioBusPtr->PushSendAudio(EAtomBusSendStage::PreEffect, SourceID, SourceInfo.CurrentPCMBuffer->AudioData);
			}
		}
#else
		if (SourceInfo.AudioBusSends[(int32)EBusSendType::PreEffect].Num() > 0)
		{
			FScopeLock Lock(&SourceInfo.SendsBufferCS);
			SourceInfo.SendsBuffer.Reserve(NumSamples, true);
			SourceInfo.SendsBuffer.Push(PCMBufferPtr, NumSamples);
		}
#endif

		float* PreDistanceAttenBufferPtr = PCMBufferPtr;
		TArrayView<float> PreDistanceAttenBufferView(PreDistanceAttenBufferPtr, NumSamples);
		//const bool bWasEffectTailsDone = SourceInfo.bEffectTailsDone;

		if (!AtomDisableEnvelopeFollowingCvar)
		{
			// Compute the source envelope using pre-distance attenuation buffer
			float AverageSampleValue = Audio::ArrayGetAverageAbsValue(PreDistanceAttenBufferView);
			SourceInfo.SourceEnvelopeValue = SourceInfo.SourceEnvelopeFollower.ProcessSample(AverageSampleValue);
			SourceInfo.SourceEnvelopeValue = FMath::Clamp(SourceInfo.SourceEnvelopeValue, 0.f, 1.f);

			//SourceInfo.bEffectTailsDone = SourceInfo.bEffectTailsDone || SourceInfo.SourceEnvelopeValue < ENVELOPE_TAIL_THRESHOLD;
		}
		else
		{
			//SourceInfo.bEffectTailsDone = true;
		}

		//if (!bWasEffectTailsDone && SourceInfo.bEffectTailsDone)
		//{
		//	SourceInfo.SourceListener->OnEffectTailsDone();
		//}
	}

	/*void FMixerSourceManager::ReadSourceFrame(const int32 SourceID)
	{
		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		const int32 NumChannels = SourceInfo.NumInputChannels;

		// Check if the next frame index is out of range of the total number of frames we have in our current audio buffer
		bool bNextFrameOutOfRange = (SourceInfo.CurrentFrameIndex + 1) >= SourceInfo.CurrentAudioChunkNumFrames;
		bool bCurrentFrameOutOfRange = SourceInfo.CurrentFrameIndex >= SourceInfo.CurrentAudioChunkNumFrames;

		bool bReadCurrentFrame = true;

		// Check the boolean conditions that determine if we need to pop buffers from our queue (in PCMRT case) *OR* loop back (looping PCM data)
		while (bNextFrameOutOfRange || bCurrentFrameOutOfRange)
		{
			// If our current frame is in range, but next frame isn't, read the current frame now to avoid pops when transitioning between buffers
			if (bNextFrameOutOfRange && !bCurrentFrameOutOfRange)
			{
				// Don't need to read the current frame audio after reading new audio chunk
				bReadCurrentFrame = false;

				ATOM_MIXER_CHECK(SourceInfo.CurrentPCMBuffer.IsValid());
				const float* AudioData = SourceInfo.CurrentPCMBuffer->AudioData.GetData();
				const int32 CurrentSampleIndex = SourceInfo.CurrentFrameIndex * NumChannels;

				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					SourceInfo.CurrentFrameValues[Channel] = AudioData[CurrentSampleIndex + Channel];
				}
			}

			// If this is our first PCM buffer, we don't need to do a callback to get more audio
			if (SourceInfo.CurrentPCMBuffer.IsValid())
			{
				if (SourceInfo.CurrentPCMBuffer->LoopCount == Audio::LOOP_FOREVER && !SourceInfo.CurrentPCMBuffer->bRealTimeBuffer)
				{
					AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Hit Loop boundary, looping."));

					SourceInfo.CurrentFrameIndex = FMath::Max(SourceInfo.CurrentFrameIndex - SourceInfo.CurrentAudioChunkNumFrames, 0);
					break;
				}

				if (ensure(SourceInfo.MixerSourceBuffer.IsValid()))
				{
					SourceInfo.MixerSourceBuffer->OnBufferEnd();
				}
			}

			// If we have audio in our queue, we're still playing
			if (ensure(SourceInfo.MixerSourceBuffer.IsValid()) && SourceInfo.MixerSourceBuffer->GetNumBuffersQueued() > 0 && NumChannels > 0)
			{
				SourceInfo.CurrentPCMBuffer = SourceInfo.MixerSourceBuffer->GetNextBuffer();
				SourceInfo.CurrentAudioChunkNumFrames = SourceInfo.CurrentPCMBuffer->AudioData.Num() / NumChannels;

				// Subtract the number of frames in the current buffer from our frame index.
				// Note: if this is the first time we're playing, CurrentFrameIndex will be 0
				if (bReadCurrentFrame)
				{
					SourceInfo.CurrentFrameIndex = FMath::Max(SourceInfo.CurrentFrameIndex - SourceInfo.CurrentAudioChunkNumFrames, 0);
				}
				else
				{
					// Since we're not reading the current frame, we allow the current frame index to be negative (NextFrameIndex will then be 0)
					// This prevents dropping a frame of audio on the buffer boundary
					SourceInfo.CurrentFrameIndex = -1;
				}
			}
			else
			{
				SourceInfo.bIsLastBuffer = !SourceInfo.SubCallbackDelayLengthInFrames;
				SourceInfo.SubCallbackDelayLengthInFrames = 0;
				return;
			}

			bNextFrameOutOfRange = (SourceInfo.CurrentFrameIndex + 1) >= SourceInfo.CurrentAudioChunkNumFrames;
			bCurrentFrameOutOfRange = SourceInfo.CurrentFrameIndex >= SourceInfo.CurrentAudioChunkNumFrames;
		}

		if (SourceInfo.CurrentPCMBuffer.IsValid())
		{
			// Grab the float PCM audio data (which could be a new audio chunk from previous ReadSourceFrame call)
			const float* AudioData = SourceInfo.CurrentPCMBuffer->AudioData.GetData();
			const int32 CurrentSampleIndex = SourceInfo.CurrentFrameIndex * NumChannels;
			const int32 NextSampleIndex = (SourceInfo.CurrentFrameIndex + 1)  * NumChannels;
			const int32 AudioDataNum = SourceInfo.CurrentPCMBuffer->AudioData.Num();

			if(ensureAlwaysMsgf(AudioDataNum >= NextSampleIndex + NumChannels
				, TEXT("Bailing due to bad CurrentPCMBuffer:  AudioData.Num() = %i, NextSampleIndex = %i, NumChannels = %i"), AudioDataNum, NextSampleIndex, NumChannels))
			{
				if (bReadCurrentFrame)
				{
					for (int32 Channel = 0; Channel < NumChannels; ++Channel)
					{
						SourceInfo.CurrentFrameValues[Channel] = AudioData[CurrentSampleIndex + Channel];
						SourceInfo.NextFrameValues[Channel] = AudioData[NextSampleIndex + Channel];
					}
				}
				else if (NextSampleIndex != SourceInfo.CurrentPCMBuffer->AudioData.Num())
				{
					for (int32 Channel = 0; Channel < NumChannels; ++Channel)
					{
						SourceInfo.NextFrameValues[Channel] = AudioData[NextSampleIndex + Channel];
					}
				}
			}
			else
			{
				// fill w/ silence instead of the bad access
				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					SourceInfo.CurrentFrameValues[Channel] = 0.f;
					SourceInfo.NextFrameValues[Channel] = 0.f;
				}
			}
		}
	}

	void FAtomSourceManager::ComputeSourceBuffersForIdRange(const bool bGenerateBuses, const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		CSV_SCOPED_TIMING_STAT(Audio, SourceBuffers);
		CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceBuffers, (SourceIdStart < SourceIdEnd));

		const double AudioRenderThreadTime = MixerDevice->GetAudioRenderThreadTime();
		const double AudioClockDelta = MixerDevice->GetAudioClockDelta();

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || SourceInfo.bIsPaused || SourceInfo.bIsPausedForQuantization)
			{
				continue;
			}

			// If this source is still playing at this point but technically done, zero the buffers. We haven't yet been removed by the FMixerSource owner.
			// This should be rare but could happen due to thread timing since done-ness is queried on audio thread.
			if (SourceInfo.bIsDone)
			{
				const int32 NumSamples = NumOutputFrames * SourceInfo.NumInputChannels;

				SourceInfo.PreDistanceAttenuationBuffer.Reset();
				SourceInfo.PreDistanceAttenuationBuffer.AddZeroed(NumSamples);

				SourceInfo.SourceBuffer.Reset();
				SourceInfo.SourceBuffer.AddZeroed(NumSamples);

				continue;
			}

			const bool bIsSourceBus = SourceInfo.AudioBusId != INDEX_NONE;
			if ((bGenerateBuses && !bIsSourceBus) || (!bGenerateBuses && bIsSourceBus))
			{
				continue;
			}

			// Fill array with elements all at once to avoid sequential Add() operation overhead.
			const int32 NumSamples = NumOutputFrames * SourceInfo.NumInputChannels;
			
			// Initialize both the pre-distance attenuation buffer and the source buffer
			SourceInfo.PreDistanceAttenuationBuffer.Reset();
			SourceInfo.PreDistanceAttenuationBuffer.AddZeroed(NumSamples);


			SourceInfo.SourceEffectScratchBuffer.Reset();
			SourceInfo.SourceEffectScratchBuffer.AddZeroed(NumSamples);

			SourceInfo.SourceBuffer.Reset();
			SourceInfo.SourceBuffer.AddZeroed(NumSamples);

			if (SourceInfo.SubCallbackDelayLengthInFrames && !SourceInfo.bDelayLineSet)
			{
				SourceInfo.SourceBufferDelayLine.SetCapacity(SourceInfo.SubCallbackDelayLengthInFrames * SourceInfo.NumInputChannels + SourceInfo.NumInputChannels);
				SourceInfo.SourceBufferDelayLine.PushZeros(SourceInfo.SubCallbackDelayLengthInFrames * SourceInfo.NumInputChannels);
				SourceInfo.bDelayLineSet = true;
			}

			float* PreDistanceAttenBufferPtr = SourceInfo.PreDistanceAttenuationBuffer.GetData();

			// if this is a bus, we just want to copy the bus audio to this source's output audio
			// Note we need to copy this since bus instances may have different audio via dynamic source effects, etc.
			if (bIsSourceBus)
			{
				// Get the source's rendered and mixed audio bus data
				const TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(SourceInfo.AudioBusId);
				if (AudioBusPtr.IsValid())
				{
					int32 NumFramesPlayed = NumOutputFrames;
					if (SourceInfo.SourceBusDurationFrames != INDEX_NONE)
					{
						// If we're now finishing, only copy over the real data
						if ((SourceInfo.NumFramesPlayed + NumOutputFrames) >= SourceInfo.SourceBusDurationFrames)
						{
							NumFramesPlayed = SourceInfo.SourceBusDurationFrames - SourceInfo.NumFramesPlayed;
							SourceInfo.bIsLastBuffer = true;
						}
					}

					SourceInfo.NumFramesPlayed += NumFramesPlayed;

					// Retrieve the channel map of going from the audio bus channel count to the source channel count since they may not match
					int32 NumAudioBusChannels = AudioBusPtr->GetNumChannels();
					if (NumAudioBusChannels != SourceInfo.NumInputChannels)
					{
						Audio::FAlignedFloatBuffer ChannelMap;
						MixerDevice->Get2DChannelMap(SourceInfo.bIsVorbis, AudioBusPtr->GetNumChannels(), SourceInfo.NumInputChannels, SourceInfo.bIsCenterChannelOnly, ChannelMap);
						AudioBusPtr->CopyCurrentBuffer(ChannelMap, SourceInfo.NumInputChannels, SourceInfo.PreDistanceAttenuationBuffer, NumFramesPlayed);
					}
					else
					{
						AudioBusPtr->CopyCurrentBuffer(SourceInfo.NumInputChannels, SourceInfo.PreDistanceAttenuationBuffer, NumFramesPlayed);
					}
				}
			}
			else
			{

#if AUDIO_SUBFRAME_ENABLED
				// If we're not going to start yet, just continue
				double StartFraction = (SourceInfo.StartTime - AudioRenderThreadTime) / AudioClockDelta;
				if (StartFraction >= 1.0)
				{
					// note this is already zero'd so no need to write zeroes
					SourceInfo.PitchSourceParam.Reset();
					continue;
				}

				// Init the frame index iterator to 0 (i.e. render whole buffer)
				int32 StartFrame = 0;

				// If the start fraction is greater than 0.0 (and is less than 1.0), we are starting on a sub-frame
				// Otherwise, just start playing it right away
				if (StartFraction > 0.0)
				{
					StartFrame = NumOutputFrames * StartFraction;
				}

				// Update sample index to the frame we're starting on, accounting for source channels
				int32 SampleIndex = StartFrame * SourceInfo.NumInputChannels;
				bool bWriteOutZeros = true;
#else
				int32 SampleIndex = 0;
				int32 StartFrame = 0;
#endif

				// Modulate parameter target should modulation be active
				// Due to managing two separate pitch values that are updated at different rates
				// (game thread rate and copy set by SetPitch and buffer callback rate set by Modulation System),
				// the PitchSourceParam's target is marshaled before processing by mult'ing in the modulation pitch,
				// processing the buffer, and then resetting it back if modulation is active. 

				const bool bModActive = MixerDevice->IsModulationPluginEnabled() && MixerDevice->ModulationInterface.IsValid();
				if (bModActive)
				{
					SourceInfo.PitchModulation.ProcessControl(SourceInfo.PitchModulationBase);
				}

				const float TargetPitch = SourceInfo.PitchSourceParam.GetTarget();
				// Convert from semitones to frequency multiplier
				const float ModPitch = bModActive
					? Audio::GetFrequencyMultiplier(SourceInfo.PitchModulation.GetValue())
					: 1.0f;
				const float FinalPitch = FMath::Clamp(TargetPitch * ModPitch, MinModulationPitchRangeFreqCVar, MaxModulationPitchRangeFreqCVar);
				SourceInfo.PitchSourceParam.SetValue(FinalPitch, NumOutputFrames);

#if ATOM_PROFILERTRACE_ENABLED
				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AudioMixerChannel);
				if (bChannelEnabled)
				{
					UE_TRACE_LOG(Audio, MixerSourcePitch, AudioMixerChannel)
						<< MixerSourcePitch.DeviceId(MixerDevice->DeviceID)
						<< MixerSourcePitch.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourcePitch.PlayOrder(SourceInfo.PlayOrder)
						<< MixerSourcePitch.Pitch(TargetPitch);
				}
#endif // ATOM_PROFILERTRACE_ENABLED

				for (int32 Frame = StartFrame; Frame < NumOutputFrames; ++Frame)
				{
					// If we've read our last buffer, we're done
					if (SourceInfo.bIsLastBuffer)
					{
						break;
					}

					// Whether or not we need to read another sample from the source buffers
					// If we haven't yet played any frames, then we will need to read the first source samples no matter what
					bool bReadNextSample = !SourceInfo.bHasStarted;

					// Reset that we've started generating audio
					SourceInfo.bHasStarted = true;

					// Update the PrevFrameIndex value for the source based on alpha value
					if (SourceInfo.CurrentFrameAlpha >= 1.0f)
					{
						// Our inter-frame alpha lerping value is causing us to read new source frames
						bReadNextSample = true;
						
						const float Delta = FMath::FloorToFloat(SourceInfo.CurrentFrameAlpha);
						const int DeltaInt = (int)Delta;

						// Bump up the current frame index
						SourceInfo.CurrentFrameIndex += DeltaInt;

						// Bump up the frames played -- this is tracking the total frames in source file played
						// CurrentFrameIndex can wrap for looping sounds so won't be accurate in that case
						SourceInfo.NumFramesPlayed += DeltaInt;

						SourceInfo.CurrentFrameAlpha -= Delta;
					}

					// If our alpha parameter caused us to jump to a new source frame, we need
					// read new samples into our prev and next frame sample data
					if (bReadNextSample)
					{
						ReadSourceFrame(SourceId);
					}

					// perform linear SRC to get the next sample value from the decoded buffer
					if (SourceInfo.SubCallbackDelayLengthInFrames == 0)
					{
						for (int32 Channel = 0; Channel < SourceInfo.NumInputChannels; ++Channel)
						{
							const float CurrFrameValue = SourceInfo.CurrentFrameValues[Channel];
							const float NextFrameValue = SourceInfo.NextFrameValues[Channel];
							const float CurrentAlpha = SourceInfo.CurrentFrameAlpha;
							PreDistanceAttenBufferPtr[SampleIndex++] = FMath::Lerp(CurrFrameValue, NextFrameValue, CurrentAlpha);
						}
					}
					else
					{
						for (int32 Channel = 0; Channel < SourceInfo.NumInputChannels; ++Channel)
						{
							const float CurrFrameValue = SourceInfo.CurrentFrameValues[Channel];
							const float NextFrameValue = SourceInfo.NextFrameValues[Channel];
							const float CurrentAlpha = SourceInfo.CurrentFrameAlpha;

							const float CurrentSample = FMath::Lerp(CurrFrameValue, NextFrameValue, CurrentAlpha);

							SourceInfo.SourceBufferDelayLine.Push(&CurrentSample, 1);
							SourceInfo.SourceBufferDelayLine.Pop(&PreDistanceAttenBufferPtr[SampleIndex++], 1);
						}
					}

					const float CurrentPitchScale = SourceInfo.PitchSourceParam.Update();
					SourceInfo.CurrentFrameAlpha += CurrentPitchScale;
				}

				// After processing the frames, reset the pitch param
				SourceInfo.PitchSourceParam.Reset();

				// Reset target value as modulation may have modified prior to processing
				// And source param should not store modulation value internally as its
				// processed by the modulation plugin independently.
				SourceInfo.PitchSourceParam.SetValue(TargetPitch, NumOutputFrames);
			}
		}
	}*/

	void FMixerSourceManager::ConnectBusPatches()
	{
		while (TOptional<FPendingAudioBusConnection> PendingAudioBusConnection = PendingAudioBusConnections.Dequeue())
		{
			FAudioBusKey& AudioBusKey = PendingAudioBusConnection->AudioBusKey;
			int32 NumChannels = PendingAudioBusConnection->NumChannels;
			bool bIsAutomatic = PendingAudioBusConnection->bIsAutomatic;

			bool bAlreadyExisted = false;
			TSharedRef<FMixerAudioBus> AudioBusPtr = FindOrAddAudioBus(AudioBusKey, NumChannels, bIsAutomatic, &bAlreadyExisted);
			if (bAlreadyExisted)
			{
				// If this audio bus already existed, make sure the num channels lines up
				ensure(AudioBusPtr->GetNumChannels() == NumChannels);
				AudioBusPtr->SetAutomatic(bIsAutomatic);
			}

			switch (PendingAudioBusConnection->PatchVariant.GetIndex())
			{
			case FPendingAudioBusConnection::FPatchVariant::IndexOfType<Audio::FPatchInput>():
				AudioBusPtr->AddNewPatchInput(PendingAudioBusConnection->PatchVariant.Get<Audio::FPatchInput>());
				break;
			case FPendingAudioBusConnection::FPatchVariant::IndexOfType<Audio::FPatchOutputStrongPtr>():
				AudioBusPtr->AddNewPatchOutput(PendingAudioBusConnection->PatchVariant.Get<Audio::FPatchOutputStrongPtr>());
				break;
			}
		}
	}

	// this is processed on server thread
	void FMixerSourceManager::ComputeBuses()
	{
		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::ComputeBusses;

		ConnectBusPatches();

		// Loop through the bus registry and mix source audio
		for (auto& Entry : AudioBuses)
		{
			TSharedPtr<FMixerAudioBus>& AudioBus = Entry.Value;
			AudioBus->MixBuffer();
		}
	}

	void FMixerSourceManager::UpdateBuses()
	{
		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::UpdateBusses;

		// Update the bus states post mixing. This flips the current/previous buffer indices.
		for (auto& [AudioBusKey, AudioBus] : AudioBuses)
		{
			AudioBus->Update();
		
#if ATOM_PROFILERTRACE_ENABLED
			const FAlignedFloatBuffer& AudioBuffer = AudioBus->GetCurrentBusBuffer();
			const int32 NumSamples = AudioBus->NumChannels * AudioBus->NumFrames;

			const bool bIsAudioBufferSilent = AudioBuffer.Num() <= 0 || IsAudioBufferSilent(AudioBuffer.GetData(), NumSamples);

			UE_TRACE_LOG(CriWareAtom, AudioBusHasActivity, AtomChannel)
				<< AudioBusHasActivity.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< AudioBusHasActivity.AudioBusID(AudioBusKey.ObjectId)
				<< AudioBusHasActivity.Timestamp(FPlatformTime::Cycles64())
				<< AudioBusHasActivity.HasActivity(!bIsAudioBufferSilent);
#endif // ATOM_PROFILERTRACE_ENABLED
		}
	}

	float FMixerSourceManager::GetFloatCompareTolerance() const
	{
		const float Percent = GetCommandQueueFillPercentage();
		return FMath::IsNearlyEqual(Percent, 1.0f) ? AtomFloatCompareMaxScaleCVar : AtomFloatCompareMinScaleCVar;
	}

	float FMixerSourceManager::GetCommandQueueFillPercentage() const
	{
		if (AtomNumCmdsConsideredFullCVar == 0)
		{
			return 0.f;
		}
		return static_cast<float>(FMath::Min(NumCommands.GetValue(), AtomNumCmdsConsideredFullCVar) / AtomNumCmdsConsideredFullCVar);
	}

	// ctor
	FMixerSourceManager::FAtomMixerThreadCommand::FAtomMixerThreadCommand(TFunction<void()>&& InFunction, const char* InDebugString, bool bInDeferExecution)
		: Function(MoveTemp(InFunction))
		, bDeferExecution(bInDeferExecution)
#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
		, DebugString(InDebugString)
#endif
	{
	}

	void FMixerSourceManager::FAtomMixerThreadCommand::operator()() const
	{
#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
		StartExecuteTimeInCycles = FPlatformTime::Cycles64();
#endif

		Function();

#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
		StartExecuteTimeInCycles = 0;
#endif
	}

	FString FMixerSourceManager::FAtomMixerThreadCommand::GetSafeDebugString() const
	{
#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
		return FString(DebugString ? ANSI_TO_TCHAR(DebugString) : TEXT(""));
#else
		return {};
#endif
	}

	float FMixerSourceManager::FAtomMixerThreadCommand::GetExecuteTimeInSeconds() const
	{
#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
		return FPlatformTime::ToSeconds64(FPlatformTime::Cycles64() - StartExecuteTimeInCycles);
#else
		return 0.f;
#endif
	}

	/*void FMixerSourceManager::ApplyDistanceAttenuation(FSourceInfo& SourceInfo, int32 NumSamples)
	{
		if (DisableDistanceAttenuationCvar)
		{
			return;
		}

		TArrayView<float> PostDistanceAttenBufferView(SourceInfo.SourceBuffer.GetData(), SourceInfo.SourceBuffer.Num());
		Audio::ArrayFade(PostDistanceAttenBufferView, SourceInfo.DistanceAttenuationSourceStart, SourceInfo.DistanceAttenuationSourceDestination);
		SourceInfo.DistanceAttenuationSourceStart = SourceInfo.DistanceAttenuationSourceDestination;
	}*/

	/*void FMixerSourceManager::ComputePluginAudio(FSourceInfo& SourceInfo, FMixerSourceSubmixOutputBuffer& InSourceSubmixOutputBuffer, int32 SourceId, int32 NumSamples)
	{
		if (BypassAudioPluginsCvar)
		{
			// If we're bypassing audio plugins, our pre- and post-effect channels are the same as the input channels
			SourceInfo.NumPostEffectChannels = SourceInfo.NumInputChannels;

			// Set the ptr to use for post-effect buffers:
			InSourceSubmixOutputBuffer.SetPostAttenuationSourceBuffer(&SourceInfo.SourceBuffer);

			if (SourceInfo.bHasPreDistanceAttenuationSend)
			{
				InSourceSubmixOutputBuffer.SetPreAttenuationSourceBuffer(&SourceInfo.PreDistanceAttenuationBuffer);
			}
			return;
		}

		if (SourceInfo.AudioLink.IsValid())
		{
			IAudioLinkSourcePushed::FOnNewBufferParams Params;
			Params.SourceId = SourceId;
			Params.Buffer = SourceInfo.PreDistanceAttenuationBuffer;
			SourceInfo.AudioLink->OnNewBuffer(Params);
		}

		// If we have Source Buffer Listener
		if (SourceInfo.SourceBufferListener.IsValid())
		{
			// Pack all our state into a single struct.
			ISourceBufferListener::FOnNewBufferParams Params;
			Params.SourceId			= SourceId;
			Params.AudioData		= SourceInfo.PreDistanceAttenuationBuffer.GetData();
			Params.NumSamples		= SourceInfo.PreDistanceAttenuationBuffer.Num();
			Params.NumChannels		= SourceInfo.NumInputChannels;
			Params.SampleRate		= MixerDevice->GetSampleRate();

			// Fire callback.
			SourceInfo.SourceBufferListener->OnNewBuffer(Params);

			// Optionally, clear the buffer after we've broadcast it. 
			if (SourceInfo.bShouldSourceBufferListenerZeroBuffer)
			{
				FMemory::Memzero(SourceInfo.PreDistanceAttenuationBuffer.GetData(), SourceInfo.PreDistanceAttenuationBuffer.Num() * sizeof(float));
				FMemory::Memzero(SourceInfo.SourceBuffer.GetData(), SourceInfo.SourceBuffer.Num() * sizeof(float));
			}
		}

		float* PostDistanceAttenBufferPtr = SourceInfo.SourceBuffer.GetData();

		bool bShouldMixInReverb = false;
		if (SourceInfo.bUseReverbPlugin)
		{
			const FSpatializationParams* SourceSpatParams = &SourceInfo.SpatParams;

			// Move the audio buffer to the reverb plugin buffer
			FAudioPluginSourceInputData AudioPluginInputData;
			AudioPluginInputData.SourceId = SourceId;
			AudioPluginInputData.AudioBuffer = &SourceInfo.SourceBuffer;
			AudioPluginInputData.SpatializationParams = SourceSpatParams;
			AudioPluginInputData.NumChannels = SourceInfo.NumInputChannels;
			AudioPluginInputData.AudioComponentId = SourceInfo.AudioComponentID;
			SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
			SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

			MixerDevice->ReverbPluginInterface->ProcessSourceAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);

			// Make sure the buffer counts didn't change and are still the same size
			ATOM_MIXER_CHECK(SourceInfo.AudioPluginOutputData.AudioBuffer.Num() == NumSamples);

			//If the reverb effect doesn't send it's audio to an external device, mix the output data back in.
			if (!MixerDevice->bReverbIsExternalSend)
			{
				// Copy the reverb-processed data back to the source buffer
				InSourceSubmixOutputBuffer.CopyReverbPluginOutputData(SourceInfo.AudioPluginOutputData.AudioBuffer);
				bShouldMixInReverb = true;
			}
		}

		TArrayView<const float> ReverbPluginOutputBufferView(InSourceSubmixOutputBuffer.GetReverbPluginOutputData(), NumSamples);
		TArrayView<const float> AudioPluginOutputDataView(SourceInfo.AudioPluginOutputData.AudioBuffer.GetData(), NumSamples);
		TArrayView<float> PostDistanceAttenBufferView(PostDistanceAttenBufferPtr, NumSamples);

		if (SourceInfo.bUseOcclusionPlugin)
		{
			const FSpatializationParams* SourceSpatParams = &SourceInfo.SpatParams;

			// Move the audio buffer to the occlusion plugin buffer
			FAudioPluginSourceInputData AudioPluginInputData;
			AudioPluginInputData.SourceId = SourceId;
			AudioPluginInputData.AudioBuffer = &SourceInfo.SourceBuffer;
			AudioPluginInputData.SpatializationParams = SourceSpatParams;
			AudioPluginInputData.NumChannels = SourceInfo.NumInputChannels;
			AudioPluginInputData.AudioComponentId = SourceInfo.AudioComponentID;

			SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
			SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

			MixerDevice->OcclusionInterface->ProcessAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);

			// Make sure the buffer counts didn't change and are still the same size
			ATOM_MIXER_CHECK(SourceInfo.AudioPluginOutputData.AudioBuffer.Num() == NumSamples);

			// Copy the occlusion-processed data back to the source buffer and mix with the reverb plugin output buffer
			if (bShouldMixInReverb)
			{
				Audio::ArraySum(ReverbPluginOutputBufferView, AudioPluginOutputDataView, PostDistanceAttenBufferView);
			}
			else
			{
				FMemory::Memcpy(PostDistanceAttenBufferPtr, SourceInfo.AudioPluginOutputData.AudioBuffer.GetData(), sizeof(float) * NumSamples);
			}
		}
		else if (bShouldMixInReverb)
		{
			Audio::ArrayMixIn(ReverbPluginOutputBufferView, PostDistanceAttenBufferView);
		}

		// If the source has HRTF processing enabled, run it through the spatializer
		if (SourceInfo.bUseHRTFSpatializer)
		{
			CSV_SCOPED_TIMING_STAT(Audio, HRTF);
			SCOPE_CYCLE_COUNTER(STAT_AudioMixerHRTF);

			ATOM_MIXER_CHECK(SpatialInterfaceInfo.SpatializationPlugin.IsValid());
			ATOM_MIXER_CHECK(SourceInfo.NumInputChannels <= SpatialInterfaceInfo.MaxChannelsSupportedBySpatializationPlugin);

			FAudioPluginSourceInputData AudioPluginInputData;
			AudioPluginInputData.AudioBuffer = &SourceInfo.SourceBuffer;
			AudioPluginInputData.NumChannels = SourceInfo.NumInputChannels;
			AudioPluginInputData.SourceId = SourceId;
			AudioPluginInputData.SpatializationParams = &SourceInfo.SpatParams;

			if (!SpatialInterfaceInfo.bSpatializationIsExternalSend)
			{
				SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
				SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(2 * NumOutputFrames);
			}

			{
				LLM_SCOPE(ELLMTag::AudioMixerPlugins);
				SpatialInterfaceInfo.SpatializationPlugin->ProcessAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);
			}

			// If this is an external send, we treat this source audio as if it was still a mono source
			// This will allow it to traditionally pan in the ComputeOutputBuffers function and be
			// sent to submixes (e.g. reverb) panned and mixed down. Certain submixes will want this spatial 
			// information in addition to the external send. We've already bypassed adding this source
			// to a base submix (e.g. master/eq, etc)
			if (SpatialInterfaceInfo.bSpatializationIsExternalSend)
			{
				// Otherwise our pre- and post-effect channels are the same as the input channels
				SourceInfo.NumPostEffectChannels = SourceInfo.NumInputChannels;

				// Set the ptr to use for post-effect buffers rather than the plugin output data (since the plugin won't have output audio data)
				InSourceSubmixOutputBuffer.SetPostAttenuationSourceBuffer(&SourceInfo.SourceBuffer);

				if (SourceInfo.bHasPreDistanceAttenuationSend)
				{
					InSourceSubmixOutputBuffer.SetPreAttenuationSourceBuffer(&SourceInfo.PreDistanceAttenuationBuffer);
				}
			}
			else
			{
				// Otherwise, we are now a 2-channel file and should not be spatialized using normal 3d spatialization
				SourceInfo.NumPostEffectChannels = 2;

				// Set the ptr to use for post-effect buffers rather than the plugin output data (since the plugin won't have output audio data)
				InSourceSubmixOutputBuffer.SetPostAttenuationSourceBuffer(&SourceInfo.AudioPluginOutputData.AudioBuffer);

				if (SourceInfo.bHasPreDistanceAttenuationSend)
				{
					InSourceSubmixOutputBuffer.SetPreAttenuationSourceBuffer(&SourceInfo.PreDistanceAttenuationBuffer);
				}
			}
		}
		else
		{
			// Otherwise our pre- and post-effect channels are the same as the input channels
			SourceInfo.NumPostEffectChannels = SourceInfo.NumInputChannels;

			InSourceSubmixOutputBuffer.SetPostAttenuationSourceBuffer(&SourceInfo.SourceBuffer);

			if (SourceInfo.bHasPreDistanceAttenuationSend)
			{
				InSourceSubmixOutputBuffer.SetPreAttenuationSourceBuffer(&SourceInfo.PreDistanceAttenuationBuffer);
			}
		}
	}*/

	/*void FMixerSourceManager::ComputePostSourceEffectBufferForIdRange(bool bGenerateBuses, const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		CSV_SCOPED_TIMING_STAT(Audio, SourceEffectsBuffers);
		CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceEffectBuffers, (SourceIdStart < SourceIdEnd));

		const bool bIsDebugModeEnabled = DebugSoloSources.Num() > 0;

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || SourceInfo.bIsPaused || SourceInfo.bIsPausedForQuantization || (SourceInfo.bIsDone && SourceInfo.bEffectTailsDone))
			{
				continue;
			}

			const bool bIsSourceBus = SourceInfo.AudioBusId != INDEX_NONE;
			if ((bGenerateBuses && !bIsSourceBus) || (!bGenerateBuses && bIsSourceBus))
			{
				continue;
			}

			// Copy and store the current state of the pre-distance attenuation buffer before we feed it through our source effects
			// This is used by pre-effect sends
			if (SourceInfo.AudioBusSends[(int32)EAtomBusSendStage::PreEffect].Num() > 0)
			{
				SourceInfo.PreEffectBuffer.Reset();
				SourceInfo.PreEffectBuffer.Reserve(SourceInfo.PreDistanceAttenuationBuffer.Num());

				FMemory::Memcpy(SourceInfo.PreEffectBuffer.GetData(), SourceInfo.PreDistanceAttenuationBuffer.GetData(), sizeof(float)*SourceInfo.PreDistanceAttenuationBuffer.Num());
			}

			float* PreDistanceAttenBufferPtr = SourceInfo.PreDistanceAttenuationBuffer.GetData();
			const int32 NumSamples = SourceInfo.PreDistanceAttenuationBuffer.Num();

			TArrayView<float> PreDistanceAttenBufferView(PreDistanceAttenBufferPtr, NumSamples);

			// Update volume fade information if we're stopping
			{
				float VolumeStart = 1.0f;
				float VolumeDestination = 1.0f;
				if (SourceInfo.bIsStopping)
				{
					int32 NumFadeFrames = FMath::Min(SourceInfo.VolumeFadeNumFrames - SourceInfo.VolumeFadeFramePosition, NumOutputFrames);

					SourceInfo.VolumeFadeFramePosition += NumFadeFrames;
					SourceInfo.VolumeSourceDestination = SourceInfo.VolumeFadeSlope * (float)SourceInfo.VolumeFadeFramePosition + SourceInfo.VolumeFadeStart;

					if (FMath::IsNearlyZero(SourceInfo.VolumeSourceDestination, KINDA_SMALL_NUMBER))
					{
						SourceInfo.VolumeSourceDestination = 0.0f;
					}

					const int32 NumFadeSamples = NumFadeFrames * SourceInfo.NumInputChannels;

					VolumeStart = SourceInfo.VolumeSourceStart;
					VolumeDestination = SourceInfo.VolumeSourceDestination;
					if (MixerDevice->IsModulationPluginEnabled() && MixerDevice->ModulationInterface.IsValid())
					{
						const bool bHasProcessed = SourceInfo.VolumeModulation.GetHasProcessed();
						const float ModVolumeStart = SourceInfo.VolumeModulation.GetValue();
						SourceInfo.VolumeModulation.ProcessControl(SourceInfo.VolumeModulationBase);
						const float ModVolumeEnd = SourceInfo.VolumeModulation.GetValue();
						if (bHasProcessed)
						{
							VolumeStart *= ModVolumeStart;
						}
						else
						{
							VolumeStart *= ModVolumeEnd;
						}
						VolumeDestination *= ModVolumeEnd;
					}

					TArrayView<float> PreDistanceAttenBufferFadeSamplesView(PreDistanceAttenBufferPtr, NumFadeSamples);
					Audio::ArrayFade(PreDistanceAttenBufferFadeSamplesView, VolumeStart, VolumeDestination);

					// Zero the rest of the buffer
					if (NumFadeFrames < NumOutputFrames)
					{
						int32 SamplesLeft = NumSamples - NumFadeSamples;

						// Protect memzero call with some sanity checking on the inputs.
						if (SamplesLeft > 0 && NumFadeSamples >= 0 && NumFadeSamples < NumSamples)
						{
							FMemory::Memzero(&PreDistanceAttenBufferPtr[NumFadeSamples], sizeof(float) * SamplesLeft);
						}
					}
				}
				else
				{
					VolumeStart = SourceInfo.VolumeSourceStart;
					VolumeDestination = SourceInfo.VolumeSourceDestination;
					if (MixerDevice->IsModulationPluginEnabled() && MixerDevice->ModulationInterface.IsValid())
					{
						const bool bHasProcessed = SourceInfo.VolumeModulation.GetHasProcessed();
						const float ModVolumeStart = SourceInfo.VolumeModulation.GetValue();
						SourceInfo.VolumeModulation.ProcessControl(SourceInfo.VolumeModulationBase);
						const float ModVolumeEnd = SourceInfo.VolumeModulation.GetValue();
						if (bHasProcessed)
						{
							VolumeStart *= ModVolumeStart;
						}
						else
						{
							VolumeStart *= ModVolumeEnd;
						}
						VolumeDestination *= ModVolumeEnd;
					}

					Audio::ArrayFade(PreDistanceAttenBufferView, VolumeStart, VolumeDestination);
				}

#if ATOM_PROFILERTRACE_ENABLED
				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AudioMixerChannel);
				if (bChannelEnabled)
				{
					UE_TRACE_LOG(Audio, MixerSourceVolume, AudioMixerChannel)
						<< MixerSourceVolume.DeviceId(MixerDevice->DeviceID)
						<< MixerSourceVolume.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceVolume.PlayOrder(SourceInfo.PlayOrder)
						<< MixerSourceVolume.Volume(VolumeDestination);

					UE_TRACE_LOG(Audio, MixerSourceDistanceAttenuation, AudioMixerChannel)
						<< MixerSourceDistanceAttenuation.DeviceId(MixerDevice->DeviceID)
						<< MixerSourceDistanceAttenuation.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceDistanceAttenuation.PlayOrder(SourceInfo.PlayOrder)
						<< MixerSourceDistanceAttenuation.DistanceAttenuation(SourceInfo.DistanceAttenuationSourceDestination);
				}
#endif // ATOM_PROFILERTRACE_ENABLED
			}

			SourceInfo.VolumeSourceStart = SourceInfo.VolumeSourceDestination;

			// Now process the effect chain if it exists
			if (!DisableSourceEffectsCvar && SourceInfo.SourceEffects.Num() > 0)
			{
				// Prepare this source's effect chain input data
				SourceInfo.SourceEffectInputData.CurrentVolume = SourceInfo.VolumeSourceDestination;

				const float Pitch = Audio::GetFrequencyMultiplier(SourceInfo.PitchModulation.GetValue());
				SourceInfo.SourceEffectInputData.CurrentPitch = SourceInfo.PitchSourceParam.GetValue() * Pitch;
				SourceInfo.SourceEffectInputData.AudioClock = MixerDevice->GetAudioClock();
				if (SourceInfo.NumInputFrames > 0)
				{
					SourceInfo.SourceEffectInputData.CurrentPlayFraction = (float)SourceInfo.NumFramesPlayed / SourceInfo.NumInputFrames;
				}
				SourceInfo.SourceEffectInputData.SpatParams = SourceInfo.SpatParams;

				// Get a ptr to pre-distance attenuation buffer ptr
				float* OutputSourceEffectBufferPtr = SourceInfo.SourceEffectScratchBuffer.GetData();

				SourceInfo.SourceEffectInputData.InputSourceEffectBufferPtr = SourceInfo.PreDistanceAttenuationBuffer.GetData();
				SourceInfo.SourceEffectInputData.NumSamples = NumSamples;

				// Loop through the effect chain passing in buffers
				FScopeLock ScopeLock(&EffectChainMutationCriticalSection);
				{
					for (TSoundEffectSourcePtr& SoundEffectSource : SourceInfo.SourceEffects)
					{
						bool bPresetUpdated = false;
						if (SoundEffectSource->IsActive())
						{
							bPresetUpdated = SoundEffectSource->Update();
						}

						if (SoundEffectSource->IsActive())
						{
							SoundEffectSource->ProcessAudio(SourceInfo.SourceEffectInputData, OutputSourceEffectBufferPtr);

							// Copy output to input
							FMemory::Memcpy(SourceInfo.SourceEffectInputData.InputSourceEffectBufferPtr, OutputSourceEffectBufferPtr, sizeof(float) * NumSamples);
						}
					}
				}
			}

			const bool bWasEffectTailsDone = SourceInfo.bEffectTailsDone;

			if (!DisableEnvelopeFollowingCvar)
			{
				// Compute the source envelope using pre-distance attenuation buffer
				float AverageSampleValue = Audio::ArrayGetAverageAbsValue(PreDistanceAttenBufferView);
				SourceInfo.SourceEnvelopeValue = SourceInfo.SourceEnvelopeFollower.ProcessSample(AverageSampleValue);
				SourceInfo.SourceEnvelopeValue = FMath::Clamp(SourceInfo.SourceEnvelopeValue, 0.f, 1.f);

				SourceInfo.bEffectTailsDone = SourceInfo.bEffectTailsDone || SourceInfo.SourceEnvelopeValue < ENVELOPE_TAIL_THRESHOLD;
			}
			else
			{
				SourceInfo.bEffectTailsDone = true;
			}

			if (!bWasEffectTailsDone && SourceInfo.bEffectTailsDone)
			{
				SourceInfo.SourceListener->OnEffectTailsDone();
			}

			const bool bModActive = MixerDevice->IsModulationPluginEnabled() && MixerDevice->ModulationInterface.IsValid();
			bool bUpdateModFilters = bModActive && (SourceInfo.bModFiltersUpdated || SourceInfo.LowpassModulation.IsActive() || SourceInfo.HighpassModulation.IsActive());

			if (SourceInfo.IsRenderingToSubmixes() || bUpdateModFilters)
			{
				// Only scale with distance attenuation and send to source audio to plugins if we're not in output-to-bus only mode
				const int32 NumOutputSamplesThisSource = NumOutputFrames * SourceInfo.NumInputChannels;

				if (!SourceInfo.IsRenderingToSubmixes())
				{
					SourceInfo.LowpassModulation.ProcessControl(SourceInfo.LowpassModulationBase);
					SourceInfo.LowPassFilter.StartFrequencyInterpolation(SourceInfo.LowpassModulation.GetValue(), NumOutputFrames);

					SourceInfo.HighpassModulation.ProcessControl(SourceInfo.HighpassModulationBase);
					SourceInfo.HighPassFilter.StartFrequencyInterpolation(SourceInfo.HighpassModulation.GetValue(), NumOutputFrames);
				}
				else if (bUpdateModFilters)
				{
					const float LowpassFreq = FMath::Min(SourceInfo.LowpassModulationBase, SourceInfo.LowPassFreq);
					SourceInfo.LowpassModulation.ProcessControl(LowpassFreq);
					SourceInfo.LowPassFilter.StartFrequencyInterpolation(SourceInfo.LowpassModulation.GetValue(), NumOutputFrames);

					const float HighpassFreq = FMath::Max(SourceInfo.HighpassModulationBase, SourceInfo.HighPassFreq);
					SourceInfo.HighpassModulation.ProcessControl(HighpassFreq);
					SourceInfo.HighPassFilter.StartFrequencyInterpolation(SourceInfo.HighpassModulation.GetValue(), NumOutputFrames);
				}

				const bool bBypassLPF = DisableFilteringCvar || (SourceInfo.LowPassFilter.GetCutoffFrequency() >= (MAX_FILTER_FREQUENCY - KINDA_SMALL_NUMBER));
				const bool bBypassHPF = DisableFilteringCvar || DisableHPFilteringCvar || (SourceInfo.HighPassFilter.GetCutoffFrequency() <= (MIN_FILTER_FREQUENCY + KINDA_SMALL_NUMBER));

				float* SourceBuffer = SourceInfo.SourceBuffer.GetData();
				float* HpfInputBuffer = PreDistanceAttenBufferPtr; // assume bypassing LPF (HPF uses input buffer as input)

				if (!bBypassLPF)
				{
					// Not bypassing LPF, so tell HPF to use LPF output buffer as input
					HpfInputBuffer = SourceBuffer;

					// process LPF audio block
					SourceInfo.LowPassFilter.ProcessAudioBuffer(PreDistanceAttenBufferPtr, SourceBuffer, NumOutputSamplesThisSource);
				}

				if (!bBypassHPF)
				{
					// process HPF audio block
					SourceInfo.HighPassFilter.ProcessAudioBuffer(HpfInputBuffer, SourceBuffer, NumOutputSamplesThisSource);
				}

#if ATOM_PROFILERTRACE_ENABLED
				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AudioMixerChannel);
				if (bChannelEnabled)
				{
					float LPFFrequency = MAX_FILTER_FREQUENCY;
					if (!bBypassLPF)
					{
						LPFFrequency = SourceInfo.LowpassModulation.GetValue();
					}

					float HPFFrequency = MIN_FILTER_FREQUENCY;
					if (!bBypassHPF)
					{
						HPFFrequency = SourceInfo.HighpassModulation.GetValue();
					}

					UE_TRACE_LOG(Audio, MixerSourceFilters, AudioMixerChannel)
						<< MixerSourceFilters.DeviceId(MixerDevice->DeviceID)
						<< MixerSourceFilters.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceFilters.PlayOrder(SourceInfo.PlayOrder)
						<< MixerSourceFilters.HPFFrequency(HPFFrequency)
						<< MixerSourceFilters.LPFFrequency(LPFFrequency);
					UE_TRACE_LOG(Audio, MixerSourceEnvelope, AudioMixerChannel)
						<< MixerSourceEnvelope.DeviceId(MixerDevice->DeviceID)
						<< MixerSourceEnvelope.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceEnvelope.PlayOrder(SourceInfo.PlayOrder)
						<< MixerSourceEnvelope.Envelope(SourceInfo.SourceEnvelopeValue);
				}
#endif // ATOM_PROFILERTRACE_ENABLED

				// We manually reset interpolation to avoid branches in filter code
				SourceInfo.LowPassFilter.StopFrequencyInterpolation();
				SourceInfo.HighPassFilter.StopFrequencyInterpolation();

				if (bBypassLPF && bBypassHPF)
				{
					FMemory::Memcpy(SourceBuffer, PreDistanceAttenBufferPtr, NumSamples * sizeof(float));
				}
			}

			if (SourceInfo.IsRenderingToSubmixes() || SpatialInterfaceInfo.bSpatializationIsExternalSend)
			{
				// Apply distance attenuation
				ApplyDistanceAttenuation(SourceInfo, NumSamples);

				FMixerSourceSubmixOutputBuffer& SourceSubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];

				// Send source audio to plugins
				ComputePluginAudio(SourceInfo, SourceSubmixOutputBuffer, SourceId, NumSamples);
			}

			// Check the source effect tails condition
			if (SourceInfo.bIsLastBuffer && SourceInfo.bEffectTailsDone)
			{
				// If we're done and our tails our done, clear everything out
				SourceInfo.CurrentFrameValues.Reset();
				SourceInfo.NextFrameValues.Reset();
				SourceInfo.CurrentPCMBuffer = nullptr;
			}
		}
	}*/

	/*void FMixerSourceManager::ComputeOutputBuffersForIdRange(const bool bGenerateBuses, const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		CSV_SCOPED_TIMING_STAT(Audio, SourceOutputBuffers);
		CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceOutputBuffers, (SourceIdStart < SourceIdEnd));

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Don't need to compute anything if the source is not playing or paused (it will remain at 0.0 volume)
			// Note that effect chains will still be able to continue to compute audio output. The source output 
			// will simply stop being read from.
			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || (SourceInfo.bIsDone && SourceInfo.bEffectTailsDone))
			{
				continue;
			}

			// If we're in generate buses mode and not a bus, or vice versa, or if we're set to only output audio to buses.
			// If set to output buses, no need to do any panning for the source. The buses will do the panning.
			const bool bIsSourceBus = SourceInfo.AudioBusId != INDEX_NONE;
			if ((bGenerateBuses && !bIsSourceBus) || (!bGenerateBuses && bIsSourceBus) || !SourceInfo.IsRenderingToSubmixes())
			{
				continue;
			}

			FMixerSourceSubmixOutputBuffer& SourceSubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];
			SourceSubmixOutputBuffer.ComputeOutput(SourceInfo.SpatParams);
		}
	}*/

	/*void FMixerSourceManager::GenerateSourceAudio(const bool bGenerateBuses, const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		// Buses generate their input buffers independently
		// Get the next block of frames from the source buffers
		ComputeSourceBuffersForIdRange(bGenerateBuses, SourceIdStart, SourceIdEnd);

		// Compute the audio source buffers after their individual effect chain processing
		ComputePostSourceEffectBufferForIdRange(bGenerateBuses, SourceIdStart, SourceIdEnd);

		// Get the audio for the output buffers
		ComputeOutputBuffersForIdRange(bGenerateBuses, SourceIdStart, SourceIdEnd);
	}*/

	
	void FMixerSourceManager::GenerateSourceAudio(const bool bGenerateBuses)
	{
		RenderThreadPhase = bGenerateBuses ?
			EAtomSourceManagerRenderThreadPhase::GenerateSrcAudio_WithBusses :
			EAtomSourceManagerRenderThreadPhase::GenerateSrcAudio_WithoutBusses;

		// cache source buffers for bus sends
		for (int SourceID = 0; SourceID < NumTotalSources; ++SourceID)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceID];

			if (SourceInfo.AudioBusSends[(int32)EAtomBusSendStage::PreEffect].Num() > 0)
			{
				uint32 MaxNumSamples = NumOutputFrames * SourceInfo.NumInputChannels;
				
#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
				FScopeLock Lock(&SourceInfo.SendsBufferCS);
				MaxNumSamples = FMath::Min(SourceInfo.SendsBuffer.Num(), MaxNumSamples);
				SourceInfo.PreEffectBuffer.SetNumUninitialized(MaxNumSamples);
				int NumSamples = SourceInfo.SendsBuffer.Pop(SourceInfo.PreEffectBuffer.GetData(), MaxNumSamples);
#endif
			}
		}
	}

	/*void FMixerSourceManager::GenerateSourceAudio(const bool bGenerateBuses)
	{
		// If there are no buses, don't need to do anything here
		if (bGenerateBuses && !AudioBuses.Num())
		{
			return;
		}

		if (NumSourceWorkers > 0 && !DisableParallelSourceProcessingCvar)
		{
			ATOM_MIXER_CHECK(SourceWorkers.Num() == NumSourceWorkers);
			for (int32 i = 0; i < SourceWorkers.Num(); ++i)
			{
				FAudioMixerSourceWorker& Worker = SourceWorkers[i]->GetTask();
				Worker.SetGenerateBuses(bGenerateBuses);

				SourceWorkers[i]->StartBackgroundTask();
			}

			for (int32 i = 0; i < SourceWorkers.Num(); ++i)
			{
				SourceWorkers[i]->EnsureCompletion();
			}
		}
		else
		{
			GenerateSourceAudio(bGenerateBuses, 0, NumTotalSources);
		}
	}*/

	/*void FMixerSourceManager::MixOutputBuffers(const int32 SourceId, int32 InNumOutputChannels, const float InSendLevel, EMixerSourceSubmixSendStage InSubmixSendStage, FAlignedFloatBuffer& OutWetBuffer) const
	{
		if (InSendLevel > 0.0f)
		{
			const FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Don't need to mix into submixes if the source is paused
			if (!SourceInfo.bIsPaused && !SourceInfo.bIsPausedForQuantization && !SourceInfo.bIsDone && SourceInfo.bIsPlaying)
			{
				const FMixerSourceSubmixOutputBuffer& SourceSubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];
				SourceSubmixOutputBuffer.MixOutput(InSendLevel, InSubmixSendStage, OutWetBuffer);
			}
		}
	}*/

	void FMixerSourceManager::Get2DChannelMap(const int32 SourceID, int32 InNumOutputChannels, Atom::FAlignedFloatBuffer& OutChannelMap)
	{
		ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		const FSourceInfo& SourceInfo = SourceInfos[SourceID];
		AtomRuntime->Get2DChannelMap(SourceInfo.bIsVorbis, SourceInfo.NumInputChannels, InNumOutputChannels, SourceInfo.bIsCenterChannelOnly, OutChannelMap);
	}

	/*const ISoundfieldAudioPacket* FMixerSourceManager::GetEncodedOutput(const int32 SourceId, const FSoundfieldEncodingKey& InKey) const
	{
		ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		const FSourceInfo& SourceInfo = SourceInfos[SourceId];

		// Don't need to mix into submixes if the source is paused
		if (!SourceInfo.bIsPaused && !SourceInfo.bIsPausedForQuantization && !SourceInfo.bIsDone && SourceInfo.bIsPlaying)
		{
			const FMixerSourceSubmixOutputBuffer& SourceSubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];
			return SourceSubmixOutputBuffer.GetSoundfieldPacket(InKey);
		}

		return nullptr;
	}

	const FQuat FMixerSourceManager::GetListenerRotation(const int32 SourceId) const
	{
		const FMixerSourceSubmixOutputBuffer& SubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];
		return SubmixOutputBuffer.GetListenerRotation();
	}

	void FMixerSourceManager::UpdateDeviceChannelCount(const int32 InNumOutputChannels)
	{
		AtomMixerThreadCommand([this, InNumOutputChannels]()
		{
			NumOutputSamples = NumOutputFrames * MixerDevice->GetNumDeviceChannels();

			// Update all source's to appropriate channel maps
			for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
			{
				FSourceInfo& SourceInfo = SourceInfos[SourceId];

				// Don't need to do anything if it's not active or not paused. 
				if (!SourceInfo.bIsActive && !SourceInfo.bIsPaused)
				{
					continue;
				}

				FMixerSourceSubmixOutputBuffer& SourceSubmixOutputBuffer = SourceSubmixOutputBuffers[SourceId];
				SourceSubmixOutputBuffer.SetNumOutputChannels(InNumOutputChannels);

				SourceInfo.ScratchChannelMap.Reset();
				const int32 NumSourceChannels = SourceInfo.bUseHRTFSpatializer ? 2 : SourceInfo.NumInputChannels;

				// If this is a 3d source, then just zero out the channel map, it'll cause a temporary blip
				// but it should reset in the next tick
				if (SourceInfo.bIs3D)
				{
					GameThreadInfo.bNeedsSpeakerMap[SourceId] = true;
					SourceInfo.ScratchChannelMap.AddZeroed(NumSourceChannels * InNumOutputChannels);
				}
				// If it's a 2D sound, then just get a new channel map appropriate for the new device channel count
				else
				{
					SourceInfo.ScratchChannelMap.Reset();
					MixerDevice->Get2DChannelMap(SourceInfo.bIsVorbis, NumSourceChannels, InNumOutputChannels, SourceInfo.bIsCenterChannelOnly, SourceInfo.ScratchChannelMap);
				}

				SourceSubmixOutputBuffer.SetChannelMap(SourceInfo.ScratchChannelMap, SourceInfo.bIsCenterChannelOnly);
			}
		});
	}*/

	/*void FMixerSourceManager::UpdateSourceEffectChain(const uint32 InSourceEffectChainId, const TArray<FSourceEffectChainEntry>& InSourceEffectChain, const bool bPlayEffectChainTails)
	{
		AtomMixerThreadCommand([this, InSourceEffectChainId, InSourceEffectChain, bPlayEffectChainTails]()
		{
			FSoundEffectSourceInitData InitData;
			InitData.AudioClock = MixerDevice->GetAudioClock();
			InitData.SampleRate = MixerDevice->SampleRate;
			InitData.AudioDeviceId = MixerDevice->DeviceID;

			for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
			{
				FSourceInfo& SourceInfo = SourceInfos[SourceId];

				if (SourceInfo.SourceEffectChainId == InSourceEffectChainId)
				{
					SourceInfo.bEffectTailsDone = !bPlayEffectChainTails;

					// Check to see if the chain didn't actually change
					FScopeLock ScopeLock(&EffectChainMutationCriticalSection);
					{
						TArray<TSoundEffectSourcePtr>& ThisSourceEffectChain = SourceInfo.SourceEffects;
						bool bReset = false;
						if (InSourceEffectChain.Num() == ThisSourceEffectChain.Num())
						{
							for (int32 SourceEffectId = 0; SourceEffectId < ThisSourceEffectChain.Num(); ++SourceEffectId)
							{
								const FSourceEffectChainEntry& ChainEntry = InSourceEffectChain[SourceEffectId];

								TSoundEffectSourcePtr SourceEffectInstance = ThisSourceEffectChain[SourceEffectId];
								if (!SourceEffectInstance->IsPreset(ChainEntry.Preset))
								{
									// As soon as one of the effects change or is not the same, then we need to rebuild the effect graph
									bReset = true;
									break;
								}

								// Otherwise just update if it's just to bypass
								SourceEffectInstance->SetEnabled(!ChainEntry.bBypass);
							}
						}
						else
						{
							bReset = true;
						}

						if (bReset)
						{
							InitData.NumSourceChannels = SourceInfo.NumInputChannels;

							// First reset the source effect chain
							ResetSourceEffectChain(SourceId);

							// Rebuild it
							TArray<TSoundEffectSourcePtr> SourceEffects;
							BuildSourceEffectChain(SourceId, InitData, InSourceEffectChain, SourceEffects);

							SourceInfo.SourceEffects = SourceEffects;
							SourceInfo.SourceEffectPresets.Add(nullptr);
						}
					}
				}
			}
		}, *//*bDeferExecution*//*true);
	}*/

	void FMixerSourceManager::PauseSoundForQuantizationCommand(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		SourceInfo.bIsPausedForQuantization = true;
		SourceInfo.bIsActive = false;
	}

	void FMixerSourceManager::SetSubBufferDelayForSound(const int32 SourceID, const int32 FramesToDelay)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		SourceInfo.SubCallbackDelayLengthInFrames = FramesToDelay;
	}

	void FMixerSourceManager::UnPauseSoundForQuantizationCommand(const int32 SourceID)
	{
		ATOM_MIXER_CHECK(SourceID < NumTotalSources);
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		FSourceInfo& SourceInfo = SourceInfos[SourceID];

		SourceInfo.bIsPausedForQuantization = false;
		SourceInfo.bIsActive = !SourceInfo.bIsPaused;

		// ADX
		if (SourceInfo.MixerSourcePlayer.Player.IsValid())
		{
			SourceInfo.MixerSourcePlayer.Player->Resume(SourceInfo.MixerSourcePlayer.PlaybackID);
		}

		SourceInfo.QuantizedCommandHandle.Reset();
	}

	static const Atom::FAlignedFloatBuffer EmptyBuffer;

#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
	const Atom::FAlignedFloatBuffer& FMixerSourceManager::GetPreDistanceAttenuationBuffer(const int32 SourceID) const
	{
		const FSourceInfo& SourceInfo = SourceInfos[SourceID];

		if (SourceInfo.bIsPaused || SourceInfo.bIsPausedForQuantization)
		{
			return EmptyBuffer;
		}

		return SourceInfo.PreEffectBuffer;
		//return SourceInfo.PreDistanceAttenuationBuffer.GetData();
	}

	const Atom::FAlignedFloatBuffer& FMixerSourceManager::GetPreEffectBuffer(const int32 SourceID) const
	{
		const FSourceInfo& SourceInfo = SourceInfos[SourceID];

		if (SourceInfo.bIsPaused || SourceInfo.bIsPausedForQuantization)
		{
			return EmptyBuffer;
		}
	
		return SourceInfo.PreEffectBuffer;
	}
#endif

	const Atom::FAlignedFloatBuffer& FMixerSourceManager::GetPreviousSourceBusBuffer(const int32 SourceID) const
	{
		if (SourceID < SourceInfos.Num())
		{
			return GetPreviousAudioBusBuffer(SourceInfos[SourceID].AudioBusID);
		}
		return EmptyBuffer;
	}

	const Atom::FAlignedFloatBuffer& FMixerSourceManager::GetPreviousAudioBusBuffer(const int32 AudioBusID) const
	{
		// This is only called from within a scope-lock
		const TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(AudioBusID);
		if (AudioBusPtr.IsValid())
		{
			return AudioBusPtr->GetPreviousBusBuffer();
		}
		return EmptyBuffer;
	}

	int32 FMixerSourceManager::GetNumChannels(const int32 SourceId) const
	{
		return SourceInfos[SourceId].NumInputChannels;
	}

	bool FMixerSourceManager::IsSourceBus(const int32 SourceId) const
	{
		return SourceInfos[SourceId].AudioBusID != INDEX_NONE;
	}

	// Process Audio Streams
	void FMixerSourceManager::ComputeNextBlockOfSamples()
	{
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		CSV_SCOPED_TIMING_STAT(Atom, SourceManagerUpdate);
		SCOPE_CYCLE_COUNTER(STAT_AtomMixerSourceManagerUpdate);
		CSV_CUSTOM_STAT(Atom, NumActiveSources, NumActiveSources, ECsvCustomStatOp::Set);

		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::Begin;

		if (FPlatformProcess::SupportsMultithreading())
		{
			// Get the this blocks commands before rendering audio
			PumpCommandQueue();
		}
		else if (bPumpQueue)
		{
			bPumpQueue = false;
			PumpCommandQueue();
		}

		// actully called in Runtime update
		// Notify modulation interface that we are beginning to update
		// RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::ProcessModulators;
		//if (auto ModulationSystem = AtomRuntime->GetAtomModulationSystem())
		//{
		//	ModulationSystem->ProcessModulators(AtomRuntime->GetRuntimeDeltaTime());
		//}

		// Update pending tasks and release them if they're finished
		UpdatePendingReleaseData();

		// First generate non-bus audio (bGenerateBuses = false)
		GenerateSourceAudio(false);

		// Now mix in the non-bus audio into the buses
		ComputeBuses();

		// Now generate bus audio (bGenerateBuses = true)
		//GenerateSourceAudio(true);

		// Update the buses now
		UpdateBuses();

		// Let the plugin know we finished processing all sources
		/*if (bUsingSpatializationPlugin)
		{
			RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::SpatialInterface_OnAllSourcesProcessed;
			ATOM_MIXER_CHECK(SpatialInterfaceInfo.SpatializationPlugin.IsValid());
			LLM_SCOPE(ELLMTag::AudioMixerPlugins);
			SpatialInterfaceInfo.SpatializationPlugin->OnAllSourcesProcessed();
		}*/

		// Let the plugin know we finished processing all sources
		if (bUsingSourceDataOverridePlugin)
		{
			RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::SourceDataOverride_OnAllSourcesProcessed;
			ATOM_MIXER_CHECK(SourceDataOverridePlugin.IsValid());
			LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixerPlugins);
			SourceDataOverridePlugin->OnAllSourcesProcessed();
		}

		// Update the game thread copy of source donenessf
		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::UpdateGameThreadCopies;
		/*for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{		
			FSourceInfo& SourceInfo = SourceInfos[SourceID];

			// Check for the stopping condition to "turn the sound off"
			if (SourceInfo.bIsLastBuffer)
			{
				if (!SourceInfo.bIsDone)
				{
					SourceInfo.bIsDone = true;

					// Notify that we're now done with this source
					SourceInfo.SourceListener->OnDone();

					if (SourceInfo.AudioLink)
					{
						SourceInfo.AudioLink->OnSourceDone(SourceId);
					}
				}
			}
		}*/

		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::Finished;
	}

	/*void FMixerSourceManager::ClearStoppingSounds()
	{
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			if (!SourceInfo.bIsDone && SourceInfo.bIsStopping && SourceInfo.VolumeSourceDestination == 0.0f)
			{
				SourceInfo.bIsStopping = false;
				SourceInfo.bIsDone = true;
				SourceInfo.SourceListener->OnDone();
				if (SourceInfo.AudioLink)
				{
					SourceInfo.AudioLink->OnSourceDone(SourceId);
				}
			}

		}
	}*/

	void FMixerSourceManager::AtomMixerThreadMPSCCommand(TFunction<void()>&& InCommand, const char* InDebugString)
	{
		MpscCommandQueue.Enqueue(FAtomMixerMpscCommand{ MoveTemp(InCommand), InDebugString, false });
	}

	void FMixerSourceManager::AtomMixerThreadCommand(TFunction<void()>&& InFunction, const char* InDebugString, bool bInDeferExecution /*= false*/)
	{
		FAtomMixerThreadCommand AudioCommand(MoveTemp(InFunction), InDebugString, bInDeferExecution);

		// collect values for debugging
		// outside of the ScopeLock so we can avoid doing a bunch of work that doesn't require the lock
		SIZE_T OldMax = 0;
		SIZE_T NewMax = 0;
		SIZE_T NewNum = 0;
		SIZE_T CurrentBufferSizeInBytes = 0;
		int32 AudioThreadCommandIndex = -1;
		{
			// Here, we make sure that we don't flip our command double buffer while modifying the command buffer
			FScopeLock ScopeLock(&CommandBufferIndexCriticalSection);
			ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

			// Add the function to the command queue:
			AudioThreadCommandIndex = !RenderThreadCommandBufferIndex.GetValue();
			FCommands& Commands = CommandBuffers[AudioThreadCommandIndex];

			OldMax = Commands.SourceCommandQueue.Max();

			// always add commands to the buffer. If we're not going to assert, might as well chug along and hope we can recover!
			Commands.SourceCommandQueue.Add(AudioCommand);
			NumCommands.Increment();

			NewNum = Commands.SourceCommandQueue.Num();
			NewMax = Commands.SourceCommandQueue.Max();
			CurrentBufferSizeInBytes = Commands.SourceCommandQueue.GetAllocatedSize();
		}

		// log warnings for command buffer growing too large
		if (OldMax != NewMax)
		{
			// Only throw a warning every time we have to reallocate, which will be less often then every single time we add
			static SIZE_T WarnSize = 1024 * 1024;
			if (CurrentBufferSizeInBytes > WarnSize)
			{
				float TimeSinceLastComplete = FPlatformTime::ToSeconds64(FPlatformTime::Cycles64() - LastPumpCompleteTimeInCycles);

				UE_LOG(LogCriWareAtomMixer, Error, TEXT("Command Queue %d has grown to %ukb, containing %d cmds, last complete pump was %2.5f seconds ago."),
					AudioThreadCommandIndex, CurrentBufferSizeInBytes >> 10, NewNum, TimeSinceLastComplete);
				WarnSize *= 2;

				DoStallDiagnostics();
			}

			// check that we haven't gone over the max size
			const SIZE_T MaxBufferSizeInBytes = ((SIZE_T)AtomCommandBufferMaxSizeInMbCvar) << 20;
			if (CurrentBufferSizeInBytes >= MaxBufferSizeInBytes)
			{
				int32 NumTimesOvergrown = CommandBuffers[AudioThreadCommandIndex].NumTimesOvergrown.Increment();
				UE_LOG(LogCriWareAtomMixer, Error, TEXT("%d: Command buffer %d allocated size has grown to %umb! Likely cause the AudioRenderer has hung"),
					NumTimesOvergrown, AudioThreadCommandIndex, CurrentBufferSizeInBytes >> 20);
			}
		}

		// update trace values
		CSV_CUSTOM_STAT(Atom, AtomMixerThreadCommands, static_cast<int32>(NewNum), ECsvCustomStatOp::Set);
		TRACE_INT_VALUE(TEXT("AtomMixerThreadCommands::NumCommands"), NewNum);
		TRACE_INT_VALUE(TEXT("AtomMixerThreadCommands::CurrentBufferSizeInKb"), CurrentBufferSizeInBytes >> 10);
	}

	void FMixerSourceManager::PumpCommandQueue()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(AtomMixerThreadCommands::PumpCommandQueue)
		AtomRenderThreadID = FPlatformTLS::GetCurrentThreadId();
		
		// If we're already triggered, we need to wait for the audio thread to reset it before pumping
		if (FPlatformProcess::SupportsMultithreading())
		{
			if (CommandsProcessedEvent->Wait(0))
			{
				return;
			}
		}

		// Pump the MPSC command queue
		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::PumpMpscCmds;
		TOptional Opt{ MpscCommandQueue.Dequeue() };
		while (Opt.IsSet())
		{
			// First copy/move out the command and keep a copy of it.
			{
				FWriteScopeLock Lock(CurrentlyExecutingCmdLock);
				CurrentlyExecuteingCmd = MoveTemp(Opt.GetValue());
			}
			
			// Execute the current under a read-lock.
			{
				FReadScopeLock Lock(CurrentlyExecutingCmdLock);
				CurrentlyExecuteingCmd();
			}
				
			Opt = MpscCommandQueue.Dequeue();
		}

		int32 CurrentRenderThreadIndex = RenderThreadCommandBufferIndex.GetValue();
		FCommands& Commands = CommandBuffers[CurrentRenderThreadIndex];

		const int32 NumCommandsToExecute = Commands.SourceCommandQueue.Num();
		TRACE_INT_VALUE(TEXT("AtomMixerThreadCommands::NumCommandsToExecute"), NumCommandsToExecute);

		// Pop and execute all the commands that came since last update tick
		TArray<FAtomMixerThreadCommand> DelayedCommands;
		RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::PumpCmds;
		for (int32 Id = 0; Id < NumCommandsToExecute; ++Id)
		{
			// First copy/move out the command and keep a copy of it.
			{ 
				FWriteScopeLock Lock(CurrentlyExecutingCmdLock);
				CurrentlyExecuteingCmd = MoveTemp(Commands.SourceCommandQueue[Id]);
			}
			
			// Execute the current command or differ under a read-lock.
			{
				FReadScopeLock Lock(CurrentlyExecutingCmdLock);
				if (CurrentlyExecuteingCmd.bDeferExecution)
				{
					CurrentlyExecuteingCmd.bDeferExecution = false;
					DelayedCommands.Add(CurrentlyExecuteingCmd);
				}
				else
				{
					CurrentlyExecuteingCmd(); // execute
				}
			}
			
			NumCommands.Decrement();
		}

		LastPumpCompleteTimeInCycles = FPlatformTime::Cycles64();
		// This is intentionally re-assigning the Command Queue and clearing the buffer in the process
		Commands.SourceCommandQueue = MoveTemp(DelayedCommands);
		Commands.SourceCommandQueue.Reserve(GetCommandBufferInitialCapacity());

		if (FPlatformProcess::SupportsMultithreading())
		{
			check(CommandsProcessedEvent != nullptr);
			CommandsProcessedEvent->Trigger();
		}
		else
		{
			RenderThreadCommandBufferIndex.Set(!CurrentRenderThreadIndex);
		}
	}

	void FMixerSourceManager::FlushCommandQueue(bool bPumpInCommand)
	{
		check(CommandsProcessedEvent != nullptr);

		// If we have no commands enqueued, exit
		if (NumCommands.GetValue() == 0)
		{
			UE_LOG(LogCriWareAtomMixer, Verbose, TEXT("No commands were queued while flushing the source manager."));
			return;
		}

		// Make sure current current executing 
		bool bTimedOut = false;
		if (!CommandsProcessedEvent->Wait(AtomCommandBufferFlushWaitTimeMsCvar))
		{
			CommandsProcessedEvent->Trigger();
			bTimedOut = true;
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Timed out waiting to flush the source manager command queue (1)."));
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Verbose, TEXT("Flush succeeded in the source manager command queue (1)."));
		}

		// Call update to trigger a final pump of commands
		Update(bTimedOut);

		if (bPumpInCommand)
		{
			PumpCommandQueue();
		}

		// Wait one more time for the double pump
		if (!CommandsProcessedEvent->Wait(1000))
		{
			CommandsProcessedEvent->Trigger();
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Timed out waiting to flush the source manager command queue (2)."));
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Verbose, TEXT("Flush succeeded the source manager command queue (2)."));
		}
	}

	void FMixerSourceManager::UpdatePendingReleaseData(bool bForceWait)
	{
		// Don't block, but let tasks finish naturally
		for (int32 Index = PendingSourcePlayers.Num() - 1; Index >= 0; --Index)
		{
			FMixerSourcePlayer& MixerSourcePlayer = PendingSourcePlayers[Index];
			FAtomExPlayer* AtomExPlayer = MixerSourcePlayer.Player.Get();
			const FAtomPlaybackId AtomPlaybackID = MixerSourcePlayer.PlaybackID;

			bool bDeleteSourceBuffer = true;
			if (bForceWait)
			{
				// ask if sound is reaaly finished 
				//AtomExPlayer->EnsureAsyncTaskFinishes();
			}
			//else if (!AtomExPlayer->IsAsyncTaskDone())
			//{			
			//	bDeleteSourceBuffer = false;
			//}

			if (bDeleteSourceBuffer)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
				PendingSourcePlayers.RemoveAtSwap(Index, EAllowShrinking::No);
#else
				PendingSourcePlayers.RemoveAtSwap(Index, 1, false);
#endif
			}
		}
	}

	/*bool FMixerSourceManager::FSourceInfo::IsRenderingToSubmixes() const
	{
		return bEnableBaseSubmix || bEnableSubmixSends;
	}
	*/

	void FMixerSourceManager::DoStallDiagnostics()
	{
		LogRenderThreadStall();
		LogInflightAsyncTasks();
		LogCallstacks();
	}

	void FMixerSourceManager::LogRenderThreadStall()
	{
		// If we are in either of the Cmd pump phases dump the current command.
		if (RenderThreadPhase == EAtomSourceManagerRenderThreadPhase::PumpMpscCmds ||
			RenderThreadPhase == EAtomSourceManagerRenderThreadPhase::PumpCmds)
		{
			FReadScopeLock Lock(CurrentlyExecutingCmdLock);
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Stall in Cmd Queue: Cmd='%s', Executing For: %2.5f secs, AudioRenderThread='%s'"),
				*CurrentlyExecuteingCmd.GetSafeDebugString(), CurrentlyExecuteingCmd.GetExecuteTimeInSeconds(), ToCStr(LexToString(RenderThreadPhase)));
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Stall in AudioRenderThread Phase: '%s'"), ToCStr(LexToString(RenderThreadPhase)));
		}
	}

	void FMixerSourceManager::LogInflightAsyncTasks()
	{
		// NOTE: we iterate these lists without a lock, so this is somewhat dangerous!

		// Dump all in flight decodes/procedural sources.
		/*using FSrcBuffer = TSharedPtr<FMixerSourceBuffer, ESPMode::ThreadSafe>;
		TArray<FMixerSourceBuffer::FDiagnosticState> InflightTasks;
		for (FSourceInfo& i : SourceInfos)
		{
			if (i.MixerSourceBuffer.IsValid())
			{
				FMixerSourceBuffer::FDiagnosticState State;
				i.MixerSourceBuffer->GetDiagnosticState(State);
				if (State.bInFlight)
				{
					InflightTasks.Add(State);
				}
			}
		}
		for (FSrcBuffer& i : PendingSourceBuffers)
		{
			FMixerSourceBuffer::FDiagnosticState State;
			if (i.IsValid())
			{
				i->GetDiagnosticState(State);
				if (State.bInFlight)
				{
					InflightTasks.Add(State);
				}
			}
		}
		for (FMixerSourceBuffer::FDiagnosticState& i : InflightTasks)
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Inflight Task: %s, %2.2f secs, Procedural=%d"),
				*i.WaveName.ToString(), i.RunTimeInSecs, (int32)i.bProcedural);
		}*/
	}

	void FMixerSourceManager::LogCallstacks()
	{
		LogCallstack(AtomRenderThreadID);
	}

	void FMixerSourceManager::LogCallstack(uint32 InThreadId)
	{
		if (InThreadId != INVALID_ATOM_RENDER_THREAD_ID)
		{
			const SIZE_T StackTraceSize = 65536;
			ANSICHAR StackTrace[StackTraceSize] = { 0 };
			FPlatformStackWalk::ThreadStackWalkAndDump(StackTrace, StackTraceSize, 0, InThreadId);
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("***** ThreadStackWalkAndDump for ThreadID(%lu) ******\n%s"), InThreadId, ANSI_TO_TCHAR(StackTrace));
		}
	}

	TSharedRef<FMixerAudioBus> FMixerSourceManager::FindOrAddAudioBus(const FAudioBusKey BusKey, const int32 InNumChannels, const bool bIsAutomatic, bool* bOutAlreadyExisted)
	{
		TSharedPtr<FMixerAudioBus> AudioBusPtr = AudioBuses.FindRef(BusKey);
		if (bOutAlreadyExisted != nullptr)
		{
			*bOutAlreadyExisted = AudioBusPtr.IsValid();
		}
		if (!AudioBusPtr.IsValid())
		{
			AudioBusPtr = MakeShared<FMixerAudioBus>(this, BusKey, bIsAutomatic, InNumChannels);
			AudioBuses.Add(BusKey, AudioBusPtr);
		}

		return AudioBusPtr.ToSharedRef();
	}

	void FMixerSourceManager::IterateOverVoicePools(TFunctionRef<void(EMixerSourceVoiceCodecType CodecType, EMixerSourceVoiceStreamingType StreamingType, int32 ID, int32 NumUsedVoices, int32 MaxVoices, int32 MaxChannels, int32 MaxSampleRate)> Func) const
	{
		for (auto MixerSourceVoice : MixerSources)
		{
			if (MixerSourceVoice)
			{
				if (MixerSourceVoice->VoicePool.IsValid())
				{
					CriSint32 CurrentNum = 0;
					CriSint32 Limit = 0;
					FCriWareApi::criAtomExVoicePool_GetNumUsedVoices(MixerSourceVoice->VoicePool, &CurrentNum, &Limit);
					const int32 ID = MixerSourceVoice->CodecType != EMixerSourceVoiceCodecType::InputPort ? MixerSourceVoice->PoolID : MixerSourceVoice->SourceID;
					Func(MixerSourceVoice->CodecType, MixerSourceVoice->StreamingType, ID, (int32)CurrentNum, (int32)Limit, MixerSourceVoice->MaxInputChannels, MixerSourceVoice->MaxInputSampleRate);
				}
			}
		}
	}
} // namespace
