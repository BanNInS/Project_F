
#include "Atom/Components/AtomSynthComponent.h"

#include "CriWareLLM.h"
#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomSynthComponent)

/* UAtomSynthComponent implementation
 *****************************************************************************/

UAtomSynthSound::UAtomSynthSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomSynthSound::Init(UAtomSynthComponent* InSynthComponent, const int32 InNumChannels, const int32 InSampleRate, const int32 InCallbackSize)
{
	check(InSynthComponent);

	OwningSynthComponent = InSynthComponent;
	VirtualizationMode = EAtomVirtualizationMode::Normal;
	//VirtualizationMode = EAtomVirtualizationMode::PlayWhenSilent; // not supported by atom yet

	Duration = INDEFINITELY_LOOPING_DURATION;
	
	WaveInfo.NumChannels = InNumChannels;
	WaveInfo.SampleRate = InSampleRate;
	WaveInfo.NumFrames = InCallbackSize / InNumChannels;
	WaveInfo.bIsStreamed = false;
	WaveInfo.Format = EAtomFormat::AudioBuffer;

	NumSamplesToGeneratePerCallback = InCallbackSize;
	//bCanProcessAsync = true;
}

 IAtomSoundGeneratorPtr UAtomSynthSound::CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams)
 {
	 if (OwningSynthComponent.IsValid())
	 {
		 return OwningSynthComponent->CreateSoundGeneratorInternal(InParams);
	 }
	 return nullptr;
 }

/* UAtomSynthComponent implementation
 *****************************************************************************/

 UAtomSynthComponent::UAtomSynthComponent(const FObjectInitializer& ObjectInitializer)
	 : Super(ObjectInitializer)
	 , AtomComponentClass(UAtomComponent::StaticClass())
	 , bApplyExtensionSettings(false)
	 , ExtensionSettings(nullptr)
{
	bAutoActivate = false;

	bStopWhenOwnerDestroyed = true;

	bEnableSourceBusSends = true;
	bEnableBaseRack = true;
	bEnableBusSends = true;

	bNeverNeedsRenderUpdate = true;
	bUseAttachParentBound = true; // Avoid CalcBounds() when transform changes.

	bIsSynthPlaying = false;
	bIsInitialized = false;
	bIsUISound = false;
	bAlwaysPlay = true;
	SynthSound = nullptr;

#if WITH_EDITORONLY_DATA
	bVisualizeComponent = false;
#endif
}

 void UAtomSynthComponent::OnAtomComponentEnvelopeValue(const UAtomComponent* InAtomComponent, const UAtomSoundBase* Sound, const float EnvelopeValue)
 {
	 if (OnAtomSoundEnvelopeValue.IsBound())
	 {
		 OnAtomSoundEnvelopeValue.Broadcast(EnvelopeValue);
	 }

	 if (OnAtomSoundEnvelopeValueNative.IsBound())
	 {
		 OnAtomSoundEnvelopeValueNative.Broadcast(InAtomComponent, EnvelopeValue);
	 }
 }

void UAtomSynthComponent::BeginDestroy()
{
	Super::BeginDestroy();
	Stop();
}

void UAtomSynthComponent::Activate(bool bReset)
{
	if (bReset || ShouldActivate())
	{
		Start();
		if (IsActive())
		{
			OnComponentActivated.Broadcast(this, bReset);
		}
	}
}

void UAtomSynthComponent::Deactivate()
{
	if (ShouldActivate() == false)
	{
		Stop();

		if (!IsActive())
		{
			OnComponentDeactivated.Broadcast(this);
		}
	}
}

void UAtomSynthComponent::Initialize(int32 SampleRateOverride /* = -1 */)
{
	// This will try to create the audio component if it hasn't yet been created
	CreateAtomComponent();

	// Initialize the component
	
	//FAtomSynthSoundParameters SynthSoundParams;
	//bool bObtainedParams = Init(SynthSoundParams);
	int32 SampleRate = SampleRateOverride;
	if (SampleRate == INDEX_NONE)
	{
		SampleRate = 48000;
	}

	bool bInitialized = Init(SampleRate);

	if (NumChannels < 0 || NumChannels > ATOM_MAX_DSP_CHANNELS)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Synthesis component '%s' has set an invalid channel count '%d'."), *GetName(), NumChannels);
	}

	NumChannels = FMath::Clamp(NumChannels, ATOM_MIN_DSP_CHANNELS, ATOM_MAX_DSP_CHANNELS);

	if (!SynthSound)
	{
		SynthSound = NewObject<UAtomSynthSound>();
	}
	check(SynthSound);

	if (bInitialized)
	{
		// Copy sound base data to the sound

		//SynthSound->SourceEffectChain = SourceEffectChain;
		SynthSound->AtomRack = AtomRack;
		SynthSound->AtomBusSends = AtomBusSends;
		//SynthSound->PosteEffectSourceBusSends = PosteEffectSourceBusSends;
		SynthSound->PreEffectSourceBusSends = PreEffectSourceBusSends;
		SynthSound->bEnableBaseRack = bEnableBaseRack;
		SynthSound->bEnableBusSends = bEnableBusSends;
		SynthSound->bEnableSourceBusSends = bEnableSourceBusSends;

		//SynthSound->Init(this, SynthSoundParams);
		SynthSound->Init(this, NumChannels, SampleRate, PreferredBufferLength);
	}

	bIsInitialized = bInitialized;
}

UAtomComponent* UAtomSynthComponent::GetAtomComponent()
{
	return AtomComponent;
}

void UAtomSynthComponent::CreateAtomComponent()
{
	if (!AtomComponentClass)
	{
		AtomComponentClass = UAtomComponent::StaticClass();
	}

	if (!AtomComponent)
	{
		// Create the audio component which will be used to play the procedural sound wave
		AtomComponent = NewObject<UAtomComponent>(this, AtomComponentClass, NAME_None, RF_Transactional | RF_Transient | RF_TextExportTransient);
		AtomComponent->CreationMethod = CreationMethod;

		AtomComponent->OnAtomSoundSingleEnvelopeValueNative.AddUObject(this, &UAtomSynthComponent::OnAtomComponentEnvelopeValue);

		if (!AtomComponent->GetAttachParent() && !AtomComponent->IsAttachedTo(this))
		{
			AActor* Owner = GetOwner();

			// If the media component has no owner or the owner doesn't have a world
			if (!Owner || !Owner->GetWorld())
			{
				// Attempt to retrieve the synth component's world and register the audio component with it
				// This ensures that the synth component plays on the correct world in cases where there isn't an owner
				if (UWorld* World = GetWorld())
				{
					AtomComponent->RegisterComponentWithWorld(World);
					AtomComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				}
				else
				{
					AtomComponent->SetupAttachment(this);
				}
			}
			else
			{
				AtomComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
				AtomComponent->RegisterComponent();
			}
		}
	}

	if (AtomComponent)
	{
		AtomComponent->bAutoActivate = false;
		AtomComponent->bStopWhenOwnerDestroyed = true;
		AtomComponent->bShouldRemainActiveIfDropped = true;
		AtomComponent->Mobility = EComponentMobility::Movable;

#if WITH_EDITORONLY_DATA
		AtomComponent->bVisualizeComponent = false;
#endif

		// Set defaults to be the same as audio component defaults
		AtomComponent->EnvelopeFollowerAttackTime = EnvelopeFollowerAttackTime;
		AtomComponent->EnvelopeFollowerReleaseTime = EnvelopeFollowerReleaseTime;
		AtomComponent->bAlwaysPlay = bAlwaysPlay;
	}
}

void UAtomSynthComponent::OnRegister()
{
	CreateAtomComponent();

	Super::OnRegister();
}

void UAtomSynthComponent::OnUnregister()
{
	// Route OnUnregister event.
	Super::OnUnregister();

	// Don't stop audio and clean up component if owner has been destroyed (default behaviour). This function gets
	// called from AActor::ClearComponents when an actor gets destroyed which is not usually what we want for one-
	// shot sounds.
	AActor* Owner = GetOwner();
	if (!Owner || bStopWhenOwnerDestroyed)
	{
		Stop();
	}

	// Make sure the audio component is destroyed during unregister
	if (AtomComponent && !AtomComponent->IsBeingDestroyed())
	{
		if (Owner && Owner->GetWorld())
		{
			AtomComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
			AtomComponent->UnregisterComponent();
		}
		AtomComponent->DestroyComponent();
		AtomComponent = nullptr;
	}

	// Clear out the synth component's reference to the sound generator or it will leak until it gets GC'd
	// Normally this is ok to wait till GC but some derived synths might need for the handle to be released
	SoundGenerator.Reset();
}

void UAtomSynthComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	if (GetOwner() && (Reason == EEndPlayReason::LevelTransition || Reason == EEndPlayReason::RemovedFromWorld || Reason == EEndPlayReason::Destroyed))
	{
		// If our world or sublevel is going away, stop immediately to prevent the containing world/level from being leaked via hard references from the audio device.
		Stop();
	}
}

UAtomSoundClass* UAtomSynthComponent::GetSoundClass()
{
	if (SoundClass)
	{
		return SoundClass;
	}

	const UCriWareCoreSettings* AtomSettings = GetDefault<UCriWareCoreSettings>();
	if (ensure(AtomSettings))
	{
		return AtomSettings->GetDefaultSoundClass();
	}

	return nullptr;
}

bool UAtomSynthComponent::IsReadyForOwnerToAutoDestroy() const
{
	const bool bIsAtomComponentReadyForDestroy = !AtomComponent || (AtomComponent && !AtomComponent->IsPlaying());
	const bool bIsSynthSoundReadyForDestroy = !SynthSound || !SynthSound->IsGeneratingAudio();
	return bIsAtomComponentReadyForDestroy && bIsSynthSoundReadyForDestroy;
}

#if WITH_EDITOR
void UAtomSynthComponent::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomSynthComponent, AtomComponentClass))
	{
		CachedAtomComponentClass = AtomComponentClass;
	}
}

void UAtomSynthComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (IsActive())
	{
		// If this is an auto destroy component we need to prevent it from being auto-destroyed since we're really just restarting it
		const bool bWasAutoDestroy = bAutoDestroy;
		bAutoDestroy = false;
		Stop();
		bAutoDestroy = bWasAutoDestroy;
		Start();
	}

	// crate the extension settings object if used by the component class
	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomSynthComponent, AtomComponentClass))
	{
		if (CachedAtomComponentClass != AtomComponentClass)
		{
			auto ExtensionClass = UAtomComponentExtensionSettings::FindExtensionSettingsClassForAtomComponent(AtomComponentClass);
			ExtensionSettings = ExtensionClass ? NewObject<UAtomComponentExtensionSettings>(this, ExtensionClass, NAME_None, RF_Public) : nullptr;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif //WITH_EDITOR

#if WITH_EDITORONLY_DATA
void UAtomSynthComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (AtomComponentClass && !ExtensionSettings)
	{
		auto ExtensionClass = UAtomComponentExtensionSettings::FindExtensionSettingsClassForAtomComponent(AtomComponentClass);
		ExtensionSettings = ExtensionClass ? NewObject<UAtomComponentExtensionSettings>(this, ExtensionClass, NAME_None, RF_Public) : nullptr;
	}
#endif
}
#endif //WITH_EDITORONLY_DATA

void UAtomSynthComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

FAtomRuntime* UAtomSynthComponent::GetAtomRuntime() const
{
	// If the Synth component has a world, that means it was already registed with that world
	if (UWorld* World = GetWorld())
	{
		return FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(World);
	}

	// Otherwise, retrieve the Atom component's runtime (probably from it's owner)
	if (AtomComponent)
	{
		return AtomComponent->GetAtomRuntime();
	}

	// No Atom runtime
	return nullptr;
}

void UAtomSynthComponent::Start()
{
	// Only need to start if we're not already active
	if (IsActive())
	{
		return;
	}

	// We will also ensure that this synth was initialized before attempting to play.
	Initialize();

	// If there is no Synth USoundBase, we can't start. This can happen if start is called in a cook, a server, or
	// if the audio engine is set to "noaudio".
	// TODO: investigate if this should be handled elsewhere before this point
	if (!SynthSound)
	{
		return;
	}

	if (AtomComponent)
	{
		if (bIsInitialized)
		{
			// Copy the attenuation and concurrency data from the synth component to the audio component
			AtomComponent->AttenuationSettings = AttenuationSettings;
			AtomComponent->bOverrideAttenuation = bOverrideAttenuation;
			AtomComponent->bIsUISound = bIsUISound;
			AtomComponent->bIsPreviewSound = bIsPreviewSound;
			AtomComponent->bAllowSpatialization = bAllowSpatialization;
			AtomComponent->ConcurrencySet = ConcurrencySet;
			AtomComponent->AttenuationOverrides = AttenuationOverrides;
			AtomComponent->SoundClassOverride = GetSoundClass();
			AtomComponent->EnvelopeFollowerAttackTime = EnvelopeFollowerAttackTime;
			AtomComponent->EnvelopeFollowerReleaseTime = EnvelopeFollowerReleaseTime;
			AtomComponent->ModulationRouting = ModulationRouting;
			// Apply extension settings for AtomComponent if available.
			if (bApplyExtensionSettings)
			{
				if (auto AtomComponentAsExtension = Cast<IAtomComponentExtension>(AtomComponent))
				{
					AtomComponentAsExtension->ApplyExtensionSettings(ExtensionSettings);
				}
			}

			// Copy sound base data to the sound
			SynthSound->AttenuationSettings = AttenuationSettings;
			//SynthSound->SourceEffectChain = SourceEffectChain;
			SynthSound->AtomRack = AtomRack;
			SynthSound->AtomBusSends = AtomBusSends;

			// Set the audio component's sound to be our procedural sound wave
			AtomComponent->SetSound(SynthSound);
			AtomComponent->Play(0);
			
			//SetActiveFlag(AtomComponent->IsActive());
			
			SetActiveFlag(true);
			bIsSynthPlaying = true;
		}
		else
		{
			SetActiveFlag(true);
		}
	}
}

void UAtomSynthComponent::Stop()
{
	if (IsActive())
	{
		if (AtomComponent)
		{
			AtomComponent->Stop();
		}

		SetActiveFlag(false);
		bIsSynthPlaying = false;
	}
}

bool UAtomSynthComponent::IsPlaying() const
{
	return AtomComponent && AtomComponent->IsPlaying();
}

void UAtomSynthComponent::SetVolumeMultiplier(float VolumeMultiplier)
{
	if (AtomComponent)
	{
		AtomComponent->SetVolumeMultiplier(VolumeMultiplier);
	}
}

void UAtomSynthComponent::SetRackSend(UAtomRackBase* Rack)
{
	if (AtomComponent)
	{
		AtomComponent->SetRackSend(Rack);
	}
}

void UAtomSynthComponent::SetAtomBusSend(UAtomBus* Bus, float SendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAtomBusSend(Bus, SendLevel);
	}
}

void UAtomSynthComponent::SetSourceBusSendPreEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetSourceBusSendPreEffect(SoundSourceBus, SourceBusSendLevel);
	}
}

/*void UAtomSynthComponent::SetSourceBusSendPostEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetSourceBusSendPostEffect(SoundSourceBus, SourceBusSendLevel);
	}
}*/

void UAtomSynthComponent::SetAudioBusSendPreEffect(UAtomAudioBus* AudioBus, float AudioBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAudioBusSendPreEffect(AudioBus, AudioBusSendLevel);
	}
}

/*void UAtomSynthComponent::SetAtomBusSendPostEffect(UAtomAudioBus* AudioBus, float AudioBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAudioBusSendPostEffect(AudioBus, AudioBusSendLevel);
	}
}*/

void UAtomSynthComponent::SetLowPassFilterEnabled(bool InLowPassFilterEnabled)
{
	if (AtomComponent)
	{
		AtomComponent->SetLowPassFilterEnabled(InLowPassFilterEnabled);
	}
}

void UAtomSynthComponent::SetLowPassFilterFrequency(float InLowPassFilterFrequency)
{
	if (AtomComponent)
	{
		AtomComponent->SetLowPassFilterFrequency(InLowPassFilterFrequency);
	}
}

void UAtomSynthComponent::FadeIn(float FadeInDuration, float FadeVolumeLevel/* = 1.0f*/, float StartTime/* = 0.0f*/, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->FadeIn(FadeInDuration, FadeVolumeLevel, StartTime, FadeCurve);
	}
}

void UAtomSynthComponent::FadeOut(float FadeOutDuration, float FadeVolumeLevel, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->FadeOut(FadeOutDuration, FadeVolumeLevel, FadeCurve);
	}
}

void UAtomSynthComponent::AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->AdjustVolume(AdjustVolumeDuration, AdjustVolumeLevel, FadeCurve);
	}
}

void UAtomSynthComponent::SetModulationRouting(const TSet<UAtomModulatorBase*>& Modulators, const EAtomModulationDestination Destination, const EAtomModulationRouting RoutingMethod)
{
	if (AtomComponent)
	{
		AtomComponent->SetModulationRouting(Modulators, Destination, RoutingMethod);
	}
}

TSet<UAtomModulatorBase*> UAtomSynthComponent::GetModulators(const EAtomModulationDestination Destination)
{
	if (AtomComponent)
	{
		return AtomComponent->GetModulators(Destination);
	}

	return TSet<UAtomModulatorBase*>();
}

void UAtomSynthComponent::SetMultiplePositions(TArray<FTransform> Positions, EAtomMultiPositionType MultiPositionType)
{
	if (AtomComponent)
	{
		AtomComponent->SetMultiplePositions(Positions, MultiPositionType);
	}
}

void UAtomSynthComponent::SetAisacControlValue(const FAtomAisacControl& AisacControl, float Value)
{
	if (AtomComponent)
	{
		AtomComponent->SetAisacControlValue(AisacControl, Value);
	}
}

IAtomSoundGeneratorPtr UAtomSynthComponent::CreateSoundGeneratorInternal(const FAtomSoundGeneratorInitParams& InParams)
{
	LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomAudioSynthesis);
	return SoundGenerator = CreateSoundGenerator(InParams);
}
