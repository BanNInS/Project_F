/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomActiveSound.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "WorldCollision.h"
#include "HAL/ThreadSafeBool.h"
#include "Atom/AtomQuartzQuantizationUtilities.h"

#include "Atom.h"
#include "AtomDebug.h"
#include "AtomSoundBase.h"
#include "AtomAttenuation.h"
#include "AtomConcurrency.h"
#include "AtomAisacPatch.h"
#include "AtomEnvelope.h"
#include "AtomSourceVoiceEffect.h"
#include "AtomVolumeFader.h"
#include "Modulation/AtomModulationDestination.h"
#include "Modulation/AtomAisacModulation.h"

// Forward Declarations
struct FRuntimeFloatCurve;
class FAtomRuntime;
class FAtomSource;
class UAtomSoundBase;
class UAtomRack;
class UAtomComponent;
class UAtomSoundClass;
struct FAtomSoundSourceBusSendInfo;
struct FAtomListener;
struct FAtomAttenuationListenerData;
namespace Atom {
	class FAtomExPlayer;
	struct FAtomExPLayerArgs;
}

/**
 * Attenuation focus system data computed per update per active sound
 */
struct FAtomAttenuationFocusData
{
	/** Azimuth of the active sound relative to the listener. Used by sound  focus. */
	float Azimuth = 0.0f;

	/** Absolute azimuth of the active sound relative to the listener. Used for 3d audio calculations. */
	float AbsoluteAzimuth = 0.0f;

	/** Value used to allow smooth interpolation in/out of focus */
	float FocusFactor = 1.0f;

	/** Cached calculation of the amount distance is scaled due to focus */
	float DistanceScale = 1.0f;

	/** The amount priority is scaled due to focus */
	float PriorityScale = 1.0f;

	/** Cached highest priority of the parent active sound's wave instances. */
	float PriorityHighest = 1.0f;

	/** The amount volume is scaled due to focus */
	float VolumeScale = 1.0f;

	/** If this is the first update for focus. Handles edge case of starting a sound in-focus or out-of-focus. */
	bool bFirstFocusUpdate = true;
};

/**
 *	Struct used for gathering the final parameters to apply to a player instance
 * TODO: -> may have various version for the 3 type of sources (Player, ExPlayer and ExPlaybackInstance))
 *  Should be with ActiveSound or players
 */
struct FAtomSoundParseParameters
{
	// A collection of finish notification hooks
	/** Runtime will call the NotifyPlaybackFinished function of the hooked objects. */
	FAtomNotifyPlaybackFinishedHooks NotifyPlaybackFinishedHooks;

	// The Sound Class to use the settings of
	UAtomSoundClass* SoundClass;

	// The transform of the sound (scale is not used)
	FTransform Transform;

	/** Multi-Position type */
	EAtomMultiPositionType MultiPositionType;

	/** Multi-Position locations and directions. */
	TArray<FTransform> MultiPositions;

	// The speed that the sound is moving relative to the listener
	FVector Velocity;

	// The volume product of the sound
	float Volume;

	// The attenuation of the sound due to distance attenuation
	float DistanceAttenuation;

	// The attenuation of the sound due to occlusion attenuation
	float OcclusionAttenuation;

	// A volume scale on the sound specified by user
	float VolumeMultiplier;

	// Attack time of the source envelope follower
	int32 EnvelopeFollowerAttackTime;

	// Release time of the source envelope follower
	int32 EnvelopeFollowerReleaseTime;

	// The multiplier to apply if the sound category desires
	float InteriorVolumeMultiplier;
	
	// An envelope to apply to volume amplitude. 
	FAtomEnvelope AmplitudeEnvelope;

	// The priority of sound, which is the product of the component priority and the UAtomSoundBase priority
	float Priority;

	// The pitch scale factor of the sound
	float Pitch;

	// Time offset from beginning of sound to start at
	float StartTime;

	// At what distance from the source of the sound should spatialization begin 
	float NonSpatializedRadiusStart;

	// At what distance from the source the sound is fully non-spatialized
	float NonSpatializedRadiusEnd;

	// Which mode to use for non-spatialized radius
	EAtomNonSpatializedRadiusSpeakerMapMode NonSpatializedRadiusMode;

	// The distance over which the sound is attenuated
	float AttenuationDistance;

	// The distance from the listener to the sound
	float ListenerToSoundDistance;

	// The distance from the listener to the sound (ignores attenuation settings)
	float ListenerToSoundDistanceForPanning;

	// The absolute azimuth angle of the sound relative to the forward listener vector (359 degrees to left, 1 degrees to right)
	float AbsoluteAzimuth;

	// The sound rack to use for the wave instance
	UAtomRackBase* SoundRack;

	// The rack bus sends to use (overrides) (submix)
	TArray<FAtomSoundToBusSend> SoundBusSends;

	// The source bus sends to use
	TArray<FAtomSoundSourceBusSendInfo> SourceBusSends[(int32)EAtomBusSendStage::Count];

	// Reverb wet-level parameters
	//EReverbSendMethod ReverbSendMethod;
	//FVector2D ReverbSendLevelRange;
	//FVector2D ReverbSendLevelDistanceRange;
	//float ManualReverbSendLevel;
	//FRuntimeFloatCurve CustomReverbSendCurve;

	// Submix send params to use for this sound (from attenuations settings)
	TArray<FAtomAttenuationBusSendSettings> BusSendSettings;

	// The distance between left and right channels when spatializing stereo assets
	float StereoSpread;

	// Which spatialization algorithm to use
	EAtomSpatializationAlgorithm SpatializationMethod;

	// Whether the spatialization plugin is an external send
	bool bSpatializationIsExternalSend;

	// What spatialization plugin source settings to use
	//UAtomSpatializationPluginSourceSettingsBase* SpatializationPluginSettings;

	// What occlusion plugin source settings to use
	//UAtomOcclusionPluginSourceSettingsBase* OcclusionPluginSettings;

	// What reverb plugin source settings to use
	//UAtomReverbPluginSourceSettingsBase* ReverbPluginSettings;

	// What source data override plugin source settings to use
	UAtomSourceDataOverridePluginSourceSettingsBase* SourceDataOverridePluginSettings;

	// What runtime plugin source settings to use
	TArray<UAtomRuntimePluginSettingsBase*> RuntimePluginSettingsArray;

	// Categories - cannot be changed while playback
	TArray<FName> CategoryNames;

	// AISAC
	TArray<FName> AdditionalAisacPatchNames;
	TArray<FAtomAisacParameter> AisacControlParams;
	TArray<FAtomAisacControlSettings> AisacControlModulationRouting;

	// Cues
	TArray<FAtomSelectorParam> CueSelectorParams;
	int32 CueNextBlockIndex;
	int32 CueBeatSyncOffset;

	// The source voice effect (TimeStretch or PitchShift) to apply
	FAtomSourceVoiceEffect SourceVoiceEffect;

	// What source effect chain to use
	//USoundEffectSourcePresetChain* SourceEffectChain;

	// The lowpass filter frequency to apply (if enabled)
	float LowPassFilterFrequency;

	// The highpass filter frequency to apply (if enabled)
	float HighPassFilterFrequency;

	// The lowpass filter frequency to apply due to distance attenuation
	float AttenuationLowpassFilterFrequency;

	// The highpass filter frequency to apply due to distance attenuation
	float AttenuationHighpassFilterFrequency;

	// The lowpass filter to apply if the sound is occluded
	float OcclusionFilterFrequency;

	// The lowpass filter to apply if the sound is inside an ambient zone
	float AmbientZoneFilterFrequency;

	/** Whether or not to enable sending this audio's output to buses.*/
	uint32 bEnableSourceBusSends : 1;

	/** Whether or not to render to the main rack */
	uint32 bEnableSoundRack : 1;

	/** Whether or not to enable bus Sends from SoundBase in addition to the ActiveSound buses. */
	uint32 bEnableSoundBusSends : 1;

	/** Whether or not to enable source data override with a plugin. */
	uint32 bEnableSourceDataOverride : 1;

	// Whether the sound should be spatialized
	uint8 bUseSpatialization : 1;

	// Whether the sound should be seamlessly looped
	uint8 bLooping : 1;

	// Whether we have enabled low-pass filtering of this sound
	uint8 bEnableLowPassFilter : 1;

	// Whether we have enabled high-pass filtering of this sound
	uint8 bEnableHighPassFilter : 1;

	// Whether we have enabled amplitude envelope of this sound
	uint8 bEnableAmplitudeEnvelope : 1;

	// Whether this sound is occluded
	uint8 bIsOccluded : 1;

	// Whether or not this sound is manually paused (i.e. not by application-wide pause)
	uint8 bIsPaused : 1;

	// Whether or not this sound can re-trigger
	uint8 bEnableRetrigger : 1;

	// Whether or not to apply a =6 dB attenuation to stereo spatialization sounds
	uint8 bApplyNormalizationToStereoSounds : 1;

	FAtomSoundParseParameters()
		: SoundClass(nullptr)
		, Velocity(ForceInit)
		, Volume(1.0f)
		, DistanceAttenuation(1.0f)
		, OcclusionAttenuation(1.0f)
		, VolumeMultiplier(1.f)
		, EnvelopeFollowerAttackTime(10)
		, EnvelopeFollowerReleaseTime(100)
		, InteriorVolumeMultiplier(1.0f)
		, Pitch(1.0f)
		, StartTime(-1.0f)
		, NonSpatializedRadiusStart(0.0f)
		, NonSpatializedRadiusEnd(0.0f)
		, NonSpatializedRadiusMode(EAtomNonSpatializedRadiusSpeakerMapMode::Direct2D)
		, AttenuationDistance(0.0f)
		, ListenerToSoundDistance(0.0f)
		, ListenerToSoundDistanceForPanning(0.0f)
		, AbsoluteAzimuth(0.0f)
		, SoundRack(nullptr)
		//, ReverbSendMethod(EReverbSendMethod::Linear)
		//, ReverbSendLevelRange(0.0f, 0.0f)
		//, ReverbSendLevelDistanceRange(0.0f, 0.0f)
		//, ManualReverbSendLevel(0.0f)
		, StereoSpread(0.0f)
		, SpatializationMethod(EAtomSpatializationAlgorithm::Default)
		, bSpatializationIsExternalSend(false)
		//, SpatializationPluginSettings(nullptr)
		//, OcclusionPluginSettings(nullptr)
		//, ReverbPluginSettings(nullptr)
		, SourceDataOverridePluginSettings(nullptr)
		, CueNextBlockIndex(INDEX_NONE)
		, CueBeatSyncOffset(0)
		//, SourceEffectChain(nullptr)
		, LowPassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
		, HighPassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
		, AttenuationLowpassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
		, AttenuationHighpassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
		, OcclusionFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
		, AmbientZoneFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
		, bEnableSourceBusSends(false)
		, bEnableSoundRack(false)
		, bEnableSoundBusSends(false)
		, bEnableSourceDataOverride(false)
		, bUseSpatialization(false)
		, bLooping(false)
		, bEnableLowPassFilter(false)
		, bEnableHighPassFilter(false)
		, bEnableAmplitudeEnvelope(false)
		, bIsOccluded(false)
		, bIsPaused(false)
		, bEnableRetrigger(false)
		, bApplyNormalizationToStereoSounds(false)
	{
	}
};

struct CRIWARECORE_API FAtomActiveSound
{
public:

	FAtomActiveSound();
	FAtomActiveSound(const FAtomActiveSound&);
	~FAtomActiveSound();

	static FAtomActiveSound* CreateVirtualCopy(const FAtomActiveSound& ActiveSoundToCopy, FAtomRuntime& AtomRuntime);
	void SetVirtualized(bool bIsVirtualized);
	void SetCueBlockIndex(int32 BlockIndex);
	void SetCueBeatSyncInfo(const TSharedPtr<const FAtomBeatSyncInfo>& BeatSyncInfo);
	void OnCueBeatSync(const TSharedPtr<const FAtomBeatSyncInfo>& BeatSyncInfo);
	void OnCueSequencerEvent(const TSharedPtr<const FAtomSequencerEventInfo>& EventInfo);

private:

	TWeakObjectPtr<UWorld> World;
	uint32 WorldID;

	TObjectPtr<UAtomSoundBase> Sound;
	//TObjectPtr<USoundEffectSourcePresetChain> SourceEffectChain;
    TObjectPtr<UAtomAttenuation> SoundAttenuation;

	uint64 AtomComponentID;
	FName AtomComponentUserID;
	uint32 OwnerID;

	FName AtomComponentName;
	FName OwnerName;

	uint32 PlayOrder;

	FAtomRuntime* AtomRuntime;

public:

	uint32 GetObjectId() const { return Sound ? Sound->GetUniqueID() : INDEX_NONE; }
	int32 GetPlayCount() const;
	uint32 GetPlayOrder() const { return PlayOrder; }
	bool IsPreviewSound() const { return bIsPreviewSound; }
	void Stop();

	/** Returns a unique identifier for this active sound object */
	uint32 GetInstanceID() const { return PlayOrder; }

	uint64 GetAtomComponentID() const { return AtomComponentID; }
	FName GetAtomComponentUserID() const { return AtomComponentUserID; }
	void ClearAtomComponent();
	void SetAtomComponent(const FAtomActiveSound& ActiveSound);
	void SetAtomComponent(const UAtomComponent& Component);
	void SetOwner(const AActor* Owner);
	FString GetAtomComponentName() const;
	FString GetOwnerName() const;

	uint32 GetWorldID() const { return WorldID; }
	TWeakObjectPtr<UWorld> GetWeakWorld() const { return World; }
	UWorld* GetWorld() const { return World.Get(); }
	void SetWorld(UWorld* World);

	void SetPitch(float Value);
	void SetVolume(float Value);

	float GetPitch() const { return PitchMultiplier; }

	/** Gets volume product all gain stages pertaining to active sound */
	float GetVolume() const;

	UAtomSoundBase* GetSound() const { return Sound; }
	void SetSound(UAtomSoundBase* InSound);

	//USoundEffectSourcePresetChain* GetSourceEffectChain() const { return SourceEffectChain ? SourceEffectChain : ToRawPtr(Sound->SourceEffectChain); }
	//void SetSourceEffectChain(USoundEffectSourcePresetChain* InSourceEffectChain);

	void SetSoundClass(UAtomSoundClass* SoundClass);
    
    void SetAttenuationSettingsAsset(TObjectPtr<UAtomAttenuation> InSoundAttenuation);

    void SetAttenuationSettingsOverride(bool bInIsAttenuationSettingsOverridden);

	void SetAtomRuntime(FAtomRuntime* InAtomRuntime) { AtomRuntime = InAtomRuntime; }
	FAtomRuntime* GetAtomRuntime() const { return AtomRuntime; }

	/*void SetSourceListener(FSharedISourceBufferListenerPtr InListener, bool bShouldZeroBuffer)
	{
		SourceBufferListener = InListener;
		bShouldSourceBufferListenerZeroBuffer = bShouldZeroBuffer;
	}*/

	int32 GetClosestListenerIndex() const { return ClosestListenerIndex; }

	/** Returns whether or not the active sound can be deleted. */
	bool CanDelete() const { return !bAsyncOcclusionPending; }

	/** Whether or not the active sound is a looping sound. */
	bool IsLooping() const { return Sound && !Sound->IsOneShot() && PcmFilterFunction == nullptr; }

	/** Whether or not the active sound a one-shot sound. */
	bool IsOneShot() const { return !IsLooping(); }

	/** Whether or not the active sound is currently playing audible sound. */
	bool IsPlayingAudio() const { return bIsPlayingAudio; }

	/** Whether or not sound reference is valid and continue to play if silent. */
	bool IsPlayWhenSilent() const;

	/** Whether or not the sound reference is valid and set to use virtual voice when silent. */
	bool CanVirtualizeWhenSilent() const;

	/** The concurrent groups that this sound is actively playing in. */
	TMap<FAtomConcurrencyGroupId, FAtomConcurrencySoundData> ConcurrencyGroupData;

	/** Optional UAtomConcurrency to override for the sound. */
	TSet<TObjectPtr<UAtomConcurrency>> ConcurrencySet;


	TSharedPtr<Atom::FAtomExPlayer, ESPMode::ThreadSafe> InstancePlayer;

private:

	/** Optional SoundClass to override for the sound. */
	TObjectPtr<UAtomSoundClass> SoundClassOverride;

	/** Optional override the rack sends for the sound. */
	TArray<FAtomSoundToRackSend> AtomRackSendsOverride;

	/** Optional override the rack bus sends for the sound. */
	TArray<FAtomSoundToBusSend> AtomBusSendsOverride;

	/** Optional override for the source bus sends for the sound. */
	TArray<FAtomSoundSourceBusSendInfo> SourceBusSendsOverride[(int32)EAtomBusSendStage::Count];
public: // temp
	/** Optional override the AISAC modulations. */
	TArray<FAtomAisacParameter> AisacParametersOverride;
private:
	TMap<UPTRINT, FAtomPlaybackInstance*> PlaybackInstances;

public:

	enum class EFadeOut : uint8
	{
		// Sound is not currently fading out
		None,

		// Client code (eg. AtomComponent) is requesting a fade out
		User,

		// The concurrency system is requesting a fade due to voice stealing
		Concurrency
	};

	/** Whether or not the active sound will request a provided AtomSource object instead of a regular AtomRuntime sound source (pool of FAtomExPlayer). */

	// deprecated
	uint8 bForceUseAtomPlayer : 1;

	/** Whether or not the sound has checked if it was occluded already. Used to initialize a sound as occluded and bypassing occlusion interpolation. */
	uint8 bHasCheckedOcclusion : 1;

	/** Is this sound allowed to be spatialized? */
	uint8 bAllowSpatialization : 1;

	/** Does this sound have attenuation settings specified. */
	uint8 bHasAttenuationSettings : 1;

	/** Whether the playback instances should remain active if they're dropped by the prioritization code. Useful for e.g. vehicle sounds that shouldn't cut out. */
	uint8 bShouldRemainActiveIfDropped : 1;

	/** Whether the current component has finished playing */
	uint8 bFinished : 1;

	/** Whether or not the active sound is paused. Independently set vs global pause or unpause. */
	uint8 bIsPaused : 1;

	/** Whether or not to stop this active sound due to max concurrency. */
	uint8 bShouldStopDueToMaxConcurrency : 1;

	/** Whether or not sound is actually virtualized due to max concurrency. */
	uint8 bVirtualizedDueToMaxConcurrency : 1;

	/** Whether or not sound is actually fadein due concurrency realizing. */
	uint8 bIsRealizingWithFadeIn : 1;

	/** Whether or not sound is actually virtualizing by ADX. */
	uint8 bIsPendingVirtualization : 1;

	/** Whether or not sound has been virtualized and then realized */
	uint8 bHasVirtualized : 1;

	/** If true, the decision on whether to apply the radio filter has been made. */
	//uint8 bRadioFilterSelected : 1;

	/** If true, this sound will not be stopped when flushing the audio device. */
	//uint8 bApplyRadioFilter : 1;

	/** Whether the Location of the component is well defined */
	uint8 bLocationDefined : 1;

	/** If true, this sound will not be stopped when flushing the Atom runtime. */
	uint8 bIgnoreForFlushing : 1;

	/** Whether to artificially prioritize the component to play */
	uint8 bAlwaysPlay : 1;

	/** Whether or not this sound plays when the game is paused in the UI */
	uint8 bIsUISound : 1;

	/** Whether or not this audio component is a music clip */
	//uint8 bIsMusic : 1;

	/** Whether or not the audio component should be excluded from reverb EQ processing */
	//uint8 bReverb : 1;

	/** Whether or not this sound class forces sounds to the center channel */
	//uint8 bCenterChannelOnly : 1;

	/** Whether or not this active sound is a preview sound */
	uint8 bIsPreviewSound : 1;

	/** Whether we have queried for the interior settings at least once */
	uint8 bGotInteriorSettings : 1;

	/** Whether some part of this sound will want interior sounds to be applied */
	uint8 bApplyInteriorVolumes : 1;

#if !(NO_LOGGING || UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** For debugging purposes, output to the log once that a looping sound has been orphaned */
	uint8 bWarnedAboutOrphanedLooping : 1;
#endif

	/** Whether or not we have a low-pass filter enabled on this active sound. */
	uint8 bEnableLowPassFilter : 1;

	/** Whether or not we have a low-pass filter enabled on this active sound. */
	uint8 bEnableHighPassFilter : 1;

	/* Whether we have enabled amplitude envelope of this active sound. */
	uint8 bEnableAmplitudeEnvelope : 1;

	/** Whether or not this active sound will update play percentage. Based on set delegates on Atom component. */
	uint8 bUpdatePlayPercentage : 1;

	/** Whether or not this active sound will update the envelope value of every playback instance that plays a sound source. Based on set delegates on Atom component. */
	uint8 bUpdateSingleEnvelopeValue : 1;

	/** Whether or not this active sound will update the average envelope value of every playback instance that plays a sound source. Based on set delegates on Atom component. */
	uint8 bUpdateMultiEnvelopeValue : 1;

	/** Whether or not the active sound should update it's owning Atom component's playback time. */
	uint8 bUpdatePlaybackTime : 1;

	/** Whether or not this active sound is playing audio, as in making audible sounds. */
	uint8 bIsPlayingAudio : 1;

	/** Whether or not the active sound is stopping. */
	uint8 bIsStopping : 1;

	/** Whether or not we are overriding the routing enablement options on sounds. */
	uint8 bHasActiveSourceBusSendRoutingOverride : 1;
	uint8 bHasActiveMainRackOutputOverride : 1;
	uint8 bHasActiveSoundBusSendRoutingOverride : 1;

	/** What the value of the enablement overrides are. */
	uint8 bEnableSourceBusSendRoutingOverride : 1;
	uint8 bEnableMainRackOutputOverride : 1;
	uint8 bEnableSoundBusSendRoutingOverride : 1;

	uint8 bIsFirstAttenuationUpdate : 1;
	uint8 bStartedWithinNonBinauralRadius : 1;

	uint8 bModulationRoutingUpdated : 1;
    
    /** If this is true the active sound uses the overridden struct of the sound not the attenuation settings asset. */
    uint8 bIsAttenuationSettingsOverridden : 1;

	uint8 UserIndex;

	/** Type of fade out currently being applied */
	EFadeOut FadeOut;

	/** whether we were occluded the last time we checked */
	FThreadSafeBool bIsOccluded;

	/** Whether or not there is an async occlusion trace pending */
	FThreadSafeBool bAsyncOcclusionPending;

	/** Duration between now and when the sound has been started. */
	float PlaybackTime;

	/** If virtualized, duration between last time virtualized and now. */
	float PlaybackTimeNonVirtualized;

	float MinCurrentPitch;
	float RequestedStartTime;

	float VolumeMultiplier;
	float PitchMultiplier;

	/** The low-pass filter frequency to apply if bEnableLowPassFilter is true. */
	float LowPassFilterFrequency;

	/** The high-pass filter frequency to apply if bEnableHighPassFilter is true. */
	float HighPassFilterFrequency;

	/** An envelope to apply to volume amplitude. */
	FAtomEnvelope AmplitudeEnvelope;

	/** An effect to apply to voices. */
	FAtomSourceVoiceEffect SourceVoiceEffect;

	/** Fader that tracks component volume */
	FAtomVolumeFader ComponentVolumeFader;

	/** The interpolated parameter for the low-pass frequency due to occlusion. */
	FAtomDynamicParameter CurrentOcclusionFilterFrequency;

	/** The interpolated parameter for the volume attenuation due to occlusion. */
	FAtomDynamicParameter CurrentOcclusionVolumeAttenuation;

	/** The product of the component priority and the UAtomSoundBase priority */
	float Priority;

	/** The volume used to determine concurrency resolution for "quietest" active sound.
	 * If negative, tracking is disabled for lifetime of ActiveSound.
	 */
	float VolumeConcurrency;

	/** The time in seconds with which to check for occlusion from its closest listener */
	float OcclusionCheckInterval;

	/** Last time we checked for occlusion. */
	float LastOcclusionCheckTime;

	/** The max distance this sound will be audible. */
	float MaxDistance;

	FTransform Transform;

	/**
	 * Cached data pertaining to focus system updated each frame
	 */
	FAtomAttenuationFocusData FocusData;

	 /** Location last time playback was updated. */
	FVector LastLocation;

	/** Multi-Position type */
	EAtomMultiPositionType MultiPositionType;

	/** Multi-Position locations and directions. */
	TArray<FTransform> MultiPositions;

	/** The Attenuation setting to apply. (only works for ExPlayer) */
	FAtomAttenuationSettings AttenuationSettings;

	/** Quantization information */
	Atom::FQuartzQuantizedRequestData QuantizedRequestData;

	/** Source buffer listener */
	//FSharedISourceBufferListenerPtr SourceBufferListener;
	//bool bShouldSourceBufferListenerZeroBuffer = false;

	// Envelope follower attack and release time parameters
	int32 EnvelopeFollowerAttackTime;
	int32 EnvelopeFollowerReleaseTime;

	// Whether or not there are Source Bus Sends that have not been sent to the render thread
	bool bHasNewSourceBusSends;

	// Bus send(s) that have not yet been sent to the render thread
	TArray<TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>> NewSourceBusSends;

	/** Optional Atom Categories to use. (Note: Not exposed by UAtomComponent, acessible only by c++.) */
	TArray<FName> CategoryNames;

	/** Optional AISAC to attach. */
	TArray<FName> AdditionalAisacPatchNames;

	/** Modulation and AISAC control routings. */
	FAtomSoundModulationRoutingSettings ModulationRouting;

	/** Helper that notifies when this active sound finished to play. */
	TFunction<void(bool bWasEndReached)> OnSoundFinished;

	/** The thread that will be used to call the function in OnSoundFinished. */
	ENamedThreads::Type ThreadToCallOnSoundFinishedOn;

	/** If set, enables pcm data filter and call this function at each frame. */
	FAtomPcmFilterFunction PcmFilterFunction;

	/** When a Soundcue is used as sound, this defines the selector to use. Possible selectors are defined by the currently used AtomConfig. */
	TArray<FAtomSelectorParam> CueSelectorParams;

	/** When a Soundcue is used as sound, this defines the index of the first block to playback when sound starts. */
	int32 CueFirstBlockIndex;

	/** When a Soundcue is used as sound, this defines the next index to setup, this value is reset eachtime it is applied to playback.*/
	int32 CueNextBlockIndex;

	/** */
	int32 CueBeatSyncOffset;

	/** If set, overrides standard mixer source creation and create an unpooled mixer source. */
	TFunction<FAtomSource* (FAtomRuntime*, FAtomPlaybackInstance*)> OnCreateSource;

#if ENABLE_ATOM_DEBUG
	FColor DebugColor;
#endif // ENABLE_ATOM_DEBUG

public:

	//void UpdateInterfaceParameters(const TArray<FListener>& InListeners);

	// Updates the playback instances to be played.
	void UpdatePlaybackInstances(TArray<FAtomPlaybackInstance*>& OutPlaybackInstances, const float DeltaTime);

	/** Find an existing playback instance attached (if any) */
	FAtomPlaybackInstance* FindPlaybackInstance(const UPTRINT PlaybackInstanceHash);

	void RemovePlaybackInstance(const UPTRINT PlaybackInstanceHash);

	const TMap<UPTRINT, FAtomPlaybackInstance*>& GetPlaybackInstances() const { return PlaybackInstances; }

	/** Add newly created playback instance to active sound */
	FAtomPlaybackInstance& AddPlaybackInstance(const UPTRINT PlaybackInstanceHash);

	/**
	 * Check whether to apply the radio filter
	 */
	//void ApplyRadioFilter(const FSoundParseParameters& ParseParams);

	/** Gets total concurrency gain stage based on all concurrency memberships of sound */
	float GetTotalConcurrencyVolumeScale() const;

	void CollectAttenuationShapesForVisualization(TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap) const;

	/**
	 * Friend archive function used for serialization.
	 */
	friend FArchive& operator<<(FArchive& Ar, FAtomActiveSound* ActiveSound);

	void AddReferencedObjects(FReferenceCollector& Collector);

	/**
	 * Get the sound class to apply on this sound instance
	 */
	UAtomSoundClass* GetSoundClass() const;

	/**
	 * Get the sound rack to use for this sound instance
	 */
	UAtomRackBase* GetSoundRack() const;

	/** Sets the amount of audio from this active sound to send to the rack. */
	void SetAtomRackSend(const FAtomSoundToRackSend& RackSend);

	/** Sets the amount of audio from this active sound to send to the rack bus. */
	void SetAtomBusSend(const FAtomSoundToBusSend& BusSend);

	/** Gets the rack sends to use for this sound instance. */
	void GetAtomRackSends(TArray<FAtomSoundToRackSend>& OutSends) const;

	/** Gets the rack bus sends to use for this sound instance. */
	void GetAtomBusSends(TArray<FAtomSoundToBusSend>& OutSends) const;

	/** Sets the amount of audio from this active sound to send to the source bus. */
	void SetSourceBusSend(EAtomBusSendStage BusSendStage, const FAtomSoundSourceBusSendInfo& SourceBusSendInfo);

	/** Gets the sound source bus sends to use for this sound instance. */
	void GetSourceBusSends(EAtomBusSendStage BusSendStage, TArray<FAtomSoundSourceBusSendInfo>& OutSends) const;

	/**
	 * Checks whether there are Source Bus Sends that have not yet been updated
	 * @return true when there are new Source Bus Sends, false otherwise
	 */
	bool HasNewSourceBusSends() const;

	/** Lets the audio thread know if additional Source Bus Send information has been added
	*
	*  @return the array of Sound Bus Sends that have not yet been added to the render thread
	*/
	TArray<TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>> const& GetNewSourceBusSends() const;

	/** Resets internal data of new Source Bus Sends */
	void ResetNewSourceBusSends();

	/* Gives new Modulation Routing settings to the Active Sound. */
	void SetNewModulationRouting(const FAtomSoundModulationRoutingSettings& NewRouting);

	/**
	 * Sets multiple source postions.
	 * When MultiPositionType is set to SingleSource, sound transform is user or first position if passed list is not empty.
	 */
	void SetMultiplePositions(const TArray<FTransform>& InPositions, EAtomMultiPositionType InMultiPositionType);

	/**
	 * Gets the multiple source positions in use.
	 */
	void GetMultiplePositions(TArray<FTransform>& OutPositions);

	/** Sets the value of an AISAC control to use for this sound instance. */
	void SetAisacControl(const FAtomAisacParameter& Parameter);

	/** Gets the AISAC control with values set for this sound instance. */
	void GetAisacControls(TArray<FAtomAisacParameter>& OutParameters) const;

	/** Cues parameters */
	void SetCueSelectorLabel(const FAtomSelectorParam& SelectorParam);

	void RemoveCueSelectorLabel(const FAtomSelectorParam& SelectorParam);

	void ClearAllCueSelectorLabels();

	void GetCueSelectorParameters(TArray<FAtomSelectorParam>& OutParameters) const;

	void SetCueNextBlockIndex(int32 BlockIndex);

	void SetCueBeatSyncOffset(int32 Offset);

	/* Determines which of the provided listeners is the closest to the sound */
	int32 FindClosestListener(const TArray<struct FAtomListener>& InListeners) const;

	/* Determines which listener is the closest to the sound */
	int32 FindClosestListener() const;

	/** Returns the unique ID of the active sound's owner if it exists. Returns 0 if the sound doesn't have an owner. */
	uint32 GetOwnerID() const { return OwnerID; }

	/** Gets the sound concurrency handles applicable to this sound instance. */
	void GetConcurrencyHandles(TArray<FAtomConcurrencyHandle>& OutConcurrencyHandles) const;

	/** */
	bool GetConcurrencyFadeDuration(float& OutFadeDuration) const;
	
	/** Delegate callback function when an async occlusion trace completes. */
	static void OcclusionTraceDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);

	/** Applies the active sound's attenuation settings to the input parse params using the given listener. */
	void ParseAttenuation(FAtomSoundParseParameters& OutParseParams, int32 ListenerIndex, const FAtomAttenuationSettings& InAttenuationSettings);

	/** Returns whether or not sound or any active playback instances it manages are set to always play. */
	bool GetAlwaysPlay() const;

	/** Returns the highest effective priority of the child wave instances. If bIgnoreAlwaysPlay set to true, gives highest
	  * priority disregarding always play priority override.
	  */
	float GetHighestPriority(bool bIgnoreAlwaysPlay = false) const;

	/** Updates the active sound's attenuation settings to the input parse params using the given listener */
	void UpdateAttenuation(float DeltaTime, FAtomSoundParseParameters& ParseParams, int32 ListenerIndex, const FAtomAttenuationSettings* SettingsAttenuationNode = nullptr);

	/** Updates the provided focus data using the local */
	void UpdateFocusData(float DeltaTime, const FAtomAttenuationListenerData& ListenerData, FAtomAttenuationFocusData* OutFocusData = nullptr);
	
	// Calls OnSoundFinished on current thread if CallbackThread == ENamedThreads::AnyThread, and dispatchs an async task on a named thread otherwise.
	static void ExecuteOnSoundFinishedCallback(bool bEndReached, const TFunction<void(bool bEndReached)>& OnSoundFinished, const ENamedThreads::Type& CallbackThread);

private:

	struct FAsyncTraceDetails
	{
		FAtomRuntimeId AtomRuntimeID;
		FAtomActiveSound* ActiveSound;
	};

	static TMap<FTraceHandle, FAsyncTraceDetails> TraceToActiveSoundMap;

	static FTraceDelegate ActiveSoundTraceDelegate;

	/** Cached index to the closest listener. So we don't have to do the work to find it twice. */
	int32 ClosestListenerIndex;

	/** This is a friend so the Atom runtime can call Stop() on the active sound. */
	friend class FAtomRuntime;

	/**
	 * Marks the active sound as pending delete and begins termination of internal resources.
	 * Only to be called from the owning audio device.
	 */
	void MarkPendingDestroy(bool bDestroyNow);

	/** Whether or not the active sound is stopping. */
	bool IsStopping() const { return bIsStopping; }

	/** Called when an active sound has been stopped but needs to update it's stopping sounds. Returns true when stopping sources have finished stopping. */
	bool UpdateStoppingSources(uint64 CurrentTick, bool bEnsureStopped);

	/** Updates ramping concurrency volume scalars */
	void UpdateConcurrencyVolumeScalars(const float DeltaTime);

	/** if OcclusionCheckInterval > 0.0, checks if the sound has become (un)occluded during playback
	 * and calls eventOcclusionChanged() if so
	 * primarily used for gameplay-relevant ambient sounds
	 * CurrentLocation is the location of this component that will be used for playback
	 * @param ListenerLocation location of the closest listener to the sound
	 */
	void CheckOcclusion(const FVector ListenerLocation, const FVector SoundLocation, const FAtomAttenuationSettings* AttenuationSettingsPtr);

	/** Helper function which retrieves attenuation frequency value for HPF and LPF distance-based filtering. */
	float GetAttenuationFrequency(const FAtomAttenuationSettings* InSettings, const FAtomAttenuationListenerData& ListenerData, const FVector2D& FrequencyRange, const FRuntimeFloatCurve& CustomCurve);
};
