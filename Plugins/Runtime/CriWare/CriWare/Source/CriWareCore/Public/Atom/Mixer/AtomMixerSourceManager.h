
#pragma once

#include "Containers/MpscQueue.h"
#include "Stats/Stats.h"
#include "DSP/BufferVectorOperations.h"
#include "DSP/EnvelopeFollower.h"
#include "DSP/InterpolatedOnePole.h"
#include "DSP/ParamInterpolator.h"
#include "DSP/Dsp.h"
//#include "IAudioLink.h"
//#include "IAudioLinkFactory.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerBus.h"
#include "Atom/Mixer/AtomMixerPlayer.h"
#include "Atom/AtomQuartzQuantizationUtilities.h"

#if ATOM_PROFILERTRACE_ENABLED
#include "ProfilingDebugging/TraceAuxiliary.h"
#endif // ATOM_PROFILERTRACE_ENABLED

#include "AtomMixerSourceManager.generated.h"

// Default this to on (it's quite a small memory footprint).
#ifndef WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG
	#define WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG (1)
#endif //WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG

// Tracks the time it takes to up the source manager (computes source buffers, source effects, sample rate conversion)
DECLARE_CYCLE_STAT_EXTERN(TEXT("Source Manager Update"), STAT_AtomMixerSourceManagerUpdate, STATGROUP_AtomMixer, CRIWARECORE_API);

// Forward Definitions
struct FAtomSourceVoiceEffect;

// For diagnostics, keep track of what phase of updating the Source manager is in currently.
UENUM()
enum EAtomSourceManagerRenderThreadPhase : uint8
{
	Begin,

	PumpMpscCmds,
	PumpCmds,
	ProcessModulators,
	UpdatePendingReleaseData,
	GenerateSrcAudio_WithBusses,
	ComputeBusses,
	GenerateSrcAudio_WithoutBusses,
	UpdateBusses,
	SpatialInterface_OnAllSourcesProcessed,
	SourceDataOverride_OnAllSourcesProcessed,
	UpdateGameThreadCopies,

	Finished,
};

namespace Atom
{
	// Forward Definitions
	class FMixerSourceVoice;

	/** Struct defining a source voice buffer. */
	struct FMixerSourceVoiceBuffer
	{
		/** PCM float data. */
		FAlignedFloatBuffer AudioData;

		/** How many times this buffer will loop. */
		int32 LoopCount = 0;

		/** If this buffer is from real-time decoding and needs to make callbacks for more data. */
		bool bRealTimeBuffer = false;
	};

	class ISourceListener
	{
	public:
		virtual ~ISourceListener() = default;

		// Called before a source begins to generate audio. 
		virtual void OnBeginGenerate() = 0;

		// Called when a loop point is hit
		//virtual void OnLoopEnd() = 0;

		// Called when the source finishes on the ASR render thread
		virtual void OnDone() = 0;

		// Called when the source's effect tails finish on the ASR render thread.
		//virtual void OnEffectTailsDone() = 0;
	};

	// Struct holding mappings of bus ids (unique ids) to send level
	struct FInitAudioBusSend
	{
		uint32 AudioBusID = INDEX_NONE;
		float SendLevel = 0.0f;
		int32 BusChannels = 0;
#if ENABLE_ATOM_DEBUG
		FString AudioBusName;
#endif // ENABLE_ATOM_DEBUG
	};

	struct FSourceManagerInitParams
	{
		// Total number of sources to use in the source manager
		int32 NumSources = 0;

		// Number of worker threads to use for the source manager.
		int32 NumSourceWorkers = 0;
	};

	enum class EMixerSourceVoiceCodecType : uint32
	{
		Standard = 0, // HCA & ADX
		HcaMx,
		Wave,
		Aiff,
		RawPcm,
		InputPort,
		External,
		Audio3d,
		Opus,
		UserDefined, // Global (Standard)
		Undefined = 0xff
	};

	enum class EMixerSourceVoiceStreamingType : uint32
	{
		MemoryOnly,
		StreamOnly,
		Mixed
	};

	struct FMixerSourceVoiceInitParams
	{
		FMixerSourcePlayer MixerSourcePlayer;
		ISourceListener* SourceListener = nullptr;
		//TArray<FMixerSourceSubmixSend> SubmixSends;
		TArray<Atom::FInitAudioBusSend> AudioBusSends[(int32)EBusSendType::Count];
		uint32 AudioBusID = INDEX_NONE;
		int32 AudioBusChannels = 0;
#if ENABLE_ATOM_DEBUG
		FString AudioBusName;
#endif // ENABLE_ATOM_DEBUG
		float SourceBusDuration = 0.0f;
		//uint32 SourceEffectChainId = INDEX_NONE;
		//TArray<FSourceEffectChainEntry> SourceEffectChain;
		FMixerSourceVoice* SourceVoice = nullptr;
		int32 NumInputChannels = 0;
		int32 NumInputFrames = 0;
		float EnvelopeFollowerAttackTime = 10.0f;
		float EnvelopeFollowerReleaseTime = 100.0f;
		float TimeStretch = 0.0f;
		FString DebugName;
		//USpatializationPluginSourceSettingsBase* SpatializationPluginSettings = nullptr;
		//UOcclusionPluginSourceSettingsBase* OcclusionPluginSettings = nullptr;
		//UReverbPluginSourceSettingsBase* ReverbPluginSettings = nullptr;
		UAtomSourceDataOverridePluginSourceSettingsBase* SourceDataOverridePluginSettings = nullptr;

		FAtomSoundModulationDefaultSettings ModulationSettings;

		FQuartzQuantizedRequestData QuantizedRequestData;

		FSharedISourceBufferListenerPtr SourceBufferListener;

		//IBufferedAudioOutput
		//IAudioLinkFactory::FAudioLinkSourcePushedSharedPtr AudioLink;

		FName AtomComponentUserID;
		uint64 AtomComponentID = 0;
		bool bIs3D = false;
		bool bPlayEffectChainTails = false;
		bool bUseHRTFSpatialization = false;
		bool bIsExternalSend = false;
		bool bIsDebugMode = false;
		bool bEnableBusSends = false;
		bool bEnableBaseSubmix = false;
		bool bEnableSubmixSends = false;
		bool bIsVorbis = false;
		bool bIsSoundfield = false;
		//bool bIsSeeking = false;
		//bool bShouldSourceBufferListenerZeroBuffer = false;
		bool bUseFixedVoicePools = false;
		EAtomFormat InputFormat = EAtomFormat::None;
		int32 InputVoices = 1;
		int32 InputSampleRate = 48000;
		EMixerSourceVoiceStreamingType StreamingType = EMixerSourceVoiceStreamingType::MemoryOnly;

		FCriAtomVoicePoolPtr ExternalVoicePool = nullptr;

		uint32 PlayOrder = INDEX_NONE;
		uint32 ActiveSoundPlayOrder = INDEX_NONE;
	};

	// Atom sources and voice pool manager
	class FMixerSourceManager
	{
	public:

		FMixerSourceManager(FAtomRuntime* InAtomRuntime);
		~FMixerSourceManager();

		void Init(const FSourceManagerInitParams& InitParams);
		void Update(bool bTimedOut = false);

		bool GetFreeSourceID(int32& OutSourceID);
		int32 GetNumActiveSources() const;
		int32 GetNumActiveAudioBuses() const;

		void ReleaseSourceID(const int32 SourceID);
		void InitSource(const int32 SourceID, const FMixerSourceVoiceInitParams& InitParams);
		FAtomPlaybackId InitSourcePlayer(const int32 SourceID);

		// creates and starts an audio player manually. (AtomExPlayer) Must be called from the Atom thread.
		void StartAudioPlayer(FAtomExPlayerKey InAudioPlayerKey);

		// Stops an audio player manually. (AtomExPlayer) Must be called from the Atom thread.
		void StopAudioPlayer(FAtomExPlayerKey InAudioPlayerKey);

		// Queries if an audio player is active. Must be called from the Atom thread.
		bool IsAudioPlayerActive(FAtomExPlayerKey InAudioPlayerKey) const;

		// Creates and starts an audio bus manually.
		void StartAudioBus(FAudioBusKey InAudioBusKey, const FString& InAudioBusName, int32 InNumChannels, bool bInIsAutomatic);

		// Stops an audio bus manually
		void StopAudioBus(FAudioBusKey InAudioBusKey);

		// Queries if an audio bus is active. Must be called from the audio thread.
		bool IsAudioBusActive(FAudioBusKey InAudioBusKey) const;

		// Returns the number of channels currently set for the audio bus associated with
		// the provided BusId.  Returns 0 if the audio bus is inactive.
		int32 GetAudioBusNumChannels(FAudioBusKey InAudioBusKey) const;

		// Adds a patch output for an audio bus from the Audio Render Thread
		void AddPatchOutputForAudioBus(FAudioBusKey InAudioBusKey, const Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr);

		// Adds a patch output for an audio bus from the Audio Thread
		void AddPatchOutputForAudioBus_AudioThread(FAudioBusKey InAudioBusKey, const Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr);

		// Adds a patch input for an audio bus
		void AddPatchInputForAudioBus(FAudioBusKey InAudioBusKey, const Audio::FPatchInput& InPatchInput);

		// Adds a patch input for an audio bus from the Audio Thread
		void AddPatchInputForAudioBus_AudioThread(FAudioBusKey InAudioBusKey, const Audio::FPatchInput& InPatchInput);

		// Sets the voice effect for a given source.
		void SetVoiceEffect(const int32 SourceID, const FAtomSourceVoiceEffect& Effect);

		// Check if this source is using HRFT spatialization.
		bool IsUsingHRTFSpatializer(const int32 SourceID) const;

		void Play(const int32 SourceID);
		void Stop(const int32 SourceID);
		void CancelQuantizedSound(const int32 SourceID);
		void StopInternal(const int32 SourceID);
		void StopFade(const int32 SourceID/*, const int32 NumFrames*/);
		void Pause(const int32 SourceID);
		//void SetPitch(const int32 SourceID, const float Pitch);
		//void SetVolume(const int32 SourceID, const float Volume);
		//void SetDistanceAttenuation(const int32 SourceID, const float DistanceAttenuation);
		//void SetSpatializationParams(const int32 SourceID, const FSpatializationParams& InParams);
		//void SetChannelMap(const int32 SourceID, const uint32 NumInputChannels, const Audio::FAlignedFloatBuffer& InChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly);
		//void SetLPFFrequency(const int32 SourceID, const float Frequency);
		//void SetHPFFrequency(const int32 SourceID, const float Frequency);

		// Sets base (i.e. carrier) frequency of modulatable parameters
		void SetModPitch(const int32 SourceID, const float InModPitch);
		void SetModVolume(const int32 SourceID, const float InModVolume);
		void SetModLPFFrequency(const int32 SourceID, const float InModFrequency);
		void SetModHPFFrequency(const int32 SourceID, const float InModFrequency);

		void SetModulationRouting(const int32 SourceID, FAtomSoundModulationDefaultSettings& ModulationSettings);

		int64 GetNumFramesPlayed(const int32 SourceId) const;
		float GetEnvelopeValue(const int32 SourceId) const;

		// temp -> connect this function explayer directly!
		void SubmitBuffer(const int32 SourceID, float** Data, int32 NumChannels, int32 NumFrames);

		void ComputeNextBlockOfSamples();
		//void ClearStoppingSounds();
		//void MixOutputBuffers(const int32 SourceId, int32 InNumOutputChannels, const float InSendLevel, EMixerSourceSubmixSendStage InSubmixSendStage, FAlignedFloatBuffer& OutWetBuffer) const;

		// Retrieves a channel map for the given source ID for the given output channels
		// can be used even when a source is 3D if the source is doing any kind of bus sending or otherwise needs a channel map
		void Get2DChannelMap(const int32 SourceId, int32 InNumOutputChannels, Atom::FAlignedFloatBuffer& OutChannelMap);

		void SetBusSendInfo(const int32 SourceID, EAtomBusSendStage InAudioBusSendSTage, uint32 AudiobusID, float BusSendLevel, const FString& InBusName);

		// Quantized event methods
		void PauseSoundForQuantizationCommand(const int32 SourceID);
		void SetSubBufferDelayForSound(const int32 SourceID, const int32 FramesToDelay);
		void UnPauseSoundForQuantizationCommand(const int32 SourceID);

		// Buffer getters
#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		const Atom::FAlignedFloatBuffer& GetPreDistanceAttenuationBuffer(const int32 SourceID) const;
		const Atom::FAlignedFloatBuffer& GetPreEffectBuffer(const int32 SourceID) const;
#endif
		const Atom::FAlignedFloatBuffer& GetPreviousSourceBusBuffer(const int32 SourceID) const;
		const Atom::FAlignedFloatBuffer& GetPreviousAudioBusBuffer(const int32 AudioBusID) const;
		int32 GetNumChannels(const int32 SourceID) const;
		int32 GetNumOutputFrames() const { return NumOutputFrames; }
		FAtomRuntime* GetAtomRuntime() const { return AtomRuntime; }
		bool IsSourceBus(const int32 SourceID) const;
		void PumpCommandQueue();
		void UpdatePendingReleaseData(bool bForceWait = false);
		void FlushCommandQueue(bool bPumpCommandQueue = false);

		// Pushes a TFUnction command into an MPSC queue from an arbitrary thread to the audio render thread
		void AtomMixerThreadMPSCCommand(TFunction<void()>&& InCommand, const char* InDebugString = nullptr);

		// grab info
		void IterateOverVoicePools(TFunctionRef<void(EMixerSourceVoiceCodecType CodecType, EMixerSourceVoiceStreamingType StreamingType, int32 ID, int32 NumUsedVoices, int32 MaxVoices, int32 MaxChannels, int32 MaxSampleRate)> Func) const;

		void AddPendingAudioBusConnection(FAudioBusKey AudioBusKey, int32 NumChannels, bool bIsAutomatic, Audio::FPatchInput PatchInput)
		{
			PendingAudioBusConnections.Enqueue(FPendingAudioBusConnection{ FPendingAudioBusConnection::FPatchVariant(TInPlaceType<Audio::FPatchInput>(), MoveTemp(PatchInput)), MoveTemp(AudioBusKey), NumChannels, bIsAutomatic });
		}

		void AddPendingAudioBusConnection(FAudioBusKey AudioBusKey, int32 NumChannels, bool bIsAutomatic, Audio::FPatchOutputStrongPtr PatchOutputStrongPtr)
		{
			PendingAudioBusConnections.Enqueue(FPendingAudioBusConnection{ FPendingAudioBusConnection::FPatchVariant(TInPlaceType<Audio::FPatchOutputStrongPtr>(), MoveTemp(PatchOutputStrongPtr)), MoveTemp(AudioBusKey), NumChannels, bIsAutomatic });
		}

	private:

#define INVALID_ATOM_RENDER_THREAD_ID static_cast<uint32>(-1)
		uint32 AtomRenderThreadID = INVALID_ATOM_RENDER_THREAD_ID;

		void ReleaseSource(const int32 SourceID);

		void ComputeSourceBuffer(const bool bGenerateBuses, int32 SourceID);
		void ComputeOutputBuffer(const bool bGenerateBuses, int32 SourceID);

		void GenerateSourceAudio(const bool bGenerateBuses);
		void ConnectBusPatches();
		void ComputeBuses();
		void UpdateBuses();

		float GetFloatCompareTolerance() const;
		float GetCommandQueueFillPercentage() const;

		struct FAtomMixerThreadCommand
		{
			// ctor
			FAtomMixerThreadCommand() = default;
			FAtomMixerThreadCommand(TFunction<void()>&& InFunction, const char* InDebugString, bool bInDeferExecution = false);

			// function-call operator
			void operator()() const;

			// data
			TFunction<void()> Function;

			// Defers the execution by a single call to PumpCommandQueue()
			// (used for commands that affect a playing source,
			// and that source gets initialized after the command executes
			bool bDeferExecution = false;

#if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG			
			const char* DebugString = nullptr;					// Statically defined string from macro ATOM_MIXER_THREAD_COMMAND_STRING
			mutable uint64_t StartExecuteTimeInCycles = 0;		// Set just before Function is called, for diagnostics.
#endif // #if WITH_ATOM_MIXER_THREAD_COMMAND_DEBUG

			FString GetSafeDebugString() const;
			float GetExecuteTimeInSeconds() const;
		};

		void AtomMixerThreadCommand(TFunction<void()>&& InFunction, const char* InDebugString = nullptr, bool bInDeferExecution = false);

#if ATOM_PROFILERTRACE_ENABLED
		void OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination);

		void RegisterAudioBusEnvelopeFollowerCVars();
		void UnregisterAudioBusEnvelopeFollowerCVars();

		void OnStartAudioBusEnvelopeFollowerCVarChanged(IConsoleVariable* InStartAudioBusEnvelopeFollowerCVar);
		void OnStopAudioBusEnvelopeFollowerCVarChanged(IConsoleVariable* InStopAudioBusEnvelopeFollowerCVar);

		void StartAudioBusEnvelopeFollowerFromCVar(const uint32 InAudioBusID);
		void StopAudioBusEnvelopeFollowerFromCVar(const uint32 InAudioBusID);

		IConsoleVariable* StartAudioBusEnvelopeFollowerCVar = nullptr;
		IConsoleVariable* StopAudioBusEnvelopeFollowerCVar = nullptr;
#endif // ATOM_PROFILERTRACE_ENABLED

		static const int32 NUM_BYTES_PER_SAMPLE = 2;

		FAtomRuntime* AtomRuntime;

		// Cached ptr to an optional source data override plugin
		TAtomSourceDataOverridePtr SourceDataOverridePlugin;

		// Array of pointers to game thread audio source objects
		TArray<FMixerSourceVoice*> MixerSources;

		// A command queue to execute commands from audio thread (or game thread) to audio mixer device thread.
		struct FCommands
		{
			FThreadSafeCounter NumTimesOvergrown = 0;
			TArray<FAtomMixerThreadCommand> SourceCommandQueue;
		};

		FCommands CommandBuffers[2];
		FThreadSafeCounter RenderThreadCommandBufferIndex;

		FEvent* CommandsProcessedEvent;
		FCriticalSection CommandBufferIndexCriticalSection;

		using FAtomMixerMpscCommand = FAtomMixerThreadCommand;
		TMpscQueue<FAtomMixerMpscCommand> MpscCommandQueue;

		struct FSourceInfo
		{
			FSourceInfo() {}
			~FSourceInfo() {}

			// Used mixer player and playback id which handles source buffer decoding and playback in Atom.
			FMixerSourcePlayer MixerSourcePlayer;
			ISourceListener* SourceListener;

			// Data used for rendering sources (Interleaved)
			TSharedPtr<FMixerSourceVoiceBuffer, ESPMode::ThreadSafe> CurrentPCMBuffer;
			int32 CurrentAudioChunkNumFrames;

			// Data view from Atom (Non-interleaved)
			TArrayView<float> CurrentAudioChunk;

#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
			// Circular buffer used to get audio from ADX source filter (ASR thread) to Atom render thread (Server thread)
			Audio::TCircularAudioBuffer<float> SendsBuffer;
			FCriticalSection SendsBufferCS;

			// Send buffer/links to the audio bus (Server thread)
			// fixed pre-effect buffer send bus for audio bus
			FAlignedFloatBuffer PreEffectBuffer;
#endif

			// Data used for delaying the rendering of source audio for sample-accurate quantization
			int32 SubCallbackDelayLengthInFrames{ 0 };
			Audio::TCircularAudioBuffer<float> SourceBufferDelayLine;

			// Num frames processed
			int64 NumFramesPlayed;

			// What audio bus Id this source is sonfiying, if it is a source bus. This is INDEX_NONE for sources which are not source buses.
			uint32 AudioBusID;

			// Number of samples to count for source bus
			int64 SourceBusDurationFrames;

			// Patch to get sound from audioBus since current design with SonicSync may use various device threads for sources.
			Audio::FPatchOutputStrongPtr SourceBusPatch;

			// What buses this source is sending its audio to. Used to remove this source from the bus send list.
			TArray<uint32> AudioBusSends[(int32)EBusSendType::Count];

			// A DSP object which tracks the amplitude envelope of a source.
			Audio::FInlineEnvelopeFollower SourceEnvelopeFollower;
			float SourceEnvelopeValue;

			// Time stretching
			float TimeStretch;

			// Modulation destinations
			FModulationDestination VolumeModulation;
			FModulationDestination PitchModulation;
			FModulationDestination LowpassModulation;
			FModulationDestination HighpassModulation;

			// Modulation Base (i.e. Carrier) Values
			float VolumeModulationBase;
			float PitchModulationBase;
			float LowpassModulationBase;
			float HighpassModulationBase;

			//Asiacs
			struct FSourceAisacModulation
			{
				FModulationDestination Modulation;
				float ModulationBase = 1.0f;
			};
			TMap<FAtomAisacControl, FSourceAisacModulation> AisacControlModulations;

			// Quantization data
			FQuartzQuantizedCommandHandle QuantizedCommandHandle;

			uint8 bIs3D : 1;
			uint8 bIsCenterChannelOnly : 1;
			uint8 bIsActive : 1;
			uint8 bIsPlaying : 1;
			uint8 bIsPaused : 1;
			uint8 bIsPausedForQuantization : 1;
			//uint8 bDelayLineSet : 1;
			uint8 bIsStopping : 1;
			uint8 bHasStarted : 1;
			//uint8 bIsBusy : 1;
			uint8 bUseHRTFSpatializer : 1;
			uint8 bIsExternalSend : 1;
			//uint8 bUseOcclusionPlugin : 1;
			//uint8 bUseReverbPlugin : 1;
			uint8 bIsDone : 1;
			//uint8 bIsLastBuffer : 1;
			uint8 bEnableBusSends : 1;
			//uint8 bEnableBaseSubmix : 1;
			//uint8 bEnableSubmixSends : 1;
			uint8 bIsVorbis : 1;
			uint8 bIsSoundfield : 1;
			//uint8 bHasPreDistanceAttenuationSend : 1;
			uint8 bModFiltersUpdated : 1;
			//uint8 bShouldSourceBufferListenerZeroBuffer : 1;

			// Source format info
			int32 NumInputChannels;
			int32 NumInputFrames;

			uint32 PlayOrder;
			uint32 ActiveSoundPlayOrder;

			// ID for associated Atom Component if there is one, 0 otherwise
			uint64 AtomComponentID;

			FORCEINLINE void ResetModulators(const FAtomRuntimeId InRuntimeID)
			{
				VolumeModulation.Init(InRuntimeID, FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
				PitchModulation.Init(InRuntimeID, FName("Pitch"));
				HighpassModulation.Init(InRuntimeID, FName("HPFCutoffFrequency"));
				LowpassModulation.Init(InRuntimeID, FName("LPFCutoffFrequency"));

				VolumeModulationBase = 0.0f;
				PitchModulationBase = 0.0f;
				HighpassModulationBase = MIN_FILTER_FREQUENCY;
				LowpassModulationBase = MAX_FILTER_FREQUENCY;

				AisacControlModulations.Empty();
			}

#if ATOM_MIXER_ENABLE_DEBUG_MODE
			uint8 bIsDebugMode : 1;
			FString DebugName;
#endif // ATOM_MIXER_ENABLE_DEBUG_MODE
		};

		// Hang/crash diagnostics.
		void DoStallDiagnostics();

		void LogRenderThreadStall();
		void LogInflightAsyncTasks();
		void LogCallstacks();
		void LogCallstack(uint32 InThreadID);

		TSharedRef<FMixerAudioBus> FindOrAddAudioBus(const FAudioBusKey BusKey, const int32 InNumChannels, const bool bIsAutomatic, bool* bOutAlreadyExisted = nullptr);

		// Array of listener transforms
		TArray<FTransform> ListenerTransforms;

		// Array of source infos.
		TArray<FSourceInfo> SourceInfos;

		// Map of players Id's to AtomExPlayers. (extra player ... remove?)
		TMap<FAtomExPlayerKey, TSharedPtr<FAtomExPlayer>> AudioPlayers;

		// Map of bus object Id's to audio bus data. 
		TMap<FAudioBusKey, TSharedPtr<FMixerAudioBus>> AudioBuses;
		TArray<FAudioBusKey> AudioBusKeys_AudioThread;

		// Array of task data waiting to finished. Processed on audio render thread.
		TArray<FMixerSourcePlayer> PendingSourcePlayers;

		// General information about sources in source manager accessible from game thread.
		struct FGameThreadInfo
		{
			TArray<int32> FreeSourceIndices;
			TArray<bool> bIsBusy;
			TArray<bool> bNeedsSpeakerMap;
			TArray<bool> bIsDebugMode;
			TArray<bool> bIsUsingHRTFSpatializer;
		} GameThreadInfo;

		int32 NumActiveSources;
		int32 NumTotalSources;
		int32 NumOutputFrames;
		int32 NumOutputSamples;

		// Commands queued up to execute
		FThreadSafeCounter NumCommands;

		uint8 bInitialized : 1;
		uint8 bUsingSpatializationPlugin : 1;
		uint8 bUsingSourceDataOverridePlugin : 1;

		// Set to true when the audio source manager should pump the command queue
		FThreadSafeBool bPumpQueue;
		std::atomic<uint64> LastPumpCompleteTimeInCycles = 0;
		std::atomic<EAtomSourceManagerRenderThreadPhase> RenderThreadPhase = EAtomSourceManagerRenderThreadPhase::Begin;
		FRWLock CurrentlyExecutingCmdLock;						// R/W slim lock for the currently executing cmd, so we can safely query it.
		FAtomMixerThreadCommand CurrentlyExecuteingCmd;		// K

		struct FPendingAudioBusConnection
		{
			using FPatchVariant = TVariant<Audio::FPatchInput, Audio::FPatchOutputStrongPtr>;
			FPatchVariant PatchVariant;
			FAudioBusKey AudioBusKey;
			int32 NumChannels = 0;
			bool bIsAutomatic = false;
		};

		TMpscQueue<FPendingAudioBusConnection> PendingAudioBusConnections;

		friend class FMixerSourceVoice;
	};
} // namespace
