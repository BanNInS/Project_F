
#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "DSP/EnvelopeFollower.h"
#include "DSP/MultithreadedPatching.h"
#include "DSP/SpectrumAnalyzer.h"
#include "DSP/ReverbFast.h"

#include "Atom/Atom.h"
#include "Atom/AtomBus.h" // bussend
#include "Atom/AtomRack.h" // bussend
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Modulation/AtomModulationDestination.h"

// The time it takes to process the submix graph. Process submix effects, mix into the submix buffer, etc.
DECLARE_CYCLE_STAT_EXTERN(TEXT("Submix Graph"), STAT_AtomMixerSubmixes, STATGROUP_AtomMixer, CRIWARECORE_API);

// Forward Declarations
class FAtomRuntime;
class UAtomBus;
class UAtomRackBase;
class UAtomModulatorBase;
struct FAtomSoundSpectrumAnalyzerSettings;
struct FAtomSoundSpectrumAnalyzerDelegateSettings;

namespace Atom
{
	class FMixerSourceVoice;

	enum EMixerSourceSubmixSendStage
	{
		// Whether to do the send pre distance attenuation
		PostDistanceAttenuation,

		// Whether to do the send post distance attenuation
		//PreDistanceAttenuation,

		//
		PostFader
	};

	/**
	 * A FMixerSubmix represents an Atom DSP bus.
	 * this serves as proxy UAtomBus to atom runtime and atom DSP bus. 
	 * It will manage bus settings and PCM data that goes in and out.
	 */
	class FMixerSubmix
	{
	public:
	
		CRIWARECORE_API FMixerSubmix(FAtomRuntime* InAtomRuntime);
		CRIWARECORE_API virtual ~FMixerSubmix();

		// Initialize the submix object with the UAtomBus ptr.
		CRIWARECORE_API void Init(const UAtomBus* InAtomBus, bool bAllowReInit = false);

		// Returns the mixer submix ID
		uint32 GetID() const { return ID; }

		// Return the owners name 
		CRIWARECORE_API const FString& GetName() const { return SubmixName; }

		// Registers the given audiobus to this submix
		CRIWARECORE_API void RegisterAudioBus(const Atom::FAudioBusKey& InAudioBusKey, Audio::FPatchInput&& InPatchInput);

		// Unregisters a registered audiobus from this submix (if any)
		CRIWARECORE_API void UnregisterAudioBus(const Atom::FAudioBusKey& InAudioBusKey);

		// Sets the output level of the submix in linear gain
		CRIWARECORE_API void SetOutputVolume(float InOutputLevel);

		// Sets the static output volume of the submix in linear gain
		CRIWARECORE_API void SetDryLevel(float InDryLevel);

		// Sets the wet level of the submix in linear gain
		CRIWARECORE_API void SetWetLevel(float InWetLevel);

		// Update modulation settings of the submix
		CRIWARECORE_API void UpdateModulationSettings(const TSet<TObjectPtr<UAtomModulatorBase>>& InOutputModulators, const TSet<TObjectPtr<UAtomModulatorBase>>& InWetLevelModulators, const TSet<TObjectPtr<UAtomModulatorBase>>& InDryLevelModulators);

		// Update modulation settings of the submix with Decibel values
		CRIWARECORE_API void SetModulationBaseLevels(float InVolumeModBaseDb, float InWetModeBaseDb, float InDryModBaseDb);

		FModulationDestination* GetOutputVolumeDestination();

		FModulationDestination* GetWetVolumeDestination();

		CRIWARECORE_API Audio::FPatchOutputStrongPtr AddPatch(float InGain);

		/** Whether or not this submix instance is muted. */
		CRIWARECORE_API void SetBackgroundMuted(bool bInMuted);

		/** Checks to see if submix is valid.  Submix can be considered invalid if the OwningSubmix
		  * pointer is stale.
		  */
		CRIWARECORE_API bool IsValid() const;

		// Function which processes command for Atom SDK buses and racks.
		CRIWARECORE_API void ProcessAudio();

		// Function which processes updates from geme thread to ASR threads.
		CRIWARECORE_API void Update();

		// Returns the device sample rate this submix is rendering to
		CRIWARECORE_API int32 GetSampleRate() const;

		// Returns the output channels this submix is rendering to
		CRIWARECORE_API int32 GetNumOutputChannels() const;

		// Returns the number of effects in this submix's effect chain
		//CRIWARECORE_API int32 GetNumChainEffects();

		// This is called by the corresponding USoundSubmix when StartRecordingOutput is called.
		//CRIWARECORE_API void OnStartRecordingOutput(float ExpectedDuration);

		// This is called by the corresponding USoundSubmix when StopRecordingOutput is called.
		//CRIWARECORE_API FAlignedFloatBuffer& OnStopRecordingOutput(float& OutNumChannels, float& OutSampleRate);

		// This is called by the corresponding USoundSubmix when PauseRecording is called.
		//CRIWARECORE_API void PauseRecordingOutput();

		// This is called by the corresponding USoundSubmix when ResumeRecording is called.
		//CRIWARECORE_API void ResumeRecordingOutput();

		// Starts envelope following with the given attack time and release time
		CRIWARECORE_API void StartEnvelopeFollowing(int32 AttackTime, int32 ReleaseTime);

		// Stops envelope following the submix
		CRIWARECORE_API void StopEnvelopeFollowing();

		// Adds an envelope follower delegate
		CRIWARECORE_API void AddEnvelopeFollowerDelegate(const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP);

		// Removes an existing envelope follower delegate
		CRIWARECORE_API void RemoveEnvelopeFollowerDelegate(const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP);

		// Initializes a new FFT analyzer for this submix and immediately begins feeding audio to it.
		CRIWARECORE_API void StartSpectrumAnalysis(const FAtomSoundSpectrumAnalyzerSettings& InSettings);

		// Terminates whatever FFT Analyzer is being used for this submix.
		CRIWARECORE_API void StopSpectrumAnalysis();

		// Adds an spectral analysis delegate
		CRIWARECORE_API void AddSpectralAnalysisDelegate(const FAtomSoundSpectrumAnalyzerDelegateSettings& InDelegateSettings, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP);

		// Removes an existing spectral analysis delegate
		CRIWARECORE_API void RemoveSpectralAnalysisDelegate(const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP);

		// Gets the most recent magnitude values for each corresponding value in InFrequencies (in Hz).
		// This requires StartSpectrumAnalysis to be called first.
		CRIWARECORE_API void GetMagnitudeForFrequencies(const TArray<float>& InFrequencies, TArray<float>& OutMagnitudes);

		// Gets the most recent phase values for each corresponding value in InFrequencies (in Hz).
		// This requires StartSpectrumAnalysis to be called first.
		CRIWARECORE_API void GetPhaseForFrequencies(const TArray<float>& InFrequencies, TArray<float>& OutPhases);

		// Atom DSP Level Meter for Rack
		CRIWARECORE_API void StartLevelMeterMeasuring(const FAtomLevelMeterSettings& InSettings);
		CRIWARECORE_API void StopLevelMeterMeasuring();
		CRIWARECORE_API void AddLevelMeterDelegate(const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP);
		CRIWARECORE_API void RemoveLevelMeterDelegate(const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP);

		// Atom DSP Loudness Meter for Rack
		CRIWARECORE_API void StartLoudnessMeterMeasuring(const FAtomLoudnessMeterSettings& InSettings);
		CRIWARECORE_API void StopLoudnessMeterMeasuring();
		CRIWARECORE_API void ResetLoudnessMeterMeasuring();
		CRIWARECORE_API void AddLoudnessMeterDelegate(const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP);
		CRIWARECORE_API void RemoveLoudnessMeterDelegate(const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP);

		// Atom DSP True Peak Meter for Rack
		CRIWARECORE_API void StartTruePeakMeterMeasuring(const FAtomTruePeakMeterSettings& InSettings);
		CRIWARECORE_API void StopTruePeakMeterMeasuring();
		CRIWARECORE_API void AddTruePeakMeterDelegate(const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP);
		CRIWARECORE_API void RemoveTruePeakMeterDelegate(const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP);

		// Atom DSP Rendered Samples Counter Delegate for Rack
		CRIWARECORE_API void AddRenderedSamplesDelegate(const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSampleBP);
		CRIWARECORE_API void RemoveRenderedSamplesDelegate(const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSampleBP);

		// Atom DSP Performance Monitor Delegate for Rack
		CRIWARECORE_API void AddPerformanceMonitorDelegate(const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP);
		CRIWARECORE_API void RemovePerformanceMonitorDelegate(const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP);
		CRIWARECORE_API void ResetPerformanceMonitor();

		// Broadcast the envelope and submix delegates on the game thread
		CRIWARECORE_API void BroadcastDelegates();

		// returns true if this submix is encoded to a soundfield.
		CRIWARECORE_API bool IsSoundfieldSubmix() const;

		// returns true if this submix sends it's audio to the default endpoint.
		CRIWARECORE_API bool IsDefaultEndpointSubmix() const;

		//Returns true if this is an endpoint type that should no-op for this platform
		CRIWARECORE_API bool IsDummyEndpointSubmix() const;

	protected:

		// Initialize the submix internal
		CRIWARECORE_API void InitInternal();

		// Function which processes Audio on ASR thread before applying DSP effect chain defined by the DspBusSetting of the bus owner rack.
		void ProcessAudioPreEffects(FAlignedFloatBuffer& InAudioBuffer, int InNumChannels);

		// Function which processes Audio on ASR thread after had applyed DSP effect chain defined by the DspBusSetting of the bus owner rack.
		void ProcessAudioPostEffects(FAlignedFloatBuffer& InAudioBuffer, int InNumChannels);
		
		// Down mix the given buffer to the desired down mix channel count
		static CRIWARECORE_API void DownmixBuffer(const int32 InChannels, const FAlignedFloatBuffer& InBuffer, const int32 OutChannels, FAlignedFloatBuffer& OutNewBuffer);

		CRIWARECORE_API void MixBufferDownToMono(const FAlignedFloatBuffer& InBuffer, int32 NumInputChannels, FAlignedFloatBuffer& OutBuffer);
	
		// Pump command queue
		CRIWARECORE_API void PumpCommandQueue();

		// Add command to the command queue
		CRIWARECORE_API void SubmixCommand(TFunction<void()> Command);

		// The name of this submix (the owning USoundSubmix) (at top so we can see in debugger it's name)
		FString SubmixName;

		// This mixer submix's ID
		uint32 ID;

		// This mixer submix parent rack's ID
		uint32 RackID;

		// The filer bus index
		uint32 FilterBusIndex;

		// Owning mixer runtime. 
		FAtomRuntime* AtomRuntime;

		// Atom bus/rack can have various sampling rate and channels
		int32 NumChannels;
		int32 SampleRate;

		// Map of mixer source voices with a given send level for this submix
		//TMap<FMixerSourceVoice*, FSubmixVoiceData> MixerSourceVoices;

		float CurrentOutputVolume;
		float TargetOutputVolume;
		float CurrentWetLevel;
		float TargetWetLevel;
		float CurrentDryLevel;
		float TargetDryLevel;

		FModulationDestination VolumeMod;
		FModulationDestination DryLevelMod;
		FModulationDestination WetLevelMod;

		float VolumeModBaseDb = 0.f;
		float DryModBaseDb = ATOM_MIN_VOLUME_DECIBELS;
		float WetModBaseDb = 0.f;

		// modifiers set from BP code
		float VolumeModifier = 1.f;
		float DryLevelModifier = 1.f;
		float WetLevelModifier = 1.f;

		// Envelope following data
		float EnvelopeValues[ATOM_MAX_DSP_CHANNELS];
		Audio::FEnvelopeFollower EnvelopeFollower;
		int32 EnvelopeNumChannels;
		FCriticalSection EnvelopeCriticalSection;

		// Spectrum analyzer. Created and destroyed on the audio thread.
		FCriticalSection SpectrumAnalyzerCriticalSection;
		FAtomSoundSpectrumAnalyzerSettings SpectrumAnalyzerSettings;
		TSharedPtr<Audio::FAsyncSpectrumAnalyzer, ESPMode::ThreadSafe> SpectrumAnalyzer;

		// Level meter. Attached and removed on the atom thread.
		FAtomLevelMeterSettings LevelMeterSettings;

		// Loudness meter. Attached and removed on the atom thread.
		FAtomLoudnessMeterSettings LoudnessMeterSettings;

		// True peak meter. Attached and removed on the atom thread.
		FAtomTruePeakMeterSettings TruePeakMeterSettings;

		// This buffer is used as interleaved exchange to use DSPs from Unreal.
		FAlignedFloatBuffer InterleavedBuffer;

		// This buffer is used to downmix the submix output to mono before submitting it to the SpectrumAnalyzer.
		FAlignedFloatBuffer MonoMixBuffer;

		// The dry channel buffer
		FAlignedFloatBuffer DryChannelBuffer;

		// This bus is used to mixdown to connected audio buses.
		FAlignedFloatBuffer DownmixedBuffer;

		// Submix command queue to shuffle commands from audio thread to audio render thread.
		TQueue<TFunction<void()>> CommandQueue;

		// Whether or not this submix is the main DSP bus of a rack.
		uint8 bIsMainDspBus : 1;

		// Whether or not this submix is muted.
		uint8 bIsBackgroundMuted : 1;

		// Whether or not auto-disablement is enabled. If true, the submix will disable itself.
		//uint8 bAutoDisable : 1;

		// Whether or not the submix is currently rendering audio. I.e. audio was sent to it and mixing it, or any of its child submixes are rendering audio.
		//uint8 bIsSilent : 1;

		// Whether or not we're currently disabled (i.e. the submix has been silent)
		//uint8 bIsCurrentlyDisabled : 1;

		// The time to wait to disable the submix if the auto-disablement is active.
		//double AutoDisableTime;

		// Bool set to true when envelope following is enabled
		std::atomic<bool> bIsEnvelopeFollowing;

		// Multi-cast delegate to broadcast envelope data from this submix instance
		FOnAtomBusEnvelope OnAtomBusEnvelope;

		struct FSpectralAnalysisBandInfo
		{
			Audio::FInlineEnvelopeFollower EnvelopeFollower;
		};

		struct FSpectrumAnalysisDelegateInfo
		{
			FAtomSoundSpectrumAnalyzerDelegateSettings DelegateSettings;

			FOnAtomBusSpectralAnalysis OnAtomBusSpectralAnalysis;

			TUniquePtr<Audio::ISpectrumBandExtractor> SpectrumBandExtractor;
			TArray<FSpectralAnalysisBandInfo> SpectralBands;

			float LastUpdateTime = -1.0f;
			float UpdateDelta = 0.0f;

			FSpectrumAnalysisDelegateInfo()
			{
			}

			FSpectrumAnalysisDelegateInfo(FSpectrumAnalysisDelegateInfo&& Other)
			{
				OnAtomBusSpectralAnalysis = Other.OnAtomBusSpectralAnalysis;
				SpectrumBandExtractor.Reset(Other.SpectrumBandExtractor.Release());
				DelegateSettings = Other.DelegateSettings;
				SpectralBands = Other.SpectralBands;
				UpdateDelta = Other.UpdateDelta;
			}

			~FSpectrumAnalysisDelegateInfo()
			{
			}
		};

		TArray<FSpectrumAnalysisDelegateInfo> SpectralAnalysisDelegates;

		// Bool set to true when spectrum analysis is enabled
		std::atomic<bool> bIsSpectrumAnalyzing;

		// Array to hold last level meter results (game thread)
		TArray<FAtomLevelMeterMeasure> LevelMeterMeasures;

		// Bool set to true when level meter is enabled
		std::atomic<bool> bIsLevelMeterMeasuring;

		// Multi-cast delegate to broadcast measure data from this submix instance
		FOnAtomRackLevelMeterMeasure OnAtomRackLevelMeterMeasure;

		// Hold last loudness meter results (game thread)
		FAtomLoudnessMeterMeasure LoudnessMeterMeasure;

		// Bool set to true when loudness meter is enabled
		std::atomic<bool> bIsLoudnessMeterMeasuring;

		// Multi-cast delegate to broadcast measure data from this submix instance
		FOnAtomRackLoudnessMeterMeasure OnAtomRackLoudnessMeterMeasure;

		// Array to hold last true peak meter results (game thread)
		TArray<FAtomTruePeakMeterMeasure> TruePeakMeterMeasures;

		// Bool set to true when tru peak meter is enabled
		std::atomic<bool> bIsTruePeakMeterMeasuring;

		// Multi-cast delegate to broadcast measure data from this submix instance
		FOnAtomRackTruePeakMeterMeasure OnAtomRackTruePeakMeterMeasure;

		// Multi-cast delegate to broadcast rendered samples data from this submix instance
		FOnAtomRackRenderedSamples OnAtomRackRenderedSamples;

		// Multi-cast delegate to broadcast performance monitor data from this submix instance
		FOnAtomRackPerformanceMonitorResult OnAtomRackPerformanceMonitorResult;

		// Hold last loudness meter results (game thread)
		FAtomPerformanceMonitorResult PerformanceMonitorResult;

		// Handle back to the owning UAtomBus. Used when the device is shutdown to prematurely end a recording.
		TWeakObjectPtr<const UAtomBus> OwningAtomBusObject;

		Audio::FPatchSplitter PatchSplitter;

		friend class FAtomRuntime;

	private:

		CRIWARECORE_API void SendAudioToRegisteredAudioBuses(FAlignedFloatBuffer& OutAudioBuffer);

		// Registered audio buses
		TMap<Atom::FAudioBusKey, TPair<Audio::FPatchInput, int32>> AudioBuses;

	public:

		void HandleNativeOnPreEffectFilter(CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[]);
		void HandleNativeOnPostEffectFilter(CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[]);

		// TEST DSP
		//TSharedPtr<Audio::FPlateReverbFast, ESPMode::ThreadSafe> Reverb;
	};
} // namespace
