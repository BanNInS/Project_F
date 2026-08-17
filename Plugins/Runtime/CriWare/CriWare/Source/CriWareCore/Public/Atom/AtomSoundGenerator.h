
#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Templates/Function.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"

namespace Atom
{
	using AtomTaskQueueId = uint32;
}

// Parameters used for constructing a new IAtomSoundGenerator.
struct FAtomSoundGeneratorInitParams
{
	FAtomRuntimeId AtomRuntimeID;
	uint64 AtomComponentID = 0;
	float SampleRate = 0.0f;
	int32 AtomMixerNumOutputFrames = 0;
	int32 NumChannels = 0;
	int32 NumFramesPerCallback = 0;
	uint64 InstanceID = 0;
	bool bIsPreviewSound = false;
};

class IAtomSoundGenerator
{
public:
	CRIWARECORE_API IAtomSoundGenerator();
	CRIWARECORE_API virtual ~IAtomSoundGenerator();

	// Called when a new buffer is required.
	virtual int32 OnGenerateAudio(float** OutAudio, int32 NumFrames, int32 NumChannels) = 0;

	// Returns the number of samples to render per callback
	virtual int32 GetDesiredNumSamplesToRenderPerCallback() const { return 1024; }

	// Optional. Called on audio generator thread right when the generator begins generating.
	virtual void OnBeginGenerate() {}

	// Optional. Called on audio generator thread right when the generator ends generating.
	virtual void OnEndGenerate() {}

	// Optional. Can be overridden to end the sound when generating is finished.
	virtual bool IsFinished() const { return false; }

	// Retrieves the next buffer of audio from the generator, called from the audio mixer
	CRIWARECORE_API int32 GetNextBuffer(float** OutAudio, int32 NumFrames, int32 NumChannels, bool bRequireNumberSamples = false);

	virtual Atom::AtomTaskQueueId GetSynchronizedRenderQueueID() const { return 0; }

protected:

	// Protected method to execute lambda in audio render thread
	// Used for conveying parameter changes or events to the generator thread.
	CRIWARECORE_API void SynthCommand(TUniqueFunction<void()> Command);

private:

	CRIWARECORE_API void PumpPendingMessages();

	// The command queue used to convey commands from game thread to generator thread 
	TQueue<TUniqueFunction<void()>> CommandQueue;

	friend class UAtomSynthComponent;
};

// Null implementation of IAtomSoundGenerator which no-ops audio generation
class FAtomSoundGeneratorNull : public IAtomSoundGenerator
{
public:
	virtual int32 OnGenerateAudio(float** OutAudio, int32 NumFrames, int32 NumChannels) override
	{
		return NumFrames * NumChannels;
	}
};

typedef TSharedPtr<IAtomSoundGenerator, ESPMode::ThreadSafe> IAtomSoundGeneratorPtr;
