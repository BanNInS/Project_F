/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundCue.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ScriptMacros.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomSoundBase.h"
#include "AtomAttenuation.h"

#include "Atom/AtomActiveSound.h"

#include "AtomSoundCue.generated.h"

 // Forward Declarations
class UAtomCueSheet;
class UAtomCueSheetFactory;
class IAtomSoundResource;

UENUM()
enum class EAtomCueType : uint8
{
	/** Polyphonic. */
	Polyphonic = 0,
	
	/** Sequential. */
	Sequential,
	
	/** Shuffle. */
	Shuffle,
	
	/** Random. */
	Random,

	/** Random but not repeated. (sounds played randomly other than the last played sound.) */
	RandomNoRepeat,
	
	/** Switch playback (referring to game variables to switch playback tracks.) */
	SwitchGameVariable,
	
	/** Combo Sequential (Sequential if a continuous combo is determined within the "combo time", returns to the "combo loopback" point when it reaches the end.) */
	ComboSequential,	
	
	/** Switch playback (Referring to the selector to switch playback tracks.) */
	SwitchSelector,

	/** Track transition playback (referring to the selector to switch playback tracks.) */
	TrackTransitionBySelector,
};

inline const TCHAR* ToString(EAtomCueType Type)
{
	switch (Type) {
	case EAtomCueType::Polyphonic:					return TEXT("Polyphonic");
	case EAtomCueType::Sequential:					return TEXT("Sequential");
	case EAtomCueType::Shuffle:						return TEXT("Shuffle");
	case EAtomCueType::Random:						return TEXT("Random");
	case EAtomCueType::RandomNoRepeat:				return TEXT("Random No Repeat");
	case EAtomCueType::SwitchGameVariable:			return TEXT("Switch Game Variable");
	case EAtomCueType::ComboSequential:				return TEXT("Combo Sequential");
	case EAtomCueType::SwitchSelector:				return TEXT("Switch Selector");
	case EAtomCueType::TrackTransitionBySelector:	return TEXT("Track Transition by Selector");
	default:										return TEXT("Unknown");
	}
}

UENUM()
enum class EAtomCueSpatializationType : uint8
{
	/** This sound will use speaker panning by default. Attenuation settings can override to 3D positionning. */
	Panning UMETA(DisplayName = "Speaker Panning"),

	/** This sound will use 3D positionning by default. Attenuation settings will not override settings from Cue (Craft) when disabled (Default). */
	Positionning UMETA(DisplayName = "3D Positionning"),

	/** This sound will use 3D positionning by default. Attenuation settings always overrides settings from Cue (Craft). */
	Auto UMETA(DisplayName = "Auto (Overriding)")
};

inline const TCHAR* ToString(EAtomCueSpatializationType Type)
{
	switch (Type) {
	case EAtomCueSpatializationType::Panning:		return TEXT("Polyphonic");
	case EAtomCueSpatializationType::Positionning:	return TEXT("Positionning");
	case EAtomCueSpatializationType::Auto:			return TEXT("Auto");
	default:										return TEXT("Unknown");
	}
}

USTRUCT(BlueprintType, meta = (ToolTip = "AtomAttenuationDistanceParam structure."))
struct CRIWARECORE_API FAtomAttenuationDistanceParam
{
	GENERATED_BODY()

public:

	FAtomAttenuationDistanceParam();

	/** Minimal Distance. */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Minimal Distance.", ClampMin = "0.0"))
	float MinDistance;

	/** Maximal Distance. */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Maximal Distance.", ClampMin = "0.0"))
	float MaxDistance;

	/** Inner Cone Angle in degres. */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Inner Cone Angle.", ClampMin = "0.0", ClampMax = "360.0"))
	float ConeInsideAngle;

	/** Outter Cone Angle in degres. */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Outter Cone Angle.", ClampMin = "0.0", ClampMax = "360.0"))
	float ConeOutsideAngle;

	/** Source Radius */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Source Radius.", ClampMin = "0.0"))
	float SourceRadius;

	/** Interior Distance */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Interior Panning.", ClampMin = "0.0"))
	float InteriorDistance;

	/** Doppler Intensity. */
	UPROPERTY(EditAnywhere, Category = "Attenuation", meta = (ToolTip = "Doppler Intensity.", ClampMin = "0.0", ClampMax = "360.0"))
	float DopplerFactor;

	FORCEINLINE bool operator == (const FAtomAttenuationDistanceParam& Other) const
	{
		return MinDistance == Other.MinDistance 
			&& MaxDistance == Other.MaxDistance
			&& ConeInsideAngle == Other.ConeInsideAngle
			&& ConeOutsideAngle == Other.ConeOutsideAngle
			&& SourceRadius == Other.SourceRadius
			&& InteriorDistance == Other.InteriorDistance
			&& DopplerFactor == Other.DopplerFactor;
	}

	FORCEINLINE bool operator != (const FAtomAttenuationDistanceParam& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType, meta = (ToolTip = "AtomCueInfo structure."))
struct CRIWARECORE_API FAtomCueInfo
{
	GENERATED_BODY()

public:

	FAtomCueInfo();

	/** Cue identifier is used internaly to identify and play the cue. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	int32 ID;

	/** The cue name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	FString Name;

	/** The cue type. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, AssetRegistrySearchable)
	EAtomCueType Type;

	/** The cue duration. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, AssetRegistrySearchable)
	FTimespan Duration;
	
	/** The default volume applied to the cue in dB. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (DisplayName = "Volume (dB)"))
	float Volume;

	/** The default pitch value applied to the cue in cents. */
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (DisplayName = "Volume (Cent)"))
	//int32 Pitch;
	
	/** The playback probability of this cue. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (Units = %))
	float Probability;

	/** The cue virtualization mode (Voice Behaviour).*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (DisplayAfter = "bLooping"))
	EAtomVirtualizationMode VirtualizationMode;

	/** The playback priority of this cue. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	float Priority;

	/** Whether this cue is looping. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, AssetRegistrySearchable)
	uint8 bLooping : 1;

	/** Whether this cue is public accessible or used privately by another cue. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	uint8 bHeaderVisibility : 1;

	/** Whether parameter can be changed while playing or is locked to data settings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, AssetRegistrySearchable)
	uint8 bIgnorePlayerParameters : 1;

	/** Whether this cue is using parameter pallet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (DisplayAfter = "AttenuationDistance"))
	uint8 bIsParameterPalletAssigned : 1;

	/** The categories in which this cue is associated. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	TArray<FString> CategoryNames;

	/** The AISAC control this cue is bound to. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	TArray<FAtomAisacControl> AisacControls;

	/** The spatialization type used. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	EAtomCueSpatializationType SpatializationType;

	/** The cue volume attenuation distance parameters if using 3d positioning. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	FAtomAttenuationDistanceParam AttenuationDistance;

	/** The name of the game variable to use when this cue is of type SwitchGameVariable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue, meta = (EditCondition = "Type == EAtomCueType::SwitchGameVariable", EditConditionHides))
	FName GameVariable;

	/** */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	int32 CueLimit;

	/** */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	int32 NumBlocks;

	/** */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	int32 NumTracks;

	/** */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Cue)
	int32 NumRelatedWaveforms;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Meta = (DeprecatedProperty, DeprecationMessage = "Deprecated in favor of FAtomAisacControl. See AisacControls member."))
	TArray<FString> AisacControlNames_DEPRECATED;

	/** Whether this cue is using 3d positioning (Spatialization and volume attenuation). */
	UPROPERTY(Meta = (DeprecatedProperty, DeprecationMessage = "Deprecated in favor of EAtomPanType. See PanType member."))
	uint8 bIs3D_DEPRECATED : 1;
#endif
};

USTRUCT(BlueprintType, meta = (ToolTip = "AtomCueVoiceSettings structure."))
struct CRIWARECORE_API FAtomCueVoiceSettings
{
	GENERATED_BODY()

public:

	/** Overrides the number of channels used by a voice when using dynamically managed voices. */
	UPROPERTY(EditAnywhere, Category = "Voice Management", meta = (InlineEditConditionToggle))
	uint8 bOverrideNumChannels : 1 = false;

	/** Overrides the sampling rate used by a voice when using dynamically managed voices. */
	UPROPERTY(EditAnywhere, Category = "Voice Management", meta = (InlineEditConditionToggle))
	uint8 bOverrideSamplingRate : 1 = false;

	/** Overrides the number of channels used by a voice when using dynamically managed voices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Management", meta = (
		EditCondition = "bOverrideNumChannels", DisplayName = "Number of Channels",
		ClampMin = 1, ClampMax = 16, UIMin = 1, UIMax = 16,
		Tooltip = "Overrides the number of channels used by a voice when using dynamically managed voices.."))
	int32 NumChannels = 1;

	/** Overrides the sampling rate used by a voice when using dynamically managed voices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Management", meta = (
		EditCondition = "bOverrideSamplingRate",
		ClampMin = 8000, ClampMax = 384000, SliderExponent = 10.0, Units = Hz,
		Tooltip = "Overrides the sampling rate used by a voice when using dynamically managed voices.."))
	int32 SamplingRate = 48000;
};

/** */
UCLASS(HideCategories = Object, BlueprintType, Within = AtomCueSheet)
class CRIWARECORE_API UAtomSoundCue
	: public UAtomSoundBase
{
	GENERATED_BODY()

public:

	UAtomSoundCue(const FObjectInitializer& ObjectInitializer);

	void Init(UAtomCueSheet* InCueSheet, const FAtomCueInfo& InCueInfo);

	//~ Begin UObject Interface
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
#endif
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UAtomSoundBase interface.
	virtual bool IsPlayable() const override;
	virtual bool IsStreaming(const TCHAR* PlatformName = nullptr) const override;
	virtual float GetMaxDistance() const;
	virtual float GetDuration() const;
	virtual float GetVolumeMultiplier() override;
	virtual float GetPitchMultiplier() override;
	virtual EAtomVirtualizationMode GetVirtualizationMode() const override;
	virtual const FAtomAttenuationSettings* GetAttenuationSettingsToApply() const override;
	virtual IAtomSoundResource* GetSoundResource() override;
	virtual void GetAllSoundResources(TArray<IAtomSoundResource*>& Resources) override;
	virtual void Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& PlaybackInstances) override;
	virtual FString GetSoundName() const override;
	//~ End UAtomSoundBase interface.

	/** Helper that returns the name of this cue. */
	UFUNCTION(BlueprintCallable, Category = Cue)
	FString GetCueName() const { return CueInfo.Name; }

	/** CueSheet holding this sound cue. */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = Cue)
	TObjectPtr<UAtomCueSheet> CueSheet;

	/** Indicates whether attenuation should use the Attenuation Overrides or the Attenuation Settings asset */
	UPROPERTY(EditAnywhere, Category = Attenuation)
	uint8 bOverrideAttenuation : 1;

	/** Cached cue information from cue sheet. */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = Cue, meta = (ShowOnlyInnerProperties))
	FAtomCueInfo CueInfo;

	/** Base volume multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound, AssetRegistrySearchable)
	float VolumeMultiplier;

	/** Base pitch multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound, AssetRegistrySearchable)
	float PitchMultiplier;

	/** Attenuation settings to use if Override Attenuation is set to true */
	UPROPERTY(EditAnywhere, Category = Attenuation)
	FAtomAttenuationSettings AttenuationOverrides;

	/** Whether to play this cue over dynamically managed voices or over shared voices defined in project property. */
	UPROPERTY(EditAnywhere, Category = "Voice Management", meta = (DisplayAfter = "VirtualizationMode"))
	bool bUseDynamicVoices = false;

	/** The maximum number of voice instances this cue can play when dynamically managed. Voices that play above this number may be stopped or virtualized according virtualisation settings of the cue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Management", meta = (
		ClampMin = 0, ClampMax = 32767, UIMin = 0, UIMax = 128, 
		Tooltip = "Specifies the number of voices that are reserved.", DisplayAfter = "bUseDynamicVoices"))
	int32 VoiceLimit = 1;

	/** Voice allocation settings to override for dynamically managed voices. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Management", meta = (DisplayAfter = "VoiceLimit"))
	FAtomCueVoiceSettings VoiceSettingsOverrides;

	/** Call to cache any values which need to be computed from the sound cue info. e.g. MaxDistance, Duration, etc. */
	void CacheAggregateValues();

#if WITH_EDITOR
	virtual bool GetWaveInfo(FAtomResourceHandle& InHandle, FAtomWaveInfo& OutWaveInfo) const;
#endif

	friend UAtomCueSheet;
	friend UAtomCueSheetFactory;
};
