
#pragma once

#include "IAudioLink.h"
#include "IAudioLinkFactory.h"
#include "Logging/LogMacros.h"

#include "Atom/Atom.h"
#include "Atom/AtomSoundGenerator.h"

#include "AtomAudioLinkInputClient.h"

#define ATOM_TRACE_AUDIOLINK 0

class FAtomAudioLinkSubmix
	: public IAudioLink
{
public:

	FAtomAudioLinkSubmix(FSharedBufferedOutputPtr ProducerSP, FAtomAudioLinkInputClientPtr ConsumerSP, FAudioDevice* InAudioDevice);
	virtual ~FAtomAudioLinkSubmix();

private:

	// Circular buffer (submix/source) that listening for new buffers from AudioDevice.
	FSharedBufferedOutputPtr ProducerSP;

	// Atom Mixer Input
	FAtomAudioLinkInputClientPtr ConsumerSP;
};

class FAtomAudioLinkSource
	: public IAudioLink
{
public:

	FAtomAudioLinkSource(FSharedBufferedOutputPtr ProducerSP, FAtomAudioLinkInputClientPtr ConsumerSP);
	virtual ~FAtomAudioLinkSource();

	void Teardown();

private:

	// Circular buffer (submix/source) that listening for new buffers from AudioDevice.
	FSharedBufferedOutputPtr ProducerSP;

	// Atom Mixer Input
	FAtomAudioLinkInputClientPtr ConsumerSP;
};
