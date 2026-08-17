
#pragma once

#include "IAudioLink.h"
#include "IAudioLinkFactory.h"
#include "Logging/LogMacros.h"

#include "Atom/Atom.h"
#include "Atom/AtomSoundGenerator.h"

#include "AtomAudioLink.h"
#include "AtomAudioLinkComponent.h"

class UAtomAudioLinkComponent;
class FAtomAudioLinkSettingsProxy;
using FAtomAudioLinkSettingProxyPtr = TSharedPtr<FAtomAudioLinkSettingsProxy, ESPMode::ThreadSafe>;

class FAtomAudioLinkSourcePushed
	: public IAudioLinkSourcePushed
{
public:

	FAtomAudioLinkSourcePushed(const IAudioLinkFactory::FAudioLinkSourcePushedCreateArgs& InArgs, IAudioLinkFactory* InFactory);
	~FAtomAudioLinkSourcePushed();

	//~ Begin IAudioLinkSourcePushed Interface
	virtual void OnUpdateWorldState(const FOnUpdateWorldStateParams&) override;
	virtual void OnNewBuffer(const FOnNewBufferParams&) override;
	virtual void OnSourceDone(const int32 SourceId) override;
	virtual void OnSourceReleased(const int32 SourceId)override;
	//~ End IAudioLinkSourcePushed Interface

private:

	int32 SourceId = INDEX_NONE;
	int32 NumFramesReceivedSoFar = INDEX_NONE;
	FSharedBufferedOutputPtr ProducerSP;
	FAtomAudioLinkInputClientPtr ConsumerSP;
	IAudioLinkFactory::FAudioLinkSourcePushedCreateArgs CreateArgs;
};
