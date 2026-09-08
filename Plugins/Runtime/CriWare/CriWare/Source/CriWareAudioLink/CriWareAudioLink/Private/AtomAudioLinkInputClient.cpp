
#include "AtomAudioLinkInputClient.h"

#include "CriWare.h"
#include "Sound/SoundSubmix.h"
#include "AudioDevice.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundWaveProcedural.h"

#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/Mixer/AtomMixerSourceVoice.h"
#include "Atom/Mixer/AtomMixerSource.h"

#include "AtomAudioLink.h"
#include "AtomAudioLinkSettings.h"
#include "AtomAudioLinkComponent.h"

FAtomAudioLinkInputClient::FAtomAudioLinkInputClient(const FSharedBufferedOutputPtr& InProducerSP, const FAtomAudioLinkSettingProxyPtr& InSettingsSP, FName InName)
	: ProducerName(InName)
	, WeakProducer(InProducerSP)
	, SettingsSP(InSettingsSP)
	, bStarted(false)
{
	Register(ProducerName);
}

FAtomAudioLinkInputClient::~FAtomAudioLinkInputClient()
{
	Unregister();
}

void FAtomAudioLinkInputClient::Start(UAtomAudioLinkComponent* InComponent)
{
	check(IsInGameThread());

	if (UNLIKELY(!IsValid(InComponent)))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkInputClient::Start: Invalid component (this=%p %s)"), this, *ProducerName.ToString());
		return;
	}

	FAtomAudioLinkSettingsProxy* Settings = static_cast<FAtomAudioLinkSettingsProxy*>(SettingsSP.Get());
	if (UNLIKELY(!Settings))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkInputClient::Start: Invalid Settings (this=%p %s)"), this, *ProducerName.ToString());
		return;
	}

	// if our engine is not ready defere this call when ready
	if (!GCriWare)
	{
		RuntimeActivatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.AddLambda([WeakThis = AsWeak(), InComponent](FAtomRuntimeId RuntimeID)
		{
			if (auto ThisClient = WeakThis.Pin())
			{
				ThisClient->Start(InComponent);
			}
		});

		return;
	}

	FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.Remove(RuntimeActivatedHandle);

	if (TrySetStartedFlag())
	{
		SCOPED_NAMED_EVENT(AtomAudioLink_StartComponent, FColor::Red);

		auto Runtime = FAtomRuntimeManager::Get()->GetActiveAtomRuntime();
		check(Runtime);
		Runtime->GameThreadMPSCCommand([ClientSP = AsShared(), SettingsSP = SettingsSP, LinkComponent = InComponent]()
		{
			auto Producer = ClientSP->WeakProducer.Pin();
			if (!Producer)
			{
				UE_LOG(LogAtomAudioLink, Error, TEXT("Audio Link buffer is invalid."));
				return;
			}

			IBufferedAudioOutput::FBufferFormat OutFormat;
			Producer->GetFormat(OutFormat);

			ClientSP->WeakLinkComponent = LinkComponent;
			ClientSP->AtomComponentID = INDEX_NONE;
			ClientSP->UnrealFormat = OutFormat;

			// inform that we are ready to read the link
			LinkComponent->OnLinkStart(ClientSP);
		});
	}
}

// create a new PCM voice.
void FAtomAudioLinkInputClient::Start()
{
	// this will only start when atom runtime is ready
	// current audiolink does not check is we ready or not.

	// if our engine is not ready defere this call when ready
	if (!GCriWare)
	{
		RuntimeActivatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.AddLambda([WeakThis = AsWeak()](FAtomRuntimeId RuntimeID)
		{
			if (auto ThisClient = WeakThis.Pin())
			{
				ThisClient->Start();
			}
		});

		return;
	}

	FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.Remove(RuntimeActivatedHandle);

	if (TrySetStartedFlag())
	{
		SCOPED_NAMED_EVENT(AtomAudioLink_Start, FColor::Green);

		auto Runtime = FAtomRuntimeManager::Get()->GetActiveAtomRuntime();
		check(Runtime);
		Runtime->GameThreadMPSCCommand([ClientSP = AsShared(), SettingsSP = SettingsSP]()
		{
			auto Producer = ClientSP->WeakProducer.Pin();
			if (!Producer)
			{
				UE_LOG(LogAtomAudioLink, Error, TEXT("Audio Link buffer is invalid."));
				return;
			}

			IBufferedAudioOutput::FBufferFormat OutFormat;
			Producer->GetFormat(OutFormat);

			// link tru synth component version

			// create component
			UAtomAudioLinkComponent* LinkComponent = NewObject<UAtomAudioLinkComponent>();

			if (LinkComponent)
			{
				LinkComponent->bAutoActivate = false;
				LinkComponent->bStopWhenOwnerDestroyed = true;
				//LinkComponent->bShouldRemainActiveIfDropped = true;
				LinkComponent->Mobility = EComponentMobility::Movable;

#if WITH_EDITORONLY_DATA
				LinkComponent->bVisualizeComponent = false;
#endif
				// keep away form GC
				LinkComponent->AddToRoot();

				ClientSP->WeakLinkComponent = LinkComponent;
				ClientSP->AtomComponentID = INDEX_NONE;
				ClientSP->UnrealFormat = OutFormat;

				// inform that we are ready to read the link
				LinkComponent->OnLinkStart(ClientSP);
			}

			// direct playback version
			/*FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UAtomAudioLinkSound::StaticClass(), ClientSP->ProducerName);
			UAtomAudioLinkSound* LinkSound = NewObject<UAtomAudioLinkSound>(GetTransientPackage(), UniqueName);
			LinkSound->Init(ClientSP);

			//InputWave->OutFormat = EAtomPcmBitDepth::Float32;

			UAtomComponent* InputComponent = NewObject<UAtomComponent>(); // optional?
			InputComponent->SetSound(LinkSound); // keep
			InputComponent->AddToRoot(); // keep away form GC

			ClientSP->AtomComponentID = InputComponent->GetAtomComponentID();
			ClientSP->UnrealFormat = OutFormat;

			FAtomActiveSound NewActiveSound;
			NewActiveSound.SetSound(LinkSound); // need!
			NewActiveSound.SetAtomComponent(*InputComponent); // optional
			NewActiveSound.PcmFilterFunction = [ClientSP](EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
			{
				SCOPED_NAMED_EVENT(AtomAudioLink_Data, FColor::Red);
				// todo: check if on atom render thread

				ClientSP->GetPCMSamples(NumChannels, NumFrames, (float**)Data);
			};

			// send active sound to runtime and start
			auto Runtime = FAtomRuntimeManager::Get()->GetActiveAtomRuntime();
			Runtime->AddNewActiveSound(NewActiveSound);*/
		});
	}
}

// destroy the PCM voice.
void FAtomAudioLinkInputClient::Stop(int32 SourceID)
{
	SCOPED_NAMED_EVENT(AtomAudioLink_Stop, FColor::Green);

	if (TryUnsetStartedFlag())
	{
		// synth component version
		if (UAtomAudioLinkComponent* LinkComponent = WeakLinkComponent.Get())
		{
			LinkComponent->OnLinkStop(this->AsShared(), SourceID);
		}

		// Atom Component version
		/*if (AtomComponentID != INDEX_NONE && GCriWare)
		{
			FAtomRuntimeHandle Runtime = FAtomRuntimeManager::Get()->GetActiveAtomRuntime();
			if (Runtime.IsValid())
			{
				Runtime->StopActiveSound(AtomComponentID);
			}
		}*/
	}
}

void FAtomAudioLinkInputClient::Register(const FName& InName)
{

}

// detatch from mixer rawpcm voice
void FAtomAudioLinkInputClient::Unregister()
{
	// synth component version
	if (UObject* LinkComponent = WeakLinkComponent.Get())
	{
		LinkComponent->RemoveFromRoot();
		WeakLinkComponent.Reset();
	}

	// Atom Component version
	/*if (AtomComponentID != INDEX_NONE)
	{
		if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
		{
			AtomComponent->RemoveFromRoot();
			AtomComponentID = INDEX_NONE;
		}
	}*/
}

bool FAtomAudioLinkInputClient::TrySetStartedFlag()
{
	// We expect not to be started.
	bool bExpected = false;
	bool bDesired = true;
	return bStarted.compare_exchange_strong(bExpected, bDesired);
}

bool FAtomAudioLinkInputClient::TryUnsetStartedFlag()
{
	// We expect be started.
	bool bExpected = true;
	bool bDesired = false;
	return bStarted.compare_exchange_strong(bExpected, bDesired);
}

// Called from Atom render thread thru IAtomSoundGenertor.
bool FAtomAudioLinkInputClient::GetPCMSamples(uint32 InNumChannels, uint32 InNumFrames, float** InChannelBuffers)
{
	FSharedBufferedOutputPtr StrongBufferProducer = WeakProducer.Pin();
	if (!StrongBufferProducer.IsValid())
	{
		// Return false, to indicate no more data.
		return false;
	}

	int32 NumFramesPopped = 0;
	bool bMoreDataRemaining = false;

	// Always zero the buffer before we start.
	for (uint32 Channel = 0; Channel < InNumChannels; ++Channel)
	{
		FMemory::Memzero(InChannelBuffers[Channel], InNumFrames * sizeof(float));
	}

	if (UnrealFormat.NumChannels == 1)
	{
		// Pop the data directly onto Atom buffers.
		// Keep record if the Producer has told us there's no more data.
		bMoreDataRemaining = StrongBufferProducer->PopBuffer(InChannelBuffers[0], InNumFrames, NumFramesPopped);
	}
	else
	{
		// Make sure we have enough space in our temp buffer
		int32 NumInterleavedSamplesNeeded = InNumFrames * UnrealFormat.NumChannels;
		if (NumInterleavedSamplesNeeded > InterleavedBuffer.Num())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			InterleavedBuffer.SetNumUninitialized(NumInterleavedSamplesNeeded, EAllowShrinking::Yes);
#else
			InterleavedBuffer.SetNumUninitialized(
				NumInterleavedSamplesNeeded,
				true // bAllowShrinking
			);
#endif
		}

		// Pop the data onto an intermediate buffer for deinterleaving.
		// Keep record if the Producer has told us there's no more data.
		int32 NumInterleavedSamplesPopped = 0;
		bMoreDataRemaining = StrongBufferProducer->PopBuffer(InterleavedBuffer.GetData(), NumInterleavedSamplesNeeded, NumInterleavedSamplesPopped);
		NumFramesPopped = NumInterleavedSamplesPopped / UnrealFormat.NumChannels;

		if (NumFramesPopped > 0)
		{
			// De-interleave.
			switch (UnrealFormat.NumChannels)
			{
			// Stereo.
			case 2:
			{
				Audio::BufferDeinterleave2ChannelFast(InterleavedBuffer.GetData(), InChannelBuffers[0], InChannelBuffers[1], NumFramesPopped);
				break;
			}
			// Generic version. 
			default:
			{
				const float* InterleavedPtr = InterleavedBuffer.GetData();
				for (int32 Frame = 0; Frame < NumFramesPopped; ++Frame)
				{
					for (int32 Channel = 0; Channel < UnrealFormat.NumChannels; ++Channel)
					{
						InChannelBuffers[Channel][Frame] = *InterleavedPtr++;
					}
				}
				break;
			}
			}
		}
	}

	return true;
}

// Called from Atom render thread thru IAtomSoundGenertor.
void FAtomAudioLinkInputClient::GetPCMFormat(EAtomPcmBitDepth& InFormat)
{
	InFormat = EAtomPcmBitDepth::Float32;
}

// Helper to grab format as Atom WaveInfo.
void FAtomAudioLinkInputClient::GetWaveInfo(FAtomWaveInfo& OutWaveInfo)
{
	OutWaveInfo.WaveID = INDEX_NONE;
	OutWaveInfo.Format = EAtomFormat::AudioBuffer; // procedural
	OutWaveInfo.SampleRate = UnrealFormat.NumSamplesPerSec;
	OutWaveInfo.NumChannels = UnrealFormat.NumChannels;
	OutWaveInfo.NumFrames = UnrealFormat.NumSamplesPerBlock / UnrealFormat.NumChannels;
	OutWaveInfo.bIsStreamed = false;
}

// Called from the Consumer Thread, at Game-tick rate.
void FAtomAudioLinkInputClient::UpdateWorldState(const FWorldState& InParams)
{
	if (UAtomAudioLinkComponent* LinkComponent = WeakLinkComponent.Get())
	{
		// logdebug
		UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT(" LinkComponent=%s, Name=%s, This=0x%p"), *LinkComponent->GetFName().ToString(), *ProducerName.GetPlainNameString(), this);

		const FTransform& T = InParams.WorldTransform;

		LinkComponent->SetWorldTransform(T);
	}
}
