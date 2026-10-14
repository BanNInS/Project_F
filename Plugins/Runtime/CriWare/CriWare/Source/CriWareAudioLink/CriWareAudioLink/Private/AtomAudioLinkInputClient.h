
#pragma once

#include "IAudioLink.h"
#include "IAudioLinkFactory.h"
#include "Logging/LogMacros.h"

#include "Atom/Atom.h"
#include "Atom/AtomSoundGenerator.h"

#include "AtomAudioLinkPrivate.h"

struct FAtomWaveInfo;
class UAtomAudioLinkComponent;
class FAtomAudioLinkSettingsProxy;
using FAtomAudioLinkSettingProxyPtr = TSharedPtr<FAtomAudioLinkSettingsProxy, ESPMode::ThreadSafe>;

/**
 * AudioLink output buffer interface to Atom source mixer input. 
 */
class FAtomAudioLinkInputClient : public TSharedFromThis<FAtomAudioLinkInputClient, ESPMode::ThreadSafe>
{
public:

	FAtomAudioLinkInputClient(const FSharedBufferedOutputPtr& InProducerSP, const FAtomAudioLinkSettingProxyPtr& InSettingsSP, FName Name);
	~FAtomAudioLinkInputClient();

	// create a new PCM voice.
	void Start(UAtomAudioLinkComponent* InComponent);
	void Start();

	// destroy the PCM voice.
	void Stop(int32 SourceID = INDEX_NONE);

	struct FWorldState
	{
		FTransform WorldTransform;
	};

	// Called from Consumer thread at game tick rate.
	void UpdateWorldState(const FWorldState&);

private:

	// attach to mixer and rawpcm voice
	void Register(const FName& InName);

	// detatch from mixer rawpcm voice
	void Unregister();

	// Called from Atom render thread.
	bool GetPCMSamples(uint32 InNumChannels, uint32 InNumFrames, float** InChannelBuffers);

	// Called from Atom render thread.
	void GetPCMFormat(EAtomPcmBitDepth& InFormat);

	// Called from game thread when using UAtomAudioLinkSound.
	void GetWaveInfo(FAtomWaveInfo& Info);

	//* Attempt to set our state to started. Not this can fail if we're already started. */	
	bool TrySetStartedFlag();

	//* Attempt to set started to false. This can fail if we're already stopped. */	
	bool TryUnsetStartedFlag();

	// The AudioLink output buffer.
	Audio::AlignedFloatBuffer InterleavedBuffer;
	FName ProducerName;
	FWeakBufferedOutputPtr WeakProducer;
	IBufferedAudioOutput::FBufferFormat UnrealFormat;

	FAtomAudioLinkSettingProxyPtr SettingsSP;
	std::atomic<bool> bStarted;

	uint32 AtomComponentID = INDEX_NONE;
	TWeakObjectPtr<UAtomAudioLinkComponent> WeakLinkComponent;

	FDelegateHandle RuntimeActivatedHandle;

	friend class UAtomAudioLinkSound;
	friend class UAtomAudioLinkComponent;
	friend class FAtomAudioLinkSoundGenerator;
};

using FAtomAudioLinkInputClientPtr = TSharedPtr<FAtomAudioLinkInputClient, ESPMode::ThreadSafe>;
