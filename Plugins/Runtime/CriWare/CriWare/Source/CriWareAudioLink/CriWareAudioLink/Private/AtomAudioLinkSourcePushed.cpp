
#include "AtomAudioLinkSourcePushed.h"

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

#include "AtomAudioLink.h"
#include "AtomAudioLinkPrivate.h"
#include "AtomAudioLinkSettings.h"
#include "AtomAudioLinkComponent.h"

FAtomAudioLinkSourcePushed::FAtomAudioLinkSourcePushed(const IAudioLinkFactory::FAudioLinkSourcePushedCreateArgs& InArgs, IAudioLinkFactory* InFactory)
	: CreateArgs(InArgs)
{
#if ATOM_TRACE_AUDIOLINK
	UE_LOG(LogAtomAudioLinkDebug, Display, TEXT("FAtomAudioLinkSourcePushed: Create SOURCE PUSHED link."));
#endif

	const FAtomAudioLinkSettingsProxy* AtomSettings = static_cast<FAtomAudioLinkSettingsProxy*>(InArgs.Settings.Get());

	auto SettingSP = StaticCastSharedPtr<FAtomAudioLinkSettingsProxy>(InArgs.Settings);

	IAudioLinkFactory::FPushedBufferListenerCreateParams Params;
	Params.SizeOfBufferInFrames = InArgs.NumFramesPerBuffer;
	Params.bShouldZeroBuffer = AtomSettings->ShouldClearBufferOnReceipt();

	ProducerSP = InFactory->CreatePushableBufferListener(Params);
	ConsumerSP = MakeShared<FAtomAudioLinkInputClient, ESPMode::ThreadSafe>(ProducerSP, SettingSP, InArgs.OwnerName);

	int32 BufferSizeInSamples = AtomSettings->GetReceivingBufferSizeInFrames() * InArgs.NumChannels;
	int32 ReserveSizeInSamples = (float)BufferSizeInSamples * AtomSettings->GetProducerConsumerBufferRatio();
	int32 SilenceToAddToFirstBuffer = FMath::Min((float)BufferSizeInSamples * AtomSettings->GetInitialSilenceFillRatio(), ReserveSizeInSamples);

	// Set circular buffer ahead of first buffer. 
	ProducerSP->Reserve(ReserveSizeInSamples, SilenceToAddToFirstBuffer);

	UE_LOG(LogAtomAudioLinkDebug, Verbose,
		TEXT("Create source pushed AudioLink: Name=%s, Producer=0x%p, Consumer=0x%p, p2c%%=%2.2f, TotalFramesForSource=%d, This=0x%p"),
		*InArgs.OwnerName.GetPlainNameString(), ProducerSP.Get(),
		ConsumerSP.Get(), AtomSettings->GetProducerConsumerBufferRatio(), CreateArgs.TotalNumFramesInSource, this);
}

FAtomAudioLinkSourcePushed::~FAtomAudioLinkSourcePushed()
{
	UE_LOG(LogAtomAudioLinkDebug, Verbose,
		TEXT("Create source pushed AudioLink: Name=%s, Producer=0x%p, Consumer=0x%p, RecievedFrames=%d/%d, This=0x%p"),
		*CreateArgs.OwnerName.GetPlainNameString(), ProducerSP.Get(), ConsumerSP.Get(), NumFramesReceivedSoFar, CreateArgs.TotalNumFramesInSource, this);

	ConsumerSP->Stop();
}

// Called by AtomThread, not the AudioRenderThread
void FAtomAudioLinkSourcePushed::OnUpdateWorldState(const FOnUpdateWorldStateParams& InParams)
{
	FAtomAudioLinkInputClient::FWorldState UpdateParams;
	UpdateParams.WorldTransform = InParams.WorldTransform;
	ConsumerSP->UpdateWorldState(UpdateParams);
}

void FAtomAudioLinkSourcePushed::OnNewBuffer(const FOnNewBufferParams& InArgs)
{ 
	UE_LOG(LogAtomAudioLinkDebug, VeryVerbose,
		TEXT("New buffer Name=%s, Producer=0x%p, Consumer=0x%p, SourceID=%d, RecievedFrames=%d/%d, This=0x%p"),
		*CreateArgs.OwnerName.GetPlainNameString(), ProducerSP.Get(), ConsumerSP.Get(), SourceId, NumFramesReceivedSoFar, CreateArgs.TotalNumFramesInSource,
		this);

	NumFramesReceivedSoFar += CreateArgs.NumFramesPerBuffer;

	if (SourceId == INDEX_NONE)
	{
		SourceId = InArgs.SourceId;
		ProducerSP->Start(nullptr /*AudioDevice*/);
		ConsumerSP->Start();
	}
	check(SourceId == InArgs.SourceId);

	IPushableAudioOutput* Pushable = ProducerSP->GetPushableInterface();
	if (ensure(Pushable))
	{
		IPushableAudioOutput::FOnNewBufferParams Params;
		Params.AudioData = InArgs.Buffer.GetData();
		Params.NumSamples = InArgs.Buffer.Num();
		Params.Id = InArgs.SourceId;
		Params.NumChannels = CreateArgs.NumChannels;
		Params.SampleRate = CreateArgs.SampleRate;
		Pushable->PushNewBuffer(Params);
	}
}

void FAtomAudioLinkSourcePushed::OnSourceDone(const int32 InSourceId)
{ 
	UE_LOG(LogAtomAudioLinkDebug, Verbose,
		TEXT("OnSourceDone: Name=%s, Producer=0x%p, Consumer=0x%p, RecievedFrames=%d/%d, This=0x%p"),
		*CreateArgs.OwnerName.GetPlainNameString(), ProducerSP.Get(), ConsumerSP.Get(), NumFramesReceivedSoFar, CreateArgs.TotalNumFramesInSource, this);

	check(SourceId == InSourceId);

	IPushableAudioOutput* Pushable = ProducerSP->GetPushableInterface();
	if (ensure(Pushable))
	{
		Pushable->LastBuffer(SourceId);
	}

	SourceId = INDEX_NONE;
}

void FAtomAudioLinkSourcePushed::OnSourceReleased(const int32 InSourceId)
{ 
	UE_LOG(LogAtomAudioLinkDebug, Verbose,
		TEXT("OnSourceReleased: Name=%s, Producer=0x%p, Consumer=0x%p, RecievedFrames=%d/%d, This=0x%p"),
		*CreateArgs.OwnerName.GetPlainNameString(), ProducerSP.Get(), ConsumerSP.Get(), NumFramesReceivedSoFar, CreateArgs.TotalNumFramesInSource, this);
}
