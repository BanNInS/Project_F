
#pragma once

#include "AtomMixerSourceManager.h"
#include "Atom/AtomSourceVoiceEffect.h"

// Forward Definitions
class FAtomRuntime;

namespace Atom
{
	// Forward Definitions
	struct FMixerSourceVoiceBuffer;
	//struct FMixerSourceVoiceFilterParams;
	struct FMixerSourceVoiceInitParams;
	//class FMixerSubmix;
	class FMixerSource;
	class FMixerSourceManager;
	//class ISourceBufferQueueListener;

	// An Atom voice
	class FMixerSourceVoice
	{
	public:

		FMixerSourceVoice();
		~FMixerSourceVoice();

		// Resets the source voice state
		void Reset(FAtomRuntime* InAtomRuntime);

		// Initializes the mixer source voice
		bool Init(const FMixerSourceVoiceInitParams& InFormat);

		// Releases the source voice back to the source buffer pool
		void Release();

		// Sets the source voice pitch value.
		//void SetPitch(const float InPitch);

		// Sets the source voice volume value.
		//void SetVolume(const float InVolume);

		// Sets the source voice distance attenuation.
		//void SetDistanceAttenuation(const float InDistanceAttenuation);

		// Sets the source voice's LPF filter frequency.
		//void SetLPFFrequency(const float InFrequency);

		// Sets the source voice's HPF filter frequency.
		//void SetHPFFrequency(const float InFrequency);

		// Sets the source voice modulation base pitch value.
		void SetModPitch(const float InPitch);

		// Sets the source voice's volume modulation base frequency.
		void SetModVolume(const float InVolume);

		// Sets the source voice's LPF filter modulation base frequency.
		void SetModLPFFrequency(const float InFrequency);

		// Sets the source voice's HPF filter modulation base frequency.
		void SetModHPFFrequency(const float InFrequency);

		void SetModulationRouting(FAtomSoundModulationDefaultSettings& RoutingSettings);

		// Starts the source voice generating audio output into it's submix.
		void Play();

		// Pauses the source voice (i.e. stops generating output but keeps its state as "active and playing". Can be restarted.)
		void Pause();

		// Immediately stops the source voice (no longer playing or active, can't be restarted.)
		void Stop();

		// Does a faded stop (to avoid discontinuity)
		void StopFade(/*int32 NumFrames*/);

		// Queries if the voice is playing
		bool IsPlaying() const;

		// Queries if the voice is paused
		bool IsPaused() const;

		// Queries if the source voice is active.
		bool IsActive() const;

		// Queries if the source has finished its fade out.
		bool IsStopFadedOut() const { return bStopFadedOut; }

		// Returns the source's ID.
		int32 GetSourceID() const { return SourceID; }

		// Returns the Atom voice pool's ID to assign to MixerSource.
		int32 GetVoicePoolID() const { return CodecType != EMixerSourceVoiceCodecType::InputPort ? PoolID : INDEX_NONE; }

		// Returns the Atom input port if voice is used as a port.
		void* GetInputPort() const { return CodecType == EMixerSourceVoiceCodecType::InputPort ? (void*)VoicePool.Get() : nullptr; }

		// Sets the fixed voice effect for this voice pool.
		void SetVoiceEffect(const FAtomSourceVoiceEffect& Params);

		// Called when the source is a bus and needs to mix other sources together to generate output
		//void OnMixBus(FMixerSourceVoiceBuffer* OutMixerSourceBuffer);

		// Whether or not the voice is currently using HRTF spatialization.
		//
		// @param bDefaultValue - This value will be returned if voice does not have a valid source id.
		bool IsUsingHRTFSpatializer(bool bDefaultValue) const;

		// Retrieves the envelope value of the source.
		float GetEnvelopeValue() const;

		void SetEnablement(bool bInEnableSourceBusSendRouting, bool bInEnableMainSubmixOutput, bool bInEnableSubmixSendRouting);

		// Set the source bus send levels
		void SetAudioBusSendInfo(EAtomBusSendStage InBusSendStage, uint32 AudioBusID, float BusSendLevel,const FString& InBusName);

		// temp
		void SubmitBuffer(float** Data, int NumChannels, int NumFrames);

		int64 GetNumFramesPlayed() const;

		FAtomPlaybackId InitMixerPlayer();

	public:

		// TEMP may move this to real time filter callback
		// Process modulation update and retun the final value 
		float GetModVolumeEnd();
		// Process modulation update and retun the final value 
		float GetModPitchEnd();
		// Process modulation update and retun the final value 
		float GetModLPFEnd();
		// Process modulation update and retun the final value 
		float GetModHPFEnd();
		// Process modulation update and retun the final value 
		bool IterateOverAisacModulations(TFunction<bool(FAtomAisacControl& Control, FModulationDestination& Modulation, float ModulationBase)> Func);

	private:

		friend class FMixerSourceManager;

		FMixerSourceManager* SourceManager;
		FAtomRuntime* AtomRuntime;

		FThreadSafeBool bStopFadedOut;
		float PitchModBase;
		float VolumeModBase;
		float LPFFrequencyModBase;
		float HPFFrequencyModBase;
		int32 SourceID;
		uint16 bIsPlaying : 1;
		uint16 bIsPaused : 1;
		uint16 bIsActive : 1;
		uint16 bIsSourceBus : 1;
		uint16 bEnableSourceBusSends : 1;
		uint16 bEnableBaseSubmix : 1;
		uint16 bEnableSubmixSends : 1;

		// Atom use static voice pools so a voice here is a pool with one or more sub-voices.
		FCriAtomVoicePoolPtr VoicePool;

		// the pool settings
		EMixerSourceVoiceCodecType CodecType = EMixerSourceVoiceCodecType::Undefined;
		EMixerSourceVoiceStreamingType StreamingType = EMixerSourceVoiceStreamingType::Mixed;
		int32 MaxInputVoices;
		int32 MaxInputChannels;
		int32 MaxInputSampleRate;

		// The Atom voice pool ID 
		int32 PoolID = INDEX_NONE;

		// Voice Effect and parameters at effect attach time
		FAtomSourceVoiceEffect VoiceEffect;
	};
} // namespace
