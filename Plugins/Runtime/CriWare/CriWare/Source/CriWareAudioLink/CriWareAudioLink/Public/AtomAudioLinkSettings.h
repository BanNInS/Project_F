
#pragma once

#include "IAudioLinkFactory.h"
#include "Engine/StreamableManager.h"
#include "UObject/Object.h"

#include "Atom/Atom.h"

#include "AtomAudioLinkSettings.generated.h"

// Forward Declarations
class UAtomBus;
class UAtomAudioLinkSettings;

UENUM()
enum class EAtomAudioLinkRoute : uint8
{
	Source UMETA(DisplayName = "As Sound Source"),

	Bus UMETA(DisplayName = "Direct to Bus")
};

class FAtomAudioLinkSettingsProxy
	: public IAudioLinkSettingsProxy
{
public:

	FAtomAudioLinkSettingsProxy(const UAtomAudioLinkSettings&);
	virtual ~FAtomAudioLinkSettingsProxy() = default;

	void Update(const UAtomAudioLinkSettings&);

	bool IsRoutingToBus() const { return DestinationRoute == EAtomAudioLinkRoute::Bus; }
	const TSoftObjectPtr<UAtomBus>& GetDestinationBus() const { return DestinationBus; }
	EAtomBusSendStage GetDestinationBusStage() const { return DestinationBusStage; }
	int GetReceivingBufferSizeInFrames() const { return ReceivingBufferSizeInFrames; }
	bool ShouldClearBufferOnReceipt() const { return bShouldZeroBuffer; }
	float GetProducerConsumerBufferRatio() const { return ProducerToConsumerBufferRatio; }
	float GetInitialSilenceFillRatio() const { return InitialSilenceFillRatio; }

private:

#if WITH_EDITOR
	void RefreshFromSettings(UAudioLinkSettingsAbstract* InSettings, FPropertyChangedEvent& InPropertyChangedEvent) override;
#endif //WITH_EDITOR

	EAtomAudioLinkRoute DestinationRoute = EAtomAudioLinkRoute::Source;
	TSoftObjectPtr<UAtomBus> DestinationBus = nullptr;
	EAtomBusSendStage DestinationBusStage;

	int32 ReceivingBufferSizeInFrames;
	bool bShouldZeroBuffer = true;
	float ProducerToConsumerBufferRatio = 2.0f;
	float InitialSilenceFillRatio = 1.0f;
};

using FAtomAudioLinkSettingProxyPtr = TSharedPtr<FAtomAudioLinkSettingsProxy, ESPMode::ThreadSafe>;

UCLASS(config = CriWare, DefaultConfig)
class CRIWARECORE_API UAtomAudioLinkSettings
	: public UAudioLinkSettingsAbstract
{
    GENERATED_BODY()

public:

	/** Whether use link as a sound source or route audio to an Atom DSP bus. */
	UPROPERTY(Config, EditAnywhere, Category = "Routing")
	EAtomAudioLinkRoute DestinationRoute = EAtomAudioLinkRoute::Source;

	/** The destination DSP bus to send audio. */
	UPROPERTY(Config, EditAnywhere, Category = "Routing", meta = (EditCondition = "DestinationRoute == EAtomAudioLinkRoute::Bus"))
	TSoftObjectPtr<UAtomBus> DestinationBus;

	/** The stage of the destination DSP bus to send audio.*/
	UPROPERTY(Config, EditAnywhere, Category = "Routing", meta = (EditCondition = "DestinationRoute == EAtomAudioLinkRoute::Bus"))
	EAtomBusSendStage DestinationBusStage = EAtomBusSendStage::PreEffect;
	
	/** If this is set, the receiving code will clear the buffer after it's read, so it's not rendered by Unreal. Only applies if running both renderers at once.  */
	UPROPERTY(Config, EditAnywhere, Category = "AudioLink")
	bool bShouldClearBufferOnReceipt = true;

	/** This is the ratio of producer to consumer buffer size, 2.0 means its twice as big as the consumer buffer.  */
	UPROPERTY(Config, EditAnywhere, Category = "AudioLink")
	float ProducerToConsumerBufferRatio = 2.0f;

	/** Ratio of initial buffer to fill with silence ahead of consumption. Adjusting this can resolve starvation at the cost of added latency. */
	UPROPERTY(Config, EditAnywhere, Category = "AudioLink")
	float InitialSilenceFillRatio = 1.0f;

protected:

	//mutable bool bLoadRequested = false;
	TSharedPtr<FStreamableHandle> LoadingHandle;

	/** Once the SoftObjectReference has been resolved, attach the reference here so it's owned. */
	UPROPERTY(Transient)
	TObjectPtr<UAtomBus> DestinationBusResolved;

	friend class FAtomAudioLinkSettingsProxy;

	int32 GetReceivingBufferSizeInFrames() const;

	virtual FSharedSettingsProxyPtr MakeProxy() const override;

	virtual FName GetFactoryName() const override;
};
