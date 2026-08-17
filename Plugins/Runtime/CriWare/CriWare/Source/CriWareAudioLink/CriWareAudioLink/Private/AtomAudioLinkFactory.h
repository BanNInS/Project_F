
#pragma once

#include "IAudioLink.h"
#include "IAudioLinkFactory.h"

// Forward Declarations
struct FAtomAudioLinkSynchronizer;
using FAtomAudioLinkSynchronizerPtr = TSharedPtr<FAtomAudioLinkSynchronizer>;

class FAtomAudioLinkFactory
	: public IAudioLinkFactory
{
public:
	FAtomAudioLinkFactory() = default;
	virtual ~FAtomAudioLinkFactory() = default;

	static FName GetFactoryNameStatic();

protected:

	//~ Begin IAudioLinkFactory Interface
	FName GetFactoryName() const override;
	TSubclassOf<UAudioLinkSettingsAbstract> GetSettingsClass() const override;
	TUniquePtr<IAudioLink> CreateSubmixAudioLink(const FAudioLinkSubmixCreateArgs& InArgs) override;
	TUniquePtr<IAudioLink> CreateSourceAudioLink(const FAudioLinkSourceCreateArgs& InArgs) override;
	FAudioLinkSourcePushedSharedPtr CreateSourcePushedAudioLink(const FAudioLinkSourcePushedCreateArgs& InArgs) override;
	FAudioLinkSynchronizerSharedPtr CreateSynchronizerAudioLink() override;
	//~ End IAudioLinkFactory Interface
};
