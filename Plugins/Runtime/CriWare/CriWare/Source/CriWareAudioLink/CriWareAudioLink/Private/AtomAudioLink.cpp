
#include "AtomAudioLink.h"

#include "CriWare.h"
#include "Sound/SoundSubmix.h"
#include "AudioDevice.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundWaveProcedural.h"

#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/Mixer/AtomMixerSourceVoice.h"
#include "Atom/Mixer/AtomMixerSource.h"

#include "AtomAudioLinkPrivate.h"
#include "AtomAudioLinkSettings.h"
#include "AtomAudioLinkComponent.h"

DEFINE_LOG_CATEGORY(LogAtomAudioLink);
DEFINE_LOG_CATEGORY(LogAtomAudioLinkDebug);

FAtomAudioLinkSubmix::FAtomAudioLinkSubmix(FSharedBufferedOutputPtr InProducerSP, FAtomAudioLinkInputClientPtr InConsumerSP, FAudioDevice* InAudioDevice)
	: ProducerSP(InProducerSP)
	, ConsumerSP(InConsumerSP)
{
#if ATOM_TRACE_AUDIOLINK
	UE_LOG(LogAtomAudioLinkDebug, Display, TEXT("FAtomAudioLinkSubmix: Create SUBMIX link."));
#endif
}

FAtomAudioLinkSubmix::~FAtomAudioLinkSubmix()
{
	if (ConsumerSP.IsValid())
	{
		ConsumerSP->Stop();
	}
}

FAtomAudioLinkSource::FAtomAudioLinkSource(FSharedBufferedOutputPtr InProducerSP, FAtomAudioLinkInputClientPtr InConsumerSP)
	: ProducerSP(InProducerSP)
	, ConsumerSP(InConsumerSP)
{
#if ATOM_TRACE_AUDIOLINK
	UE_LOG(LogAtomAudioLinkDebug, Display, TEXT("FAtomAudioLinkSource: Create SOURCE link"));
#endif
}

FAtomAudioLinkSource::~FAtomAudioLinkSource()
{
	if (ConsumerSP.IsValid())
	{
		ConsumerSP->Stop();
	}
}

void FAtomAudioLinkSource::Teardown()
{ 
	if (ProducerSP.IsValid())
	{
		ProducerSP->SetBufferStreamEndDelegate(nullptr);
	}
}
