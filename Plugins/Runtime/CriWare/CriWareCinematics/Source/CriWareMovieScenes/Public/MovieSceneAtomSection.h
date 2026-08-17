/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareMovieScene
 * File     : MovieSceneAtomSection.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EntitySystem/IMovieSceneEntityProvider.h"
#include "MovieSceneSection.h"
#include "Channels/MovieSceneBoolChannel.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "Channels/MovieSceneStringChannel.h"
#include "Sections/MovieSceneActorReferenceSection.h"
#include "Sections/MovieSceneParameterSection.h"

#include "Atom/AtomComponent.h"
#include "Atom/AtomAttenuation.h"

#include "MovieSceneAtomSection.generated.h"

// Forward Definitions
class UAtomSoundBase;
class UAtomRack;
class UAtomBus;
class UAtomComponentExtensionSettings;

/**
 * Structure representing an animated aisac control parameter and it's associated animation curve.
 */
USTRUCT()
struct FAisacControlAndCurve
{
	GENERATED_BODY()

	FAisacControlAndCurve()
	{}

	/** Creates a new FScalarParameterNameAndCurve for a specific scalar parameter. */
	CRIWAREMOVIESCENES_API FAisacControlAndCurve(const FAtomAisacControl& InAisacControl);

	/** The AISAC control which is being animated. */
	UPROPERTY()
	FAtomAisacControl AisacControl;

	/** The curve which contains the animation data for the AISAC control value. */
	UPROPERTY()
	FMovieSceneFloatChannel AisacValueCurve;
};

/**
 * Atom section, for use in the master audio, or by attached audio objects
 */
UCLASS(MinimalAPI)
class UMovieSceneAtomSection
	: public UMovieSceneSection
	, public IMovieSceneEntityProvider
{
	GENERATED_BODY()

public:

	CRIWAREMOVIESCENES_API UMovieSceneAtomSection(const FObjectInitializer& ObjectInitializer);

	/** Sets this section's sound */
	UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	CRIWAREMOVIESCENES_API void SetSound(class UAtomSoundBase* InSound);

	/** Gets the sound for this section */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	class UAtomSoundBase* GetSound() const { return Sound; }

	/** Set the offset into the beginning of the audio clip */
	UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	void SetStartOffset(FFrameNumber InStartOffset) { StartFrameOffset = InStartOffset;}

	/** Get the offset into the beginning of the audio clip */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	FFrameNumber GetStartOffset() const {return StartFrameOffset;}

	/**
	 * Gets the sound volume curve
	 *
	 * @return The rich curve for this sound volume
	 */
	const FMovieSceneFloatChannel& GetSoundVolumeChannel() const { return SoundVolume; }

	/**
	 * Gets the sound pitch curve
	 *
	 * @return The rich curve for this sound pitch
	 */
	const FMovieSceneFloatChannel& GetPitchMultiplierChannel() const { return PitchMultiplier; }

	/**
	 * Return the sound volume
	 *
	 * @param InTime	The position in time within the movie scene
	 * @return The volume the sound will be played with.
	 */
	float GetSoundVolume(FFrameTime InTime) const
	{
		float OutValue = 0.f;
		SoundVolume.Evaluate(InTime, OutValue);
		return OutValue;
	}

	/**
	 * Return the pitch multiplier
	 *
	 * @param Position	The position in time within the movie scene
	 * @return The pitch multiplier the sound will be played with.
	 */
	float GetPitchMultiplier(FFrameTime InTime) const
	{
		float OutValue = 0.f;
		PitchMultiplier.Evaluate(InTime, OutValue);
		return OutValue;
	}

	/**
	 * @return Whether to allow looping if the section length is greater than the sound duration
	 */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	bool GetLooping() const { return bLooping; }

	UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	void SetLooping(bool bInLooping) { bLooping = bInLooping; }

	/**
	 * @return Whether subtitles should be suppressed
	 */
	//UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	//bool GetSuppressSubtitles() const { return bSuppressSubtitles; }

	/** Set whether subtitles should be suppressed */
	//UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	//void SetSuppressSubtitles(bool bInSuppressSubtitles) { bSuppressSubtitles = bInSuppressSubtitles; }

	/**
	 * @return Whether override settings on this section should be used
	 */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	bool GetOverrideAttenuation() const { return bOverrideAttenuation; }

	/** Set whether the attentuation should be overriden */
	UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	void SetOverrideAttenuation(bool bInOverrideAttenuation) { bOverrideAttenuation = bInOverrideAttenuation; }

	/**
	 * @return The attenuation settings
	 */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	UAtomAttenuation* GetAttenuationSettings() const { return AttenuationSettings; }

	/** Set the attenuation settings for this audio section */
	UFUNCTION(BlueprintCallable, Category = "Sequencer|Section")
	void SetAttenuationSettings(UAtomAttenuation* InAttenuationSettings) { AttenuationSettings = InAttenuationSettings; }

	/**
	 * @return The attenuation settings overrides
	 */
	UFUNCTION(BlueprintPure, Category = "Sequencer|Section")
	const FAtomAttenuationSettings& GetAttenuationOverrides() const { return AttenuationOverrides; }

	/**
	 * @return The attach actor data
	 */
	const FMovieSceneActorReferenceData& GetAttachActorData() const { return AttachActorData; }

	/*
	 * @return The attach component given the bound actor and the actor attach key with the component and socket names
	 */
	CRIWAREMOVIESCENES_API USceneComponent* GetAttachComponent(const AActor* InParentActor, const FMovieSceneActorReferenceKey& Key) const;

	/** */
	CRIWAREMOVIESCENES_API TSubclassOf<UAtomComponent> GetAtomComponentClass() const { return AtomComponentClass; }

	/** */
	CRIWAREMOVIESCENES_API UAtomComponentExtensionSettings* GetAtomComponentExtensionSettings() const { return ExtensionSettings; }

	/** ~UObject interface */
	CRIWAREMOVIESCENES_API virtual void PostLoad() override;

	/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
	/*
	void SetOnQueueSubtitles(const FOnQueueSubtitles& InOnQueueSubtitles)
	{
		OnQueueSubtitles = InOnQueueSubtitles;
	}
	*/

	/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
	/*
	const FOnQueueSubtitles& GetOnQueueSubtitles() const
	{
		return OnQueueSubtitles;
	}
	*/

	/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
	void SetOnAtomSoundFinished(const FOnAtomSoundFinished& InOnAtomSoundFinished)
	{
		OnAtomSoundFinished = InOnAtomSoundFinished;
	}

	/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
	const FOnAtomSoundFinished& GetOnAtomSoundFinished() const
	{
		return OnAtomSoundFinished;
	}

	void SetOnAtomSoundPlaybackPercent(const FOnAtomSoundPlaybackPercent& InOnAtomSoundPlaybackPercent)
	{
		OnAtomSoundPlaybackPercent = InOnAtomSoundPlaybackPercent;
	}

	const FOnAtomSoundPlaybackPercent& GetOnAtomSoundPlaybackPercent() const
	{
		return OnAtomSoundPlaybackPercent;
	}

	/**
	 * @return Whether ExtensionSettings can be used to override the AtomComponent settings.
	 */
	bool IsApplyingExtensionSettings() const { return bApplyExtensionSettings; }

	TArray<FAtomSelectorParam> GetSelectorLabels() const { return SelectorLabels; }
	int32 GetBlockIndex() const { return FirstBlockIndex; }

	/** Gets the animated Aisac controls and their associated curves. */
	CRIWAREMOVIESCENES_API TArray<FAisacControlAndCurve>& GetAisacControlsAndCurves();
	CRIWAREMOVIESCENES_API const TArray<FAisacControlAndCurve>& GetAisacControlsAndCurves() const;

	/** Overloads for each input type, const */
	void ForEachInput(TFunction<void(FName, const FMovieSceneBoolChannel&)> InFunction) const { ForEachInternal(InFunction, Inputs_Bool); }
	void ForEachInput(TFunction<void(FName, const FMovieSceneStringChannel&)> InFunction) const { ForEachInternal(InFunction, Inputs_String); }
	void ForEachInput(TFunction<void(FName, const FMovieSceneIntegerChannel&)> InFunction) const { ForEachInternal(InFunction, Inputs_Int); }
	void ForEachInput(TFunction<void(FName, const FMovieSceneFloatChannel&)> InFunction) const { ForEachInternal(InFunction, Inputs_Float); }
	//void ForEachInput(TFunction<void(FName, const FMovieSceneAtomTriggerChannel&)> InFunction) const { ForEachInternal(InFunction, Inputs_Trigger); }

	CRIWAREMOVIESCENES_API void AddAisacChannel(const FAtomAisacControl& AisacControl);
	CRIWAREMOVIESCENES_API bool DeleteAisacChannel(FName AisacControlName);

public:

	//~ MovieSceneSection interface
	CRIWAREMOVIESCENES_API virtual TOptional<TRange<FFrameNumber>> GetAutoSizeRange() const override;
	CRIWAREMOVIESCENES_API virtual void TrimSection(FQualifiedFrameTime TrimTime, bool bTrimLeft, bool bDeleteKeys) override;
	CRIWAREMOVIESCENES_API virtual UMovieSceneSection* SplitSection(FQualifiedFrameTime SplitTime, bool bDeleteKeys) override;
	CRIWAREMOVIESCENES_API virtual TOptional<FFrameTime> GetOffsetTime() const override;
	CRIWAREMOVIESCENES_API virtual void MigrateFrameTimes(FFrameRate SourceRate, FFrameRate DestinationRate) override;
	CRIWAREMOVIESCENES_API virtual void PostEditImport() override;
	CRIWAREMOVIESCENES_API virtual EMovieSceneChannelProxyType CacheChannelProxy() override;

	//~ IMovieSceneEntityProvider interface
	CRIWAREMOVIESCENES_API virtual bool PopulateEvaluationFieldImpl(const TRange<FFrameNumber>& EffectiveRange, const FMovieSceneEvaluationFieldEntityMetaData& InMetaData, FMovieSceneEntityComponentFieldBuilder* OutFieldBuilder) override;
	CRIWAREMOVIESCENES_API virtual void ImportEntityImpl(UMovieSceneEntitySystemLinker* EntityLinker, const FEntityImportParams& Params, FImportedEntity* OutImportedEntity) override;

#if WITH_EDITOR
	CRIWAREMOVIESCENES_API virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	CRIWAREMOVIESCENES_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

#if WITH_EDITOR
	TSubclassOf<UAtomComponent> CachedAtomComponentClass;
#endif

	template<typename ChannelType, typename ForEachFunction>
	FORCEINLINE static void ForEachInternal(ForEachFunction InFuncton, const TMap<FName, ChannelType>& InMapToIterate)
	{
		for (auto& Item : InMapToIterate)
		{
			InFuncton(Item.Key, Item.Value);
		}
	}

	// CRIWAREMOVIESCENES_API void SetupSoundInputParameters(USoundBase* InSoundBase);

	/** The sound cue or wave that this section plays. */
	UPROPERTY(EditAnywhere, Category = "Atom")
	TObjectPtr<UAtomSoundBase> Sound;

	/** The class of Atom component to use for this section. */
	UPROPERTY(EditAnywhere, Category = "Atom")
	TSubclassOf<UAtomComponent> AtomComponentClass;

	/** Whether to use the Atom component extension settings if available. */
	UPROPERTY(EditAnywhere, Category = "Atom", meta = (EditCondition = "ExtensionSettings != nullptr"))
	bool bApplyExtensionSettings;

	/** The offset into the beginning of the audio clip. */
	UPROPERTY(EditAnywhere, Category = "Atom")
	FFrameNumber StartFrameOffset;

	/** The volume the sound will be played with. */
	UPROPERTY()
	FMovieSceneFloatChannel SoundVolume;

	/** The pitch multiplier the sound will be played with. */
	UPROPERTY()
	FMovieSceneFloatChannel PitchMultiplier;

	/** Generic inputs for the sound  */
	UPROPERTY()
	TMap<FName, FMovieSceneFloatChannel> Inputs_Float;
	UPROPERTY()
	TMap<FName, FMovieSceneStringChannel> Inputs_String;
	UPROPERTY()
	TMap<FName, FMovieSceneBoolChannel> Inputs_Bool;
	UPROPERTY()
	TMap<FName, FMovieSceneIntegerChannel> Inputs_Int;
	//UPROPERTY()
	//TMap<FName, FMovieSceneAtomTriggerChannel> Inputs_Trigger;

	UPROPERTY()
	FMovieSceneActorReferenceData AttachActorData;

	/** The scalar parameter names and their associated curves. */
	UPROPERTY()
	TArray<FAisacControlAndCurve> AisacControlsAndCurves;

	/* Allow looping if the section length is greater than the sound duration */
	UPROPERTY(EditAnywhere, Category = "Atom")
	bool bLooping;

	//UPROPERTY(EditAnywhere, Category = "Atom")
	//bool bSuppressSubtitles;

	/** Should the attenuation settings on this section be used. */
	UPROPERTY(EditAnywhere, Category = "Attenuation")
	bool bOverrideAttenuation;

	/** The attenuation settings to use. */
	UPROPERTY(EditAnywhere, Category = "Attenuation")
	TObjectPtr<UAtomAttenuation> AttenuationSettings;

	/** If bOverrideSettings is true, the attenuation properties to use for sounds generated by this component */
	UPROPERTY(EditAnywhere, Category = Attenuation, meta = (EditCondition = "bOverrideAttenuation", DisplayAfter = "bOverrideAttenuation", EditConditionHides))
	FAtomAttenuationSettings AttenuationOverrides;

	/** Selector label to set when playing sound (Sound Cue Only) */
	UPROPERTY(EditAnywhere, Category = "Atom")
	TArray<FAtomSelectorParam> SelectorLabels;

	/** Block index to set when playing sound (Sound Cue Only) */
	UPROPERTY(EditAnywhere, Category = "Atom")
	int32 FirstBlockIndex;

	/** Called when subtitles are sent to the SubtitleManager.  Set this delegate if you want to hijack the subtitles for other purposes */
	//UPROPERTY()
	//FOnQueueSubtitles OnQueueSubtitles;

	/** The specific settings to use if using extended AtomComponent. */
	UPROPERTY()
	TObjectPtr<UAtomComponentExtensionSettings> ExtensionSettings;

	/** called when we finish playing audio, either because it played to completion or because a Stop() call turned it off early */
	UPROPERTY()
	FOnAtomSoundFinished OnAtomSoundFinished;

	UPROPERTY()
	FOnAtomSoundPlaybackPercent OnAtomSoundPlaybackPercent;

#if WITH_EDITORONLY_DATA
private:
	UE_DEPRECATED(5.0, "Please use Keep State or an event track to control an external sound instead.")
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Please use Keep State or an event track to control an external sound instead."))
	bool bContinueSoundWhenSequenceIsEnd_DEPRECATED;
#endif
};
