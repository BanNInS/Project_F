
#pragma once

#include "CoreMinimal.h"
#include "DSP/MultithreadedPatching.h"

#include "AtomMixer.h"
#include "AtomMixerTrace.h"
#include "Atom/Atom.h"
#include "Atom/AtomUtility.h"
#include "Atom/AtomAudioBusSubsystem.h"

#if ATOM_PROFILERTRACE_ENABLED
#include "DSP/EnvelopeFollower.h"
#endif // ATOM_PROFILERTRACE_ENABLED

namespace Atom
{
	// Forward Definitions
	class FMixerSourceManager;
	class FMixerSubmix;

	// Struct holding mappings of runtime source ids (bus instances) to bus send level
	struct FAudioBusSend
	{
		int32 SourceID = INDEX_NONE;
		uint64 TransmitterID = INDEX_NONE;
		float SendLevel = 0.0f;
#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		Audio::FPatchInput PatchInput;
#endif
	};

	// Bus instance data. Holds source id bus instances and bus sends data
	class FMixerAudioBus
	{
	public:
		// Creates an audio bus.
		// SourceManager	The owning source manager object.
		// bInIsAutomatic	Whether or not this audio bus was created automatically via source buses.
		// InNumChannels	The number of channels of the source bus.
		// InSampleRate		The sample rate of the source bus.
		FMixerAudioBus(FMixerSourceManager* SourceManager, const FAudioBusKey& InBusKey, bool bInIsAutomatic, int32 InNumChannels, int32 InSampleRate = 48000);

		// Allow anybody to add a pre-existing patch output object to the audio bus
		void AddNewPatchOutput(const Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr);

		// Allow anybody to write audio into this audio bus from any thread.
		void AddNewPatchInput(const Audio::FPatchInput& InPatchInput);

		// Allow anybody to write audio into this audio bus from any thread.
		void RemovePatchInput(const Audio::FPatchInput& InPatchInput);

	private:
		
		// Sets whether or not this audio bus is automatic.
		void SetAutomatic(bool bInIsAutomatic) { bIsAutomatic = bInIsAutomatic; }

		// Returns if this is a manual audio bus vs automatic.
		bool IsAutomatic() const { return bIsAutomatic; }

		// Returns the sample rate of the audio bus.
		int32 GetSampleRate() const { return SampleRate; }

		// Returns the number of channels of the audio bus.
		int32 GetNumChannels() const { return NumChannels; }

		// Update the mixer bus after a render block.
		void Update();

		// Adds a source id for instances of this bus.
		void AddInstanceID(const int32 InSourceInstanceID, const uint64 InTransmitterID, int32 InNumOutputChannels);

		// Removes the source id from this bus. Returns true if there are no more instances or sends.
		bool RemoveInstanceID(const int32 InSourceID, const uint64 InTransmitterID);

#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		// Pushes the audio from the source to the bus send patch input.
		int32 PushSendAudio(EAtomBusSendStage BusSendStage, const int32 InSourceID, const Atom::FAlignedFloatBuffer& InBuffer);
#endif
		// Adds a bus send to the bus.
		void AddSend(EAtomBusSendStage BusSendStage, const FAudioBusSend& InBusSend);

		// Removes the source instance from this bus's send list.
		bool RemoveSend(EAtomBusSendStage BusSendStage, const int32 InSourceId);

		// Gets the current mixed bus buffer.
		const Atom::FAlignedFloatBuffer& GetCurrentBusBuffer() const;

		// Gets the previous mixed bus buffer.
		const Atom::FAlignedFloatBuffer& GetPreviousBusBuffer() const;

		// Computes the mixed buffer.
		void MixBuffer();

		// If this bus was constructed before.
		void SetNumOutputChannels(int32 InNumOutputChannels);

#if ATOM_PROFILERTRACE_ENABLED
		void StartEnvelopeFollower(const float InAttackTime, const float InReleaseTime);
		void StopEnvelopeFollower();
		void ProcessEnvelopeFollower(const float* InBuffer);
#endif // ATOM_PROFILERTRACE_ENABLED

		// Array of instance ids. These are sources which are instances of this.
		// It's possible for this data to have bus sends but no instance ids.
		// This means a source would send its audio to the bus if the bus had an instance.
		// Once and instance plays, it will then start sending its audio to the bus instances.
		TArray<int32> InstanceIDs;

		// Bus sends to this instance
		TArray<FAudioBusSend> AudioBusSends[(int32)EAtomBusSendStage::Count];

//#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		// This is the ratio of mixer-to-splitter buffer size, 2.0 means its twice as big as the consumer buffer.
		float IntermediateBufferRatio = 2.0f;

		// Ratio of initial buffer to fill with silence ahead of consumption.
		// Adjusting this when sending audio over different devices can resolve starvation at the cost of added latency.
		float InitialSilenceFillRatio = 0.0f;

		// The main mixed data of the bus, this is a circular data 
		Atom::FCircularSampleBuffer MixedSourceData;
//#endif
#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		// Audio buffer to downmix source inputs
		FAlignedFloatBuffer DownmixedBuffer;
#endif 
		// Intermediate buffer to push data to MixSplitter.
		FAlignedFloatBuffer PushBuffer;

		// The mixed source data. This is double-buffered to allow buses to send audio to themselves.
		// Buses feed audio to each other by storing their previous buffer. Current buses mix in previous other buses (including themselves)
		FAlignedFloatBuffer IntermediateBuffers[2];

		// The index of the bus data currently being rendered
		int32 CurrentBufferIndex;

		// The sample rate of this bus
		int32 SampleRate;

		// The number of channels of this bus
		int32 NumChannels;

		// The number of output frames
		int32 NumFrames;

		// Owning soruce manager
		FMixerSourceManager* SourceManager;

		// Multiple places can produce and consume from audio buses
		Audio::FPatchMixer PatchMixer;
		Audio::FPatchSplitter PatchSplitter;

		// Unique bus identifier
		FAudioBusKey BusKey;

		// Was created manually, not via source buses.
		bool bIsAutomatic;

#if ENABLE_ATOM_DEBUG
		FString BusName;
#endif

#if ATOM_PROFILERTRACE_ENABLED
		Audio::FEnvelopeFollower EnvelopeFollower;
		float EnvelopeValues[ATOM_MIXER_MAX_OUTPUT_CHANNELS];
		int32 EnvelopeNumChannels = 0;
		bool bIsEnvelopeFollowing = false;
#endif // ATOM_PROFILERTRACE_ENABLED

		friend FMixerSourceManager;
		friend FMixerSubmix;
	};
} // namespace
