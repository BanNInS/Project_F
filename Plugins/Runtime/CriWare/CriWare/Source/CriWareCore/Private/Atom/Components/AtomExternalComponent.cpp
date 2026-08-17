
#include "Atom/Components/AtomExternalComponent.h"

#include "CriWareCoreSettings.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Interfaces/IAtomComponentExtension.h"

/* UAtomExternalSound implementation
 *****************************************************************************/

UAtomExternalSound::UAtomExternalSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomExternalSound::Init(UAtomExternalComponent* InExternalComponent, FAtomExternalSoundParameters& InParameters)
{
	check(InExternalComponent);

	OwningExternalComponent = InExternalComponent;
	VirtualizationMode = EAtomVirtualizationMode::Normal;
	//VirtualizationMode = EAtomVirtualizationMode::PlayWhenSilent;

	Duration = INDEFINITELY_LOOPING_DURATION;

	WaveInfo.NumChannels = InParameters.NumChannels;
	WaveInfo.SampleRate = InParameters.SampleRate;

	// move
	ExPlayer = MoveTemp(InParameters.ExPlayer);
	ExVoicePool = MoveTemp(InParameters.ExVoicePool);

	PcmFilterFunction = InParameters.PcmFilterFunction;
}

void UAtomExternalSound::PostLoad()
{
	Super::PostLoad();
}

/* UAtomExternalComponent implementation
 *****************************************************************************/

UAtomExternalComponent::UAtomExternalComponent(const FObjectInitializer& ObjectInitializer)
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

	bIsExternalPlaying = false;
	bIsInitialized = false;
	bIsUISound = false;
	bAlwaysPlay = true;
	ExternalSound = nullptr;

#if WITH_EDITORONLY_DATA
	bVisualizeComponent = false;
#endif
}

void UAtomExternalComponent::OnAtomComponentEnvelopeValue(const UAtomComponent* InAtomComponent, const UAtomSoundBase* Sound, const float EnvelopeValue)
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

void UAtomExternalComponent::BeginDestroy()
{
	Super::BeginDestroy();
	Stop();
}

void UAtomExternalComponent::Activate(bool bReset)
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

void UAtomExternalComponent::Deactivate()
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

void UAtomExternalComponent::Initialize()
{
	// This will try to create the audio component if it hasn't yet been created
	CreateAtomComponent();

	// Initialize the component 
	FAtomExternalSoundParameters ExternalSoundParams;
	bool bObtainedParams = Init(ExternalSoundParams);

	if (!ExternalSound)
	{
		ExternalSound = NewObject<UAtomExternalSound>();
	}
	check(ExternalSound);

	if (bObtainedParams)
	{
		// Copy sound base data to the sound

		//ExternalSound->SourceEffectChain = SourceEffectChain;
		ExternalSound->AtomRack = AtomRack;
		ExternalSound->AtomBusSends = AtomBusSends;
		//ExternalSound->PostEffectSourceBusSends = PostEffectSourceBusSends;
		ExternalSound->PreEffectSourceBusSends = PreEffectSourceBusSends;
		ExternalSound->bEnableBaseRack = bEnableBaseRack;
		ExternalSound->bEnableBusSends = bEnableBusSends;
		ExternalSound->bEnableSourceBusSends = bEnableSourceBusSends;

		ExternalSound->Init(this, ExternalSoundParams);
	}

	bIsInitialized = bObtainedParams;
}

UAtomComponent* UAtomExternalComponent::GetAtomComponent()
{
	return AtomComponent;
}

void UAtomExternalComponent::CreateAtomComponent()
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

		AtomComponent->OnAtomSoundSingleEnvelopeValueNative.AddUObject(this, &UAtomExternalComponent::OnAtomComponentEnvelopeValue);

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

void UAtomExternalComponent::OnRegister()
{
	CreateAtomComponent();

	Super::OnRegister();
}

void UAtomExternalComponent::OnUnregister()
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
	//SoundGenerator.Reset();
}

void UAtomExternalComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);

	if (GetOwner() && (Reason == EEndPlayReason::LevelTransition || Reason == EEndPlayReason::RemovedFromWorld || Reason == EEndPlayReason::Destroyed))
	{
		// If our world or sublevel is going away, stop immediately to prevent the containing world/level from being leaked via hard references from the audio device.
		Stop();
	}
}

UAtomSoundClass* UAtomExternalComponent::GetSoundClass()
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

bool UAtomExternalComponent::IsReadyForOwnerToAutoDestroy() const
{
	const bool bIsAtomComponentReadyForDestroy = !AtomComponent || (AtomComponent && !AtomComponent->IsPlaying());
	const bool bIsExternalSoundReadyForDestroy = !ExternalSound || !ExternalSound->IsGeneratingAudio();
	return bIsAtomComponentReadyForDestroy && bIsExternalSoundReadyForDestroy;
}

#if WITH_EDITOR
void UAtomExternalComponent::PreEditChange(FProperty* PropertyThatWillChange)
{
	Super::PreEditChange(PropertyThatWillChange);

	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomExternalComponent, AtomComponentClass))
	{
		CachedAtomComponentClass = AtomComponentClass;
	}
}

void UAtomExternalComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomExternalComponent, AtomComponentClass))
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
void UAtomExternalComponent::PostLoad()
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

void UAtomExternalComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

FAtomRuntime* UAtomExternalComponent::GetAtomRuntime() const
{
	// If the external component has a world, that means it was already registed with that world
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

void UAtomExternalComponent::Start()
{
	// Only need to start if we're not already active
	if (IsActive())
	{
		return;
	}

	// We will also ensure that this synth was initialized before attempting to play.
	Initialize();

	// If there is no external USoundBase, we can't start. This can happen if start is called in a cook, a server, or
	// if the audio engine is set to "noaudio".
	// TODO: investigate if this should be handled elsewhere before this point
	if (!ExternalSound)
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
			ExternalSound->AttenuationSettings = AttenuationSettings;
			//ExternalSound->SourceEffectChain = SourceEffectChain;
			ExternalSound->AtomRack = AtomRack;
			ExternalSound->AtomBusSends = AtomBusSends;

			// Set the audio component's sound to be our procedural sound wave
			AtomComponent->SetSound(ExternalSound);
			AtomComponent->Play(0);
			//SetActiveFlag(AtomComponent->IsActive());
			SetActiveFlag(true);
			bIsExternalPlaying = true;
		}
		else
		{
			SetActiveFlag(true);
		}
	}
}

void UAtomExternalComponent::Stop()
{
	if (IsActive())
	{
		if (AtomComponent)
		{
			AtomComponent->Stop();
		}

		if (ExternalSound)
		{
			// invalidates shared handles - for test
#ifdef ATOM_EXTERNAL_SAFE_INVALIDATE
			if (bool* PlayerBool = ExternalSound->GetExternalPlayer().GetSharedExternalObject().Get())
			{
				*PlayerBool = false;
			} 
			
			if (bool* VoiceBool = ExternalSound->GetExternalVoicePool().GetSharedExternalObject().Get())
			{
				*VoiceBool = false;
			}
#endif
			ExternalSound = nullptr;
		}

		SetActiveFlag(false);
		bIsExternalPlaying = false;
	}
}

bool UAtomExternalComponent::IsPlaying() const
{
	return AtomComponent && AtomComponent->IsPlaying();
}

void UAtomExternalComponent::SetVolumeMultiplier(float VolumeMultiplier)
{
	if (AtomComponent)
	{
		AtomComponent->SetVolumeMultiplier(VolumeMultiplier);
	}
}

void UAtomExternalComponent::SetRackSend(UAtomRackBase* Rack)
{
	if (AtomComponent)
	{
		AtomComponent->SetRackSend(Rack);
	}
}

void UAtomExternalComponent::SetAtomBusSend(UAtomBus* Bus, float SendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAtomBusSend(Bus, SendLevel);
	}
}

void UAtomExternalComponent::SetSourceBusSendPreEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetSourceBusSendPreEffect(SoundSourceBus, SourceBusSendLevel);
	}
}

/*void UAtomExternalComponent::SetSourceBusSendPostEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetSourceBusSendPostEffect(SoundSourceBus, SourceBusSendLevel);
	}
}*/

void UAtomExternalComponent::SetAudioBusSendPreEffect(UAtomAudioBus* AudioBus, float AudioBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAudioBusSendPreEffect(AudioBus, AudioBusSendLevel);
	}
}

/*void UAtomExternalComponent::SetAudioBusSendPostEffect(UAtomAudioBus* AudioBus, float AtomBusSendLevel)
{
	if (AtomComponent)
	{
		AtomComponent->SetAudioBusSendPostEffect(AudioBus, AudioBusSendLevel);
	}
}*/

void UAtomExternalComponent::SetLowPassFilterEnabled(bool InLowPassFilterEnabled)
{
	if (AtomComponent)
	{
		AtomComponent->SetLowPassFilterEnabled(InLowPassFilterEnabled);
	}
}

void UAtomExternalComponent::SetLowPassFilterFrequency(float InLowPassFilterFrequency)
{
	if (AtomComponent)
	{
		AtomComponent->SetLowPassFilterFrequency(InLowPassFilterFrequency);
	}
}

void UAtomExternalComponent::FadeIn(float FadeInDuration, float FadeVolumeLevel/* = 1.0f*/, float StartTime/* = 0.0f*/, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->FadeIn(FadeInDuration, FadeVolumeLevel, StartTime, FadeCurve);
	}
}

void UAtomExternalComponent::FadeOut(float FadeOutDuration, float FadeVolumeLevel, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->FadeOut(FadeOutDuration, FadeVolumeLevel, FadeCurve);
	}
}

void UAtomExternalComponent::AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAtomFaderCurve FadeCurve/* = EAtomFaderCurve::Linear*/) const
{
	if (AtomComponent)
	{
		AtomComponent->AdjustVolume(AdjustVolumeDuration, AdjustVolumeLevel, FadeCurve);
	}
}

void UAtomExternalComponent::SetModulationRouting(const TSet<UAtomModulatorBase*>& Modulators, const EAtomModulationDestination Destination, const EAtomModulationRouting RoutingMethod)
{
	if (AtomComponent)
	{
		AtomComponent->SetModulationRouting(Modulators, Destination, RoutingMethod);
	}
}

TSet<UAtomModulatorBase*> UAtomExternalComponent::GetModulators(const EAtomModulationDestination Destination)
{
	if (AtomComponent)
	{
		return AtomComponent->GetModulators(Destination);
	}

	return TSet<UAtomModulatorBase*>();
}

void UAtomExternalComponent::SetMultiplePositions(TArray<FTransform> Positions, EAtomMultiPositionType MultiPositionType)
{
	if (AtomComponent)
	{
		AtomComponent->SetMultiplePositions(Positions, MultiPositionType);
	}
}

void UAtomExternalComponent::SetAisacControlValue(const FAtomAisacControl& AisacControl, float Value)
{
	if (AtomComponent)
	{
		AtomComponent->SetAisacControlValue(AisacControl, Value);
	}
}
