
#include "AtomAudioLinkSettings.h"
#include "AtomAudioLinkFactory.h"
#include "AtomAudioLink.h"

#include "CriWare.h"
#include "CriWareCore.h"

FAtomAudioLinkSettingsProxy::FAtomAudioLinkSettingsProxy(const UAtomAudioLinkSettings& OtherSettings)
{
	Update(OtherSettings);
}

void FAtomAudioLinkSettingsProxy::Update(const UAtomAudioLinkSettings& InSettings)
{
	DestinationRoute = InSettings.DestinationRoute;
	DestinationBus = InSettings.DestinationBus;
	DestinationBusStage = InSettings.DestinationBusStage;
	ReceivingBufferSizeInFrames = InSettings.GetReceivingBufferSizeInFrames();
	ProducerToConsumerBufferRatio = InSettings.ProducerToConsumerBufferRatio;
	InitialSilenceFillRatio = InSettings.InitialSilenceFillRatio;
	bShouldZeroBuffer = InSettings.bShouldClearBufferOnReceipt;
}


#if WITH_EDITOR
void FAtomAudioLinkSettingsProxy::RefreshFromSettings(UAudioLinkSettingsAbstract* InSettings, FPropertyChangedEvent& InPropertyChangedEvent)
{
	UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkSettingsProxy::RefreshFromSettings."));
	Update(*CastChecked<UAtomAudioLinkSettings>(InSettings));
}
#endif // WITH_EDITOR

int32 UAtomAudioLinkSettings::GetReceivingBufferSizeInFrames() const
{
	if (GCriWare && GCriWare->IsInitialized())
	{
		FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
		if (RuntimeManager)
		{
			return RuntimeManager->GetDefaultDSPBufferLength();
		}
	}

	static const int32 SensibleDefaultSize = 1024;
	UE_LOG(LogAtomAudioLink, Warning, TEXT("AtomAudioLinkSettings: Failed to get Atom settings for buffer size, defaulting to '%d'"), SensibleDefaultSize);
	return SensibleDefaultSize;
}

FName UAtomAudioLinkSettings::GetFactoryName() const
{
	return FAtomAudioLinkFactory::GetFactoryNameStatic();
}

UAudioLinkSettingsAbstract::FSharedSettingsProxyPtr UAtomAudioLinkSettings::MakeProxy() const
{
	return MakeShared<FAtomAudioLinkSettingsProxy>(*this);
}
