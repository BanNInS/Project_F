
#include "AtomAudioLinkComponent.h"

#include "AtomAudioLink.h"
#include "AtomAudioLinkFactory.h"
#include "AtomAudioLinkSettings.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "ActiveSound.h"

#include "Components/AudioComponent.h"
#include "Atom/AtomComponent.h"

void FAtomAudioLinkSoundGeneratorInitParams::Reset(FAtomAudioLinkSoundGeneratorInitParams& InParams)
{
	InParams.ClientSP.Reset();
}

FAtomAudioLinkSoundGenerator::FAtomAudioLinkSoundGenerator(FAtomAudioLinkSoundGeneratorInitParams&& InInitParams)
	: Params(MoveTemp(InInitParams))
{
}

// Called when a new buffer is required by Atom ADX render thread.
int32 FAtomAudioLinkSoundGenerator::OnGenerateAudio(float** OutAudio, int32 NumFrames, int32 NumChannels)
{
	if (Params.ClientSP)
	{
		SCOPED_NAMED_EVENT(AtomAudioLink_Data, FColor::Red);
		if (Params.ClientSP->GetPCMSamples(NumChannels, NumFrames, OutAudio))
		{
			return NumChannels * NumFrames;
		}
	}

	return 0;
}

bool FAtomAudioLinkSoundGenerator::IsFinished() const
{
	return !Params.ClientSP.IsValid() || !Params.ClientSP->WeakProducer.IsValid();
}

// Returns the number of samples to render per callback
int32 FAtomAudioLinkSoundGenerator::GetDesiredNumSamplesToRenderPerCallback() const
{
	return Params.ClientSP.IsValid() ? Params.ClientSP->UnrealFormat.NumSamplesPerBlock : 8192;
}

/*
 * Sound that recieves audio from an AudioLink and outputs it into Atom using a SoundWaveProcedural.
 */

UAtomAudioLinkSound::UAtomAudioLinkSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomAudioLinkSound::Init(FAtomAudioLinkInputClientPtr InClientSP)
{
	ClientSP = InClientSP;
	ClientSP->GetWaveInfo(WaveInfo);
}

IAtomSoundGeneratorPtr UAtomAudioLinkSound::CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams)
{
	FAtomAudioLinkSoundGeneratorInitParams InitParams
	{
		ClientSP
	};
	auto SoundGenerator = MakeShared<FAtomAudioLinkSoundGenerator, ESPMode::ThreadSafe>(MoveTemp(InitParams));

	return IAtomSoundGeneratorPtr(SoundGenerator);
}


/*
 * Component that recieves audio from an AudioLink and outputs it into Atom using a SynthComponent.
 */
UAtomAudioLinkComponent::UAtomAudioLinkComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PreferredBufferLength = 512;
	NumChannels = 1;
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(true);
	bAutoActivate = false; // wait for links and use playlink() in BP or start() if from c++
};

void UAtomAudioLinkComponent::OnLinkStart(FAtomAudioLinkInputClientPtr InClientSP)
{
	FAtomAudioLinkSoundGeneratorInitParams InitParams
	{
		InClientSP
	};
	SoundGenerator = MakeShared<FAtomAudioLinkSoundGenerator, ESPMode::ThreadSafe>(MoveTemp(InitParams));

	NumChannels = InClientSP->UnrealFormat.NumChannels;
	PreferredBufferLength = InClientSP->UnrealFormat.NumSamplesPerBlock;

	// initialize the sythn with new params from unreal link
	Initialize(InClientSP->UnrealFormat.NumSamplesPerSec);
	Start();
}

void UAtomAudioLinkComponent::OnLinkStop(FAtomAudioLinkInputClientPtr InClientSP, int32 SourceID)
{
	Stop();
}

IAtomSoundGeneratorPtr UAtomAudioLinkComponent::CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams)
{
	return SoundGenerator;
}

void UAtomAudioLinkComponent::OnEndGenerate(IAtomSoundGeneratorPtr Generator)
{
}

void UAtomAudioLinkComponent::CreateLink()
{
    if (!Settings)
    {
        Settings = GetMutableDefault<UAtomAudioLinkSettings>();
    }

    IAudioLinkFactory* Factory = IAudioLinkFactory::FindFactory(FAtomAudioLinkFactory::GetFactoryNameStatic());
    if (Factory)
    {
        UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("UAtomAudioLinkComponent::CreateLink."));
        IAudioLinkFactory::FAudioLinkSourceCreateArgs CreateArgs;
        CreateArgs.OwningComponent = this;
        CreateArgs.AudioComponent = AudioComponent;
        CreateArgs.Settings = Settings;
        LinkInstance = Factory->CreateSourceAudioLink(CreateArgs);
    }
}

void UAtomAudioLinkComponent::ResetLink()
{
	// Removes listener's end delgate to avoid call to stop() from AudioMixer 
	// in case previous sound source is stopped after the start of a new sound.
	if (FAtomAudioLinkSource* LinkSourceInstance = static_cast<FAtomAudioLinkSource*>(LinkInstance.Get()))
	{
		LinkSourceInstance->Teardown();
	}

	// Then we re-create a clean link for the new source.
	CreateLink();
}

void UAtomAudioLinkComponent::CreateAudioComponent()
{
    if (!AudioComponent)
    {
        UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("UAtomAudioLinkComponent::CreateAudioComponent."));
        AudioComponent = NewObject<UAudioComponent>(this);

        if (!AudioComponent->GetAttachParent() && !AudioComponent->IsAttachedTo(this))
        {
            AActor* Owner = GetOwner();
            if (!Owner || !Owner->GetWorld())
            {
                if (UWorld* World = GetWorld())
                {
                    AudioComponent->RegisterComponentWithWorld(World);
                    AudioComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
                }
                else
                {
                    AudioComponent->SetupAttachment(this);
                }
            }
            else
            {
                AudioComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
                AudioComponent->RegisterComponent();
            }
        }
    }

    if (AudioComponent)
    {
        AudioComponent->bAutoActivate = false;
        AudioComponent->bStopWhenOwnerDestroyed = true;
        AudioComponent->bShouldRemainActiveIfDropped = true;
        AudioComponent->Mobility = EComponentMobility::Movable;

#if WITH_EDITORONLY_DATA
        AudioComponent->bVisualizeComponent = false;
#endif
    }
}

void UAtomAudioLinkComponent::OnRegister()
{
    Super::OnRegister();

    CreateAudioComponent();

    if (ensure(AudioComponent))
    {
        check(LinkInstance == nullptr);
        CreateLink();
    }

	if (bAutoPlay)
	{
		PlayLink(0);
	}
}

void UAtomAudioLinkComponent::OnUnregister()
{
    LinkInstance.Reset();
    AudioComponent = nullptr;

    Super::OnUnregister();
}

void UAtomAudioLinkComponent::SetLinkSound(USoundBase* InSound)
{
    Sound = InSound;

    if (AudioComponent)
    {
        AudioComponent->SetSound(InSound);
    }
}

void UAtomAudioLinkComponent::PlayLink(float StartTime)
{
    if (AudioComponent)
    {
		ResetLink();

        UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("UAtomAudioLinkComponent::PlayLink."));
        AudioComponent->SetSound(Sound);
        AudioComponent->Play(StartTime);

        //SetActiveFlag(AudioComponent->IsActive());
		//Start(); // called when producer knew format and started
    }
}

void UAtomAudioLinkComponent::StopLink()
{
    if (IsActive())
    {
        if (AudioComponent)
        {
            UE_LOG(LogAtomAudioLink, VeryVerbose, TEXT("UAtomAudioLinkComponent::StopLink."));
            AudioComponent->Stop();
        }

		//Stop(); // called when producer is endstream
        //SetActiveFlag(false);
    }
}

bool UAtomAudioLinkComponent::IsLinkPlaying() const
{
    return AudioComponent && AudioComponent->IsPlaying();
}