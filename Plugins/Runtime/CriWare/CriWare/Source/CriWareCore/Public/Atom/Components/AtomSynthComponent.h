#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"

#include "Atom/Interfaces/IAtomComponentExtension.h"
#include "Atom/AtomSoundWaveProcedural.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundGenerator.h"

#include "CriWareApi.h"

#include "AtomSynthComponent.generated.h"

// Forward Definitions
class UAtomComponentExtension;
class UAtomSoundSourceBus;

/**
* Called by a synth component and returns the sound's envelope value (using an envelope follower in the audio renderer).
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAtomSynthEnvelopeValue, const float, EnvelopeValue);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAtomSynthEnvelopeValueNative, const class UAtomComponent*, const float);

UCLASS(MinimalAPI)
class UAtomSynthSound
	: public UAtomSoundWaveProcedural
{
	GENERATED_BODY()

public:

	UAtomSynthSound(const FObjectInitializer& ObjectInitializer);

	CRIWARECORE_API void Init(UAtomSynthComponent* InSynthComponent, const int32 InNumChannels, const int32 SampleRate, const int32 InCallbackSize);

	/** Begin UAtomSoundWaveProcedural */
	CRIWARECORE_API virtual IAtomSoundGeneratorPtr CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams) override;
	/** End UAtomSoundWaveProcedural */

protected:

	UPROPERTY()
	TWeakObjectPtr<UAtomSynthComponent> OwningSynthComponent = nullptr;

	TArray<float> FloatBuffer;

public:

	UAtomSynthComponent* GetOwningSynthComponent()
	{
		return OwningSynthComponent.Get();
	}

	TWeakObjectPtr<UAtomSynthComponent>& GetOwningSynthComponentPtr()
	{
		return OwningSynthComponent;
	}
};

/**
 * An Atom compomennet that use externally created player and voices.
 * 
 * Used for source access to sound created externally by CriWare libraries using their own AtomVoicePools and AtomExPlayers.
 */
UCLASS(abstract, ClassGroup = AtomSynth, HideCategories = (Object, ActorComponent, Physics, Rendering, Mobility, LOD), MinimalAPI)
class UAtomSynthComponent
	: public USceneComponent
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomSynthComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin USceneComponent Interface
	CRIWARECORE_API virtual void Activate(bool bReset = false) override;
	CRIWARECORE_API virtual void Deactivate() override;
	//~ End USceneComponent Interface

	//~ Begin ActorComponent Interface.
	CRIWARECORE_API virtual void OnRegister() override;
	CRIWARECORE_API virtual void OnUnregister() override;
	CRIWARECORE_API virtual bool IsReadyForOwnerToAutoDestroy() const override;
	CRIWARECORE_API virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	//~ End ActorComponent Interface.

		//~ Begin UObject Interface.
#if WITH_EDITOR
	CRIWARECORE_API virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	CRIWARECORE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
	CRIWARECORE_API virtual void PostLoad() override;
#endif //WITH_EDITORONLY_DATA

	CRIWARECORE_API virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	// Starts the external audio.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Sound")
	CRIWARECORE_API void Start();

	// Stops the external audio.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Sound")
	CRIWARECORE_API void Stop();

	/** Returns true if this component is currently playing. */
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Sound")
	CRIWARECORE_API bool IsPlaying() const;

	/** Set a new volume multiplier */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetVolumeMultiplier(float VolumeMultiplier);

	/** Sets sound send to the given Rack. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetRackSend(UAtomRackBase* Rack);

	/** Sets how much audio the sound should send to the given AudioBus (pre effect). */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetAtomBusSend(UAtomBus* Bus, float SendLevel);

	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound", meta = (DeprecatedFunction, DeprecationMessage = "Use the SetAtomBusSend function instead."))
	CRIWARECORE_API void SetAtomBusSendPreEffect(UAtomBus* AtomBus, float AtomBusSendLevel) { SetAtomBusSend(AtomBus, AtomBusSendLevel); }

	/** Sets how much audio the sound should send to the given SourceBus (pre effect). */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetSourceBusSendPreEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel);

	/** Sets how much audio the sound should send to the given SourceBus (post effect). */
	//UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	//CRIWARECORE_API void SetSourceBusSendPostEffect(UAtomSoundSourceBus* SoundSourceBus, float SourceBusSendLevel);

	/** Sets how much audio the sound should send to the given AudioBus (pre effect). */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetAudioBusSendPreEffect(UAtomAudioBus* AudioBus, float AudioBusSendLevel);

	/** Sets how much audio the sound should send to the given AudioBus (post effect). */
	//UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	//CRIWARECORE_API void SetAudioBusSendPostEffect(UAtomAudioBus* AudioBus, float AudioBusSendLevel);

	/** Sets whether or not the low pass filter is enabled on the audio component. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetLowPassFilterEnabled(bool InLowPassFilterEnabled);

	/** Sets lowpass filter frequency of the audio component. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API virtual void SetLowPassFilterFrequency(float InLowPassFilterFrequency);

	/**
	 * This function allows designers to call Play on an Audio Component instance while applying a volume curve over time.
	 * Parameters allow designers to indicate the duration of the fade, the curve shape, and the start time if seeking into the sound.
	 *
	 * @param FadeInDuration How long it should take to reach the FadeVolumeLevel
	 * @param FadeVolumeLevel The percentage of the AudioComponents's calculated volume to fade to
	 * @param FadeCurve The curve to use when interpolating between the old and new volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void FadeIn(float FadeInDuration, float FadeVolumeLevel = 1.0f, float StartTime = 0.0f, const EAtomFaderCurve FadeCurve = EAtomFaderCurve::Linear) const;

	/**
	 * This function allows designers to call a delayed Stop on an Audio Component instance while applying a
	 * volume curve over time. Parameters allow designers to indicate the duration of the fade and the curve shape.
	 *
	 * @param FadeOutDuration how long it should take to reach the FadeVolumeLevel
	 * @param FadeVolumeLevel the percentage of the AudioComponents's calculated volume in which to fade to
	 * @param FadeCurve The curve to use when interpolating between the old and new volume
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void FadeOut(float FadeOutDuration, float FadeVolumeLevel, const EAtomFaderCurve FadeCurve = EAtomFaderCurve::Linear) const;

	/** This function allows designers to trigger an adjustment to the sound instance's playback Volume with options for smoothly applying a curve over time.
	 * @param AdjustVolumeDuration The length of time in which to interpolate between the initial volume and the new volume.
	 * @param AdjustVolumeLevel The new volume to set the Audio Component to.
	 * @param FadeCurve The curve used when interpolating between the old and new volume.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void AdjustVolume(float AdjustVolumeDuration, float AdjustVolumeLevel, const EAtomFaderCurve FadeCurve = EAtomFaderCurve::Linear) const;

	/**
	 * Sets the routing for one of the given Synth component's Modulation Destinations.
	 * @param Modulators The set of modulators to apply to the given destination on the component.
	 * @param Destination The destination to assign the modulators to.
	 * @param RoutingMethod The routing method to use for the given modulator.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound", DisplayName = "Set Modulation Routing")
	CRIWARECORE_API void SetModulationRouting(const TSet<UAtomModulatorBase*>& Modulators, const EAtomModulationDestination Destination, const EAtomModulationRouting RoutingMethod = EAtomModulationRouting::Inherit);

	/**
	 * Gets the set of currently active modulators for a given Modulation Destination.
	 * @param Destination The Destination to retrieve the Modulators from.
	 * @return The set of of Modulators applied to this component for the given Destination.
	 */
	UFUNCTION(BlueprintPure, Category = "Atom|Components|Sound", DisplayName = "Get Modulators")
	CRIWARECORE_API UPARAM(DisplayName = "Modulators") TSet<UAtomModulatorBase*> GetModulators(const EAtomModulationDestination Destination);

	/**
	 * Sets multiple source attenuation positions if spatialization is enabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetMultiplePositions(TArray<FTransform> Positions, EAtomMultiPositionType MultiPositionType);

	/** Modulates in realtime the desired AISAC control value.
	 * This multiply with the base control value and with any routed modulation.
	 * @param AisacControl The AISAC control to modulate.
	 * @param Value The value used to modulate the base value.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components|Sound")
	CRIWARECORE_API void SetAisacControlValue(const FAtomAisacControl& AisacControl, float Value);

	/** Gets an interface that exposes extended functionnalities of the owned AtomComponent. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Components")
	TScriptInterface<IAtomComponentExtension> GetAtomComponentExtension() const { return TScriptInterface<IAtomComponentExtension>(AtomComponent); }

	/** Auto destroy this component on completion */
	UPROPERTY()
	uint8 bAutoDestroy : 1;

	/** Stop sound when owner is destroyed */
	UPROPERTY()
	uint8 bStopWhenOwnerDestroyed : 1;

	/** Is this audio component allowed to be spatialized? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	uint8 bAllowSpatialization : 1;

	/** Should the Attenuation Settings asset be used (false) or should the properties set directly on the component be used for attenuation properties */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation)
	uint8 bOverrideAttenuation : 1;

	/** Whether or not to enable sending this audio's output to buses.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	uint32 bEnableSourceBusSends : 1;

	/** If enabled, sound will route to the Master Rack by default or to the Base Rack if defined. If disabled, sound will route ONLY to the Bus Sends */
	UPROPERTY(EditAnywhere, Category = "Effects|Rack")
	uint32 bEnableBaseRack : 1;

	/** Whether or not to enable Bus Sends other than the Base Rack.*/
	UPROPERTY(EditAnywhere, Category = "Effects|Rack", meta = (DisplayAfter = "AtomRack"))
	uint32 bEnableBusSends : 1;

	/** If bOverrideSettings is false, the asset to use to determine attenuation properties for sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation, meta = (EditCondition = "!bOverrideAttenuation"))
	TObjectPtr<class UAtomAttenuation> AttenuationSettings;

	/** If bOverrideSettings is true, the attenuation properties to use for sounds generated by this component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attenuation, meta = (EditCondition = "bOverrideAttenuation"))
	struct FAtomAttenuationSettings AttenuationOverrides;

	/** What sound concurrency to use for sounds generated by this Atom component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Concurrency)
	TSet<TObjectPtr<UAtomConcurrency>> ConcurrencySet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	FAtomSoundModulationRoutingSettings ModulationRouting;

	/** Sound class this sound belongs to */
	UPROPERTY(EditAnywhere, Category = SoundClass)
	TObjectPtr<UAtomSoundClass> SoundClass;

	/** Rack this sound belongs to */
	UPROPERTY(EditAnywhere, Category = "Effects|Rack", meta = (EditCondition = "bEnableBaseRack", DisplayName = "Base Rack"))
	TObjectPtr<UAtomRackWithParentBase> AtomRack;

	/** An array of submix sends. Audio from this sound will send a portion of its audio to these effects.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects|Rack", meta = (EditCondition = "bEnableBusSends", DisplayName = "Bus Sends"))
	TArray<FAtomSoundToBusSend> AtomBusSends;

	/** The source effect chain to use for this sound. */
	//UPROPERTY(EditAnywhere, Category = Effects)
	//TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain;

	/** This sound will send its audio output to this list of buses if there are bus instances playing after source effects are processed.  */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (DisplayName = "Sound Post-Effect Bus Sends", EditCondition = "bEnableSourceBusSends"))
	//TArray<FSoundSourceBusSendInfo> PostEffectSourceBusSends;

	/** This sound will send its audio output to this list of buses if there are bus instances playing before source effects are processed.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects, meta = (DisplayName = "Sound Pre-Effect Bus Sends", EditCondition = "bEnableSourceBusSends"))
	TArray<FAtomSoundSourceBusSendInfo> PreEffectSourceBusSends;

	/** The AtomComponent sub-class to use (MultiPositions, Lips...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	TSubclassOf<UAtomComponent> AtomComponentClass;

	/** Whether to use the Atom component extension settings if available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (EditCondition = "ExtensionSettings != nullptr"))
	bool bApplyExtensionSettings;

	/** Whether or not this sound plays when the game is paused in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	uint8 bIsUISound : 1;

	/** Whether or not this synth is playing as a preview sound */
	UPROPERTY()
	uint8 bIsPreviewSound : 1;

	/** Whether to artificially prioritize the component to play */
	uint8 bAlwaysPlay : 1;

	/** Call for creating this external component from code or some other location. */
	CRIWARECORE_API void Initialize(int32 SampleRateOverride = -1);

	/** Creates the Atom component if it hasn't already been created yet. This should only be used when trying to
	 *  assign explicit settings to the AtomComponent before calling Start().
	 */
	CRIWARECORE_API void CreateAtomComponent();

	/** Retrieves this synth component's Atom component. */
	CRIWARECORE_API UAtomComponent* GetAtomComponent();

	/** The attack time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the
	 *  envelope value of sounds played with this audio component. Only used in audio mixer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerAttackTime;

	/** The release time in milliseconds for the envelope follower. Delegate callbacks can be registered to get the
	 *  envelope value of sounds played with this audio component. Only used in audio mixer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound, meta = (ClampMin = "0", UIMin = "0"))
	int32 EnvelopeFollowerReleaseTime;

	UPROPERTY(BlueprintAssignable)
	FOnAtomSynthEnvelopeValue OnAtomSoundEnvelopeValue;

	/** Shadow delegate for non UObject subscribers */
	FOnAtomSynthEnvelopeValueNative OnAtomSoundEnvelopeValueNative;

	CRIWARECORE_API void OnAtomComponentEnvelopeValue(const UAtomComponent* AtomComponent, const UAtomSoundBase* Sound, const float EnvelopeValue);

	// Adds and removes audio buffer listener
	//AUDIOMIXER_API void AddAudioBufferListener(IAudioBufferListener* InAudioBufferListener);
	//AUDIOMIXER_API void RemoveAudioBufferListener(IAudioBufferListener* InAudioBufferListener);

	CRIWARECORE_API virtual UAtomSoundClass* GetSoundClass();

	CRIWARECORE_API virtual void BeginDestroy() override;

protected:

	// Called when synth is created.
	virtual bool Init(int32& SampleRate) { return true; }

	// Implemented by the synth component to create a generator object.
	// This method prevents UObjects from having to exist in the Atom SDK audio render thread.
	virtual IAtomSoundGeneratorPtr CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams) { return nullptr; }
	virtual void OnEndGenerate(IAtomSoundGeneratorPtr Generator) {};

	// Gets the Atom runtime associated with this synth component
	CRIWARECORE_API FAtomRuntime* GetAtomRuntime() const;

	// Can be set by the derived class, defaults to 2
	int32 NumChannels;

	// Can be set by the derived class- sets the preferred callback size for the synth component.
	int32 PreferredBufferLength;

private:

	// Creates the synth component's sound generator, calls into overridden client code to create the instance.
	CRIWARECORE_API IAtomSoundGeneratorPtr CreateSoundGeneratorInternal(const FAtomSoundGeneratorInitParams& InParams);

	UPROPERTY(Transient)
	TObjectPtr<UAtomSynthSound> SynthSound;

	UPROPERTY(Transient)
	TObjectPtr<UAtomComponent> AtomComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extension Settings", meta = (EditCondition = "ExtensionSettings != nullptr && bApplyExtensionSettings", EditConditionHides, ShowInnerProperties, AllowPrivateAccess = "true"))
	TObjectPtr<UAtomComponentExtensionSettings> ExtensionSettings;

#if WITH_EDITOR
	TSubclassOf<UAtomComponent> CachedAtomComponentClass;
#endif
	// Whether or not synth is playing
	bool bIsSynthPlaying;
	bool bIsInitialized;

	// Synth component's handle to its sound generator instance.
	// used to forward BP functions to the instance directly.
	IAtomSoundGeneratorPtr SoundGenerator;

	friend class UAtomSynthSound;
};