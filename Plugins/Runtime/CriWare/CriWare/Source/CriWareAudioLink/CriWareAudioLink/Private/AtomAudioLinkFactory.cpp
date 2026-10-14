
#include "AtomAudioLinkFactory.h"

#include "Async/Async.h"
#include "Sound/SoundSubmix.h"

#include "CriWareCoreModule.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"

#include "AtomAudioLink.h"
#include "AtomAudioLinkInputClient.h"
#include "AtomAudioLinkSourcePushed.h"
#include "AtomAudioLinkSettings.h"
#include "AtomAudioLinkSynchronizer.h"
#include "AtomAudioLinkComponent.h"

FName FAtomAudioLinkFactory::GetFactoryNameStatic()
{
	static FName FactoryName = FName(TEXT("AtomAudioLink"));
	return FactoryName;
}

FName FAtomAudioLinkFactory::GetFactoryName() const
{
	return GetFactoryNameStatic();
}

TSubclassOf<UAudioLinkSettingsAbstract> FAtomAudioLinkFactory::GetSettingsClass() const
{
	return UAtomAudioLinkSettings::StaticClass();
}

TUniquePtr<IAudioLink> FAtomAudioLinkFactory::CreateSubmixAudioLink(const FAudioLinkSubmixCreateArgs& InArgs)
{
	if (!ICriWareCoreModule::IsAvailable())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: CriWare is not available."));
		return {};
	}

	if (!InArgs.Settings.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: AtomAudioLinkSettings is invalid."));
		return {};
	}

	if (!InArgs.Submix.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: Submix is invalid."));
		return {};
	}

	UE_LOG(LogAtomAudioLink, Verbose, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: Creating AudioLink %s for Submix %s."), *InArgs.Settings->GetName(), *InArgs.Submix->GetName());

	// Get settings proxy
	const FAtomAudioLinkSettingProxyPtr AtomSettingsSP = InArgs.Settings->GetCastProxy<FAtomAudioLinkSettingsProxy>();

	// Make buffer listener first, which is our producer.
	IAudioLinkFactory::FSubmixBufferListenerCreateParams SubmixListenerCreateArgs;
	SubmixListenerCreateArgs.SizeOfBufferInFrames = AtomSettingsSP->GetReceivingBufferSizeInFrames();
	SubmixListenerCreateArgs.bShouldZeroBuffer = AtomSettingsSP->ShouldClearBufferOnReceipt();
	FSharedBufferedOutputPtr ProducerSP = CreateSubmixBufferListener(SubmixListenerCreateArgs);

	// Create mixer input client, which is the consumer in the link.
	// This take a Weak Reference to the Producer.
	FAtomAudioLinkInputClientPtr ConsumerSP = MakeShared<FAtomAudioLinkInputClient, ESPMode::ThreadSafe>(
		ProducerSP, AtomSettingsSP, InArgs.Submix->GetFName());


	// Setup a delegate to establish the link when we know the format.
	ProducerSP->SetFormatKnownDelegate(IBufferedAudioOutput::FOnFormatKnown::CreateLambda([ProducerSP, ConsumerSP, AtomSettingsSP](const IBufferedAudioOutput::FBufferFormat& InFormat)
	{
		int32 BufferSizeInSamples = AtomSettingsSP->GetReceivingBufferSizeInFrames() * InFormat.NumChannels;
		int32 ReserveSizeInSamples = (float)BufferSizeInSamples * AtomSettingsSP->GetProducerConsumerBufferRatio();
		int32 SilenceToAddToFirstBuffer = FMath::Min((float)BufferSizeInSamples * AtomSettingsSP->GetInitialSilenceFillRatio(), ReserveSizeInSamples);

		// Set circular buffer ahead of first buffer. 
		ProducerSP->Reserve(ReserveSizeInSamples, SilenceToAddToFirstBuffer);

		ConsumerSP->Stop();

		UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: Starting AudioLink Atom consumer."));
		ConsumerSP->Start();
	}));

	UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSubmixAudioLink: Starting Unreal AudioDevice Output."));
	ProducerSP->Start(InArgs.Device);

	return MakeUnique<FAtomAudioLinkSubmix>(ProducerSP, ConsumerSP, InArgs.Device);
}
        
TUniquePtr<IAudioLink> FAtomAudioLinkFactory::CreateSourceAudioLink(const FAudioLinkSourceCreateArgs& InArgs)
{
	if (!ICriWareCoreModule::IsAvailable())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: CriWare is not available."));
		return {};
	}

	if (!InArgs.Settings.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Invalid AtomAudioLinkSettings."));
		return {};
	}

	if (!InArgs.OwningComponent.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Invalid Owning Component."));
		return {};
	}

	if (!InArgs.AudioComponent.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Invalid Audio Component."));
		return {};
	}

	const UWorld* World = InArgs.OwningComponent->GetWorld();
	if (UNLIKELY(!IsValid(World)))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Invalid World in Owning Component."));
		return {};
	}

	const FAudioDeviceHandle Handle = World->GetAudioDevice();

	UE_LOG(LogAtomAudioLink, Verbose, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Creating AudioLink %s for Audio Component %s (Owning=%s; World=%s; AudioDeviceID=%d)"),
		*InArgs.Settings->GetName(), *InArgs.AudioComponent->GetName(), *InArgs.OwningComponent->GetName(), *World->GetName(), Handle.GetDeviceID());

	// Get settings proxy
	const FAtomAudioLinkSettingProxyPtr AtomSettingsSP = InArgs.Settings->GetCastProxy<FAtomAudioLinkSettingsProxy>();

	// Make buffer listener first, which is our producer.
	IAudioLinkFactory::FSourceBufferListenerCreateParams SourceListenerCreateParams;
	SourceListenerCreateParams.SizeOfBufferInFrames = AtomSettingsSP->GetReceivingBufferSizeInFrames();
	SourceListenerCreateParams.bShouldZeroBuffer = AtomSettingsSP->ShouldClearBufferOnReceipt();
	SourceListenerCreateParams.OwningComponent = InArgs.OwningComponent;
	SourceListenerCreateParams.AudioComponent = InArgs.AudioComponent;
	FSharedBufferedOutputPtr ProducerSP = CreateSourceBufferListener(SourceListenerCreateParams);

	static const FName UnknownOwner(TEXT("Unknown"));
	FName OwnerName = InArgs.OwningComponent.IsValid() ? InArgs.OwningComponent->GetFName() : UnknownOwner;

	// Create mixer input client, which is the consumer in the link.
	// This take a Weak Reference to the Producer.
	FAtomAudioLinkInputClientPtr ConsumerSP = MakeShared<FAtomAudioLinkInputClient, ESPMode::ThreadSafe>(
		ProducerSP, AtomSettingsSP, OwnerName);


	// Setup a delegate to establish the link when we know the format.
	ProducerSP->SetFormatKnownDelegate(IBufferedAudioOutput::FOnFormatKnown::CreateLambda([ProducerSP, ConsumerSP, AtomSettingsSP, OwningComponent = InArgs.OwningComponent](const IBufferedAudioOutput::FBufferFormat& InFormat)
		{
			int32 BufferSizeInSamples = AtomSettingsSP->GetReceivingBufferSizeInFrames() * InFormat.NumChannels;
			int32 ReserveSizeInSamples = (float)BufferSizeInSamples * AtomSettingsSP->GetProducerConsumerBufferRatio();
			int32 SilenceToAddToFirstBuffer = FMath::Min((float)BufferSizeInSamples * AtomSettingsSP->GetInitialSilenceFillRatio(), ReserveSizeInSamples);

			// Set circular buffer ahead of first buffer. 
			ProducerSP->Reserve(ReserveSizeInSamples, SilenceToAddToFirstBuffer);

			AsyncTask(ENamedThreads::GameThread, [ConsumerSP, OwningComponent]()
			{
				if (OwningComponent.IsValid())
				{
					ConsumerSP->Stop();

					UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Starting AudioLink Source consumer."));
					ConsumerSP->Start(Cast<UAtomAudioLinkComponent>(OwningComponent.Get()));
				}
			});
		}));

	ProducerSP->SetBufferStreamEndDelegate(IBufferedAudioOutput::FOnBufferStreamEnd::CreateLambda([ConsumerSP](const IBufferedAudioOutput::FBufferStreamEnd& InBufferStreamEndParams)
	{
		UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Stopping AudioLink Source consumer."));
		
		const int32 SourceID = InBufferStreamEndParams.Id;
		ConsumerSP->Stop(SourceID);
	}));

	// Tell the Producer to Start receiving buffers from Sources.
	// Pass a Lambda to do the some work when we know the Format, which starts Atom up.
	UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSourceAudioLink: Starting Unreal AudioDevice Output."));
	ProducerSP->Start(Handle ? Handle.GetAudioDevice() : nullptr);

	// Make the link
	return MakeUnique<FAtomAudioLinkSource>(ProducerSP, ConsumerSP);
}

IAudioLinkFactory::FAudioLinkSourcePushedSharedPtr FAtomAudioLinkFactory::CreateSourcePushedAudioLink(const FAudioLinkSourcePushedCreateArgs& InArgs)
{
	if (!GCriWare || !GCriWare->IsInitialized())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkFactory::CreateSourcePushedAudioLink: CriWare is not initialized."));
		return nullptr;
	}

	UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSourcePushedAudioLink: Create AudioLink SourcePushed."));
	return MakeShared<FAtomAudioLinkSourcePushed, ESPMode::ThreadSafe>(InArgs, this);
}

IAudioLinkFactory::FAudioLinkSynchronizerSharedPtr FAtomAudioLinkFactory::CreateSynchronizerAudioLink()
{
	UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("FAtomAudioLinkFactory::CreateSynchronizerAudioLink: Create AudioLink Synchronizer."));
	auto SychronizerSP = MakeShared<FAtomAudioLinkSynchronizer, ESPMode::ThreadSafe>();
	return SychronizerSP;
}
