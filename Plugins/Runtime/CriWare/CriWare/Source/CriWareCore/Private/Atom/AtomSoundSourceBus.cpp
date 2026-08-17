
#include "Atom/AtomSoundSourceBus.h"

#include "Atom/Atom.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomSoundSourceBus)

UAtomSoundSourceBus::UAtomSoundSourceBus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// This is a bus. Audio will be injected by PCM filter to AtomMixerSource.
	bIsSourceBus = true;

	Init();
}

void UAtomSoundSourceBus::PostLoad()
{
	Super::PostLoad();

	Init();
}

void UAtomSoundSourceBus::Init()
{
	// Allow users to manually set the source bus duration
	Duration = GetDuration();

	// This sound is looping if the source bus duration is 0.0f
	bLooping = (SourceBusDuration == 0.0f);

	// Set the channels equal to the users channel count choice
	switch (SourceBusChannels)
	{
	case EAtomSourceBusChannels::Mono:
		WaveInfo.NumChannels = 1;
		break;

	case EAtomSourceBusChannels::Stereo:
		WaveInfo.NumChannels = 2;
		break;
	}

	WaveInfo.SampleRate = 48000;
	WaveInfo.NumFrames = 1024; // prefredcallbacksize -> procedural
	WaveInfo.bIsStreamed = false;
	WaveInfo.Format = EAtomFormat::AudioBuffer;
}

#if WITH_EDITOR
void UAtomSoundSourceBus::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Init();
	MaxDistance = ComputeMaxDistance();
}
#endif

bool UAtomSoundSourceBus::IsPlayable() const
{
	return true;
}

float UAtomSoundSourceBus::GetDuration() const
{
	return (SourceBusDuration > 0.0f) ? SourceBusDuration : ATOM_INDEFINITELY_LOOPING_DURATION;
}
