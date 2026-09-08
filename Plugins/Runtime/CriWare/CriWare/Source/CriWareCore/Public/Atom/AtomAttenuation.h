/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomAttenuation.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Curves/CurveFloat.h"

#include "AtomAttenuation.generated.h"

// Forward declarations.
class UAtomBus;
class UAtomSourceDataOverridePluginSourceSettingsBase;
class UAtomRuntimePluginSettingsBase;
struct FAtomGlobalFocusSettings;

UENUM(BlueprintType)
enum class EAtomSpatializationAlgorithm : uint8
{
	// Standard panning method for spatialization (linear or equal power method defined in project settings)
	Default UMETA(DisplayName = "Panning"),

	// Binaural spatialization method if available (requires headphones, enabled by plugins)
	Binaural UMETA(DisplayName = "Object Based / Binaural"),
};

UENUM(BlueprintType)
enum class EAtomAttenuationDistanceModel : uint8
{
	//Linear,
	Logarithmic,
	//Inverse,
	//LogReverse,
	//NaturalSound,
	//Custom,
};

UENUM(BlueprintType)
enum class EAtomAttenuationShape : uint8
{
	Sphere,
	Cone,
	//Box,
	//Capsule,
};

UENUM(BlueprintType)
enum class EAtomAirAbsorptionMethod : uint8
{
	// The air absorption conform to a linear distance function
	Linear,

	// The air absorption conforms to a custom distance curve.
	CustomCurve,
};

UENUM(BlueprintType)
enum class EAtomBusSendMethod : uint8
{
	// A bus send based on linear interpolation between a distance range and send-level range
	Linear,

	// A bus send based on a supplied curve
	CustomCurve,

	// A manual bus send level (Uses the specified constant send level value. Useful for 2D sounds.)
	Manual,
};

UENUM(BlueprintType)
enum class EAtomPriorityAttenuationMethod : uint8
{
	// A priority attenuation based on linear interpolation between a distance range and priority attenuation range
	Linear,

	// A priority attenuation based on a supplied curve
	CustomCurve,

	// A manual priority attenuation (Uses the specified constant value. Useful for 2D sounds.)
	Manual,
};

UENUM(BlueprintType)
enum class EAtomMultiPositionType : uint8
{
	// Normal attenuation. Uses only the position of the sound or uses the first given position set with SetMultiplePositions().
	SingleSource,

	// Use all the given positions with the sound attenuation settings.
	MultiSources,
};

// Defines how to speaker map the sound when using the non-spatialized radius feature
UENUM(BlueprintType)
enum class EAtomNonSpatializedRadiusSpeakerMapMode : uint8
{
	// Will blend the 3D sound to an omni-directional sound (equal output mapping in all directions)
	//OmniDirectional,

	// Will blend the 3D source to the same representation speaker map used when playing the asset 2D
	Direct2D,

	// Will blend the 3D source to a multichannel 2D version (i.e. upmix stereo to quad) if rendering in surround
	//Surround2D,
};

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAttenuationPluginSettings
{
	GENERATED_BODY()

public:

	/** Settings to use with source data override audio plugin. These are defined by the plugin creator. Not all audio plugins utilize this feature. This is an array so multiple plugins can have settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSourceDataOverride, meta = (DisplayName = "Source Data Override Plugin Settings"))
	TArray<TObjectPtr<UAtomSourceDataOverridePluginSourceSettingsBase>> SourceDataOverridePluginSettingsArray;

	/** Settings to use with runtime audio plugin. These are defined by the plugin creator. Not all audio plugins utilize this feature. This is an array so multiple plugins can have settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSourceDataOverride, meta = (DisplayName = "Runtime Plugin Settings"))
	TArray<TObjectPtr<UAtomRuntimePluginSettingsBase>> RuntimePluginSettingsArray;
};

/*
 * The settings for attenuation using BusAtom send.
 */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAttenuationBusSendSettings
{
	GENERATED_BODY()

	/** Bus to send audio to based on distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends)
	TObjectPtr<UAtomBus> Bus = nullptr;

	/** What method to use to use for Bus sends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends)
	EAtomBusSendMethod BusSendMethod = EAtomBusSendMethod::Linear;

	/** The amount to send to the Bus when the sound is located at a distance equal to value specified in the bus send distance min. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends, meta = (DisplayName = "Bus Min Send Level"))
	float BusSendLevelMin = 0.0f;

	/** The amount to send to the Bus when the sound is located at a distance equal to value specified in the bus send distance max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends, meta = (DisplayName = "Bus Max Send Level"))
	float BusSendLevelMax = 1.0f;

	/** The min distance to send to the Bus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends, meta = (DisplayName = "Bus Min Send Distance"))
	float BusSendDistanceMin = 400.0f;

	/** The max distance to send to the Bus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends, meta = (DisplayName = "Bus Max Send Distance"))
	float BusSendDistanceMax = 6000.0f;

	/* The manual Bus send level to use. Doesn't change as a function of distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends)
	float ManualBusSendLevel = 0.2f;

	/* The custom Bus send curve to use for distance-based send level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends)
	FRuntimeFloatCurve CustomBusSendCurve;
};

/*
 * The settings for attenuation from multiple source positions.
 */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomMultiPositionSettings
{
	GENERATED_BODY()

	/** Uses the Atom component position as attenuation source positions. */
	UPROPERTY(EditAnywhere, Category = MultiPosition)
	bool bUseComponentPositionAsSource = true;

	/** Actors to use as attenuation source positions. */
	UPROPERTY(EditAnywhere, Category = MultiPosition)
	TArray<TObjectPtr<AActor>> SourceActors;

	/** Classes of scene component in source actors to use as attenuation source positions. If not set, the Actor's root transform is used. */
	UPROPERTY(EditAnywhere, Category = MultiPosition)
	TSubclassOf<USceneComponent> SourceComponentClass = nullptr;

	/** Transforms to use as attenuation source positions. */
	UPROPERTY(EditAnywhere, Category = MultiPosition)
	TArray<FTransform> SourcePositions;
};

/*
 * The settings for attenuating a sound with Atom.
 */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomAttenuationSettings
{
	GENERATED_BODY()

public: // booleans & enums (uint8)

	/* Allows distance-based volume attenuation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (DisplayName = "Enable Volume Attenuation"))
	uint8 bAttenuate : 1;

	/* Allows the source to be 3D spatialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (DisplayName = "Enable Spatialization"))
	uint8 bSpatialize : 1;

	/** Allows simulation of air absorption by applying a filter with a cutoff frequency as a function of distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Enable Air Absorption"))
	uint8 bAttenuateWithLPF : 1;

	/** Enable listener focus-based adjustments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus)
	uint8 bEnableListenerFocus : 1;

	/** Enables focus interpolation to smooth transition in and and of focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus)
	uint8 bEnableFocusInterpolation : 1;

	/** Enables realtime occlusion tracing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion)
	uint8 bEnableOcclusion : 1;

	/** Enables tracing against complex collision when doing occlusion traces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion)
	uint8 bUseComplexCollisionForOcclusion : 1;

	/** Enables adjusting reverb sends based on distance. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend, meta = (DisplayName = "Enable Reverb Send"))
	//uint8 bEnableReverbSend : 1;

	/** Enables attenuation of sound priority based off distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (DisplayName = "Enable Priority Attenuation"))
	uint8 bEnablePriorityAttenuation : 1;

	/** Enables applying a -6 dB attenuation to stereo assets which are 3d spatialized. Avoids clipping when assets have spread of 0.0 due to channel summing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (DisplayName = "Normalize 3D Stereo Sounds"))
	uint8 bApplyNormalizationToStereoSounds : 1;

	/** Enables applying a log scale to frequency values (so frequency sweeping is perceptually linear). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Enable Log Frequency Scaling"))
	uint8 bEnableLogFrequencyScaling : 1;

	/** Enables AtomBus sends based on distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends, meta = (DisplayName = "Enable Bus Sends"))
	uint8 bEnableBusSends : 1;

	/** Enables overriding PlaybackInstance data using source data override plugin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSourceDataOverride, meta = (DisplayName = "Enable Source Data Override"))
	uint8 bEnableSourceDataOverride : 1;

	/* The type of attenuation as a function of distance to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (DisplayName = "Attenuation Function"))
	EAtomAttenuationDistanceModel DistanceAlgorithm;

	/* The shape of the non-custom attenuation method. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance)
	EAtomAttenuationShape AttenuationShape;

	/** What method to use to map distance values to frequency absorption values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption)
	EAtomAirAbsorptionMethod AbsorptionMethod;

	/** What method we use to spatialize the sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize", DisplayName = "Spatialization Method"))
	EAtomSpatializationAlgorithm SpatializationAlgorithm;

	/* Which trace channel to use for Atom occlusion checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion)
	TEnumAsByte<enum ECollisionChannel> OcclusionTraceChannel;

	/** What method to use to control master reverb sends */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend)
	//EReverbSendMethod ReverbSendMethod;

	/** What method to use to control priority attenuation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority)
	EAtomPriorityAttenuationMethod PriorityAttenuationMethod;

public: // Distance

	/* The dimensions to use for the attenuation shape. Interpretation of the values differ per shape.
	   Sphere  - X is Sphere Radius. Y and Z are unused
	   Cone    - X is Cone Radius, Y is Cone Angle, Z is Cone Falloff Angle
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance)
	FVector AttenuationShapeExtents;

	/* The distance back from the sound's origin to begin the cone when using the cone attenuation shape. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (ClampMin = "0"))
	//float ConeOffset;

	/* The volume level back from the sound's origin to begin the cone when using the cone attenuation shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (ClampMin = "0", clampMax = "1"))
	float ConeOutsideLevel;

	/* The distance over which volume attenuation occurs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (ClampMin = "0"))
	float FalloffDistance;

	/* An optional attenuation radius (sphere) that extends from the cone origin. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (ClampMin = "0"))
	//float ConeSphereRadius;

	/* The distance over which volume attenuation occurs for the optional sphere shape. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationDistance, meta = (ClampMin = "0"))
	//float ConeSphereFalloffDistance;

public: // Spatialization

#if WITH_EDITORONLY_DATA
	
	UPROPERTY()
	float OmniRadius_DEPRECATED;
#endif

	/** What min radius to use to swap to non-binaural audio when a sound starts playing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize"))
	float BinauralRadius;

	/** The distance below which a sound begins to linearly interpolate towards being non-spatialized (2D). See "Non Spatialized Radius End" to define the end of the interpolation and the "Non Spatialized Radius Mode" for the mode of the interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize"))
	float NonSpatializedRadiusStart;

	/** The distance below which a sound is fully non-spatialized (2D). See "Non Spatialized Radius Start" to define the start of the interpolation and the "Non Spatialized Radius Mode" for the mode of the interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize"))
	float NonSpatializedRadiusEnd;

	/** Defines how to interpolate a 3D sound towards a 2D sound when using the non-spatialized radius start and end properties. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize"))
	EAtomNonSpatializedRadiusSpeakerMapMode NonSpatializedRadiusMode;

	/** The world-space distance between left and right stereo channels when stereo assets are 3D spatialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationSpatialization, meta = (ClampMin = "0", EditCondition = "bSpatialize", DisplayName = "3D Stereo Spread"))
	float StereoSpread;

public: // Air Absorption

	/* The distance min range at which to apply an absorption LPF filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Min Distance Range"))
	float LPFRadiusMin;

	/* The max distance range at which to apply an absorption LPF filter. Absorption freq cutoff interpolates between filter frequency ranges between these distance values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Max Distance Range"))
	float LPFRadiusMax;

	/* The normalized custom curve to use for the air absorption lowpass frequency values. Does a mapping from defined distance values (x-axis) and defined frequency values (y-axis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption)
	FRuntimeFloatCurve CustomLowpassAirAbsorptionCurve;

	/* The normalized custom curve to use for the air absorption highpass frequency values. Does a mapping from defined distance values (x-axis) and defined frequency values (y-axis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption)
	FRuntimeFloatCurve CustomHighpassAirAbsorptionCurve;

	/* The range of the cutoff frequency (in Hz) of the lowpass absorption filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Low Pass Cutoff Frequency Min"))
	float LPFFrequencyAtMin;

	/* The range of the cutoff frequency (in Hz) of the lowpass absorption filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "Low Pass Cutoff Frequency Max"))
	float LPFFrequencyAtMax;

	/* The range of the cutoff frequency (in Hz) of the highpass absorption filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "High Pass Cutoff Frequency Min"))
	float HPFFrequencyAtMin;

	/* The range of the cutoff frequency (in Hz) of the highpass absorption filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationAirAbsorption, meta = (DisplayName = "High Pass Cutoff Frequency Max"))
	float HPFFrequencyAtMax;

public: // ListenerFocus

	/** Azimuth angle (in degrees) relative to the listener forward vector which defines the focus region of sounds. Sounds playing at an angle less than this will be in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus)
	float FocusAzimuth;

	/** Azimuth angle (in degrees) relative to the listener forward vector which defines the non-focus region of sounds. Sounds playing at an angle greater than this will be out of focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus)
	float NonFocusAzimuth;

	/** Amount to scale the distance calculation of sounds that are in-focus. Can be used to make in-focus sounds appear to be closer or further away than they actually are. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float FocusDistanceScale;

	/** Amount to scale the distance calculation of sounds that are not in-focus. Can be used to make in-focus sounds appear to be closer or further away than they actually are.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusDistanceScale;

	/** Amount to scale the priority of sounds that are in focus. Can be used to boost the priority of sounds that are in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableListenerFocus"))
	float FocusPriorityScale;

	/** Amount to scale the priority of sounds that are not in-focus. Can be used to reduce the priority of sounds that are not in focus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusPriorityScale;

	/** Amount to attenuate sounds that are in focus. Can be overridden at the sound-level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float FocusVolumeAttenuation;

	/** Amount to attenuate sounds that are not in focus. Can be overridden at the sound-level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableListenerFocus"))
	float NonFocusVolumeAttenuation;

	/** Scalar used to increase interpolation speed upwards to the target Focus value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableFocusInterpolation"))
	float FocusAttackInterpSpeed;

	/** Scalar used to increase interpolation speed downwards to the target Focus value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationListenerFocus, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableFocusInterpolation"))
	float FocusReleaseInterpSpeed;

public: // Occlusion

	/** The low pass filter frequency (in Hz) to apply if the sound playing in this audio component is occluded. This will override the frequency set in LowPassFilterFrequency. A frequency of 0.0 is the device sample rate and will bypass the filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionLowPassFilterFrequency;

	/** The amount of volume attenuation to apply to sounds which are occluded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion, meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionVolumeAttenuation;

	/** The amount of time in seconds to interpolate to the target OcclusionLowPassFilterFrequency when a sound is occluded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationOcclusion, meta = (ClampMin = "0", UIMin = "0.0", EditCondition = "bEnableOcclusion"))
	float OcclusionInterpolationTime;

public: // ReverbSend

	/** The amount to send to master reverb when sound is located at a distance equal to value specified in the reverb min send distance. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend, meta = (DisplayName = "Reverb Min Send Level"))
	//float ReverbWetLevelMin;

	/** The amount to send to master reverb when sound is located at a distance equal to value specified in the reverb max send distance. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend, meta = (DisplayName = "Reverb Max Send Level"))
	//float ReverbWetLevelMax;

	/** The min distance to send to the master reverb. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend, meta = (DisplayName = "Reverb Min Send Distance"))
	//float ReverbDistanceMin;

	/** The max distance to send to the master reverb. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend, meta = (DisplayName = "Reverb Max Send Distance"))
	//float ReverbDistanceMax;

	/* The manual master reverb send level to use. Doesn't change as a function of distance. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend)
	//float ManualReverbSendLevel;

	/* The custom reverb send curve to use for distance-based send level. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationReverbSend)
	//FRuntimeFloatCurve CustomReverbSendCurve;

public: // BusSends

	/** Set of submix send settings to use to send audio to submixes as a function of distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationBusSends)
	TArray<FAtomAttenuationBusSendSettings> BusSendSettings;

public: // Priority

	/** Interpolated value to scale priority against when the sound is at the minimum priority attenuation distance from the closest listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Priority Attenuation At Min Distance"))
	float PriorityAttenuationMin;

	/** Interpolated value to scale priority against when the sound is at the maximum priority attenuation distance from the closest listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Priority Attenuation At Max Distance"))
	float PriorityAttenuationMax;

	/** The min distance to attenuate priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (ClampMin = "0.0", DisplayName = "Priority Attenuation Min Distance"))
	float PriorityAttenuationDistanceMin;

	/** The max distance to attenuate priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (ClampMin = "0.0", DisplayName = "Priority Attenuation Max Distance"))
	float PriorityAttenuationDistanceMax;

	/* Static priority scalar to use (doesn't change as a function of distance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", DisplayName = "Attenuation Priority"))
	float ManualPriorityAttenuation;

	/* The custom curve to use for distance-based priority attenuation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPriority)
	FRuntimeFloatCurve CustomPriorityAttenuationCurve;

public: // Plugins

	/** Sound attenuation plugin settings to use with sounds that play with this attenuation setting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttenuationPluginSettings, meta = (ShowOnlyInnerProperties))
	FAtomAttenuationPluginSettings PluginSettings;

public:

	FAtomAttenuationSettings();	
	bool operator==(const FAtomAttenuationSettings& Other) const;
#if WITH_EDITORONLY_DATA
	void PostSerialize(const FArchive& Ar);
#endif

	float GetMinDimension() const;
	float GetMaxDimension() const;
	float GetMaxFalloffDistance() const;
	float GetConeAnglesAndOutsideLevel(float& InnerAngle, float& OuterAngle) const;

	struct AttenuationShapeDetails
	{
		FVector Extents;
		float Falloff;
		//float ConeOffset;
		float ConeSphereRadius;
		float ConeSphereFalloff;
	};

	void CollectAttenuationShapesForVisualization(TMultiMap<EAtomAttenuationShape, AttenuationShapeDetails>& ShapeDetailsMap) const;
	float GetFocusPriorityScale(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const;
	float GetFocusAttenuation(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const;
	float GetFocusDistanceScale(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const;
};

/**
 * Defines how a sound changes volume with distance to the listener.
 */
UCLASS(BlueprintType, hidecategories = Object, EditInlineNew, MinimalAPI)
class UAtomAttenuation 
	: public UObject
{
	GENERATED_BODY()

public:

	UAtomAttenuation(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attenuation, meta = (CustomizeProperty, EditCondition = "bIsEditable"))
	FAtomAttenuationSettings Attenuation;

#if WITH_EDITORONLY_DATA
	/**
	 * Was this asset created by a external or procedural tool?
	 * It's used to tell the users that they shouldn't modify the asset by themselves.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Asset, AdvancedDisplay)
	bool bIsEditable = true;
#endif
};
