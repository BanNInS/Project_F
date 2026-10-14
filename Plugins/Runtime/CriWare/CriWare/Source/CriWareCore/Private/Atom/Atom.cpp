/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : Atom.cpp
 *
 ****************************************************************************/

#include "Atom/Atom.h"

#include "CriWare.h"
#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "CriWareUtils.h"
#include "Atom/AtomPluginUtilities.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomSourceVoiceEffect.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"
#include "Atom/AtomSoundClass.h"

#if WITH_EDITOR
#include "CriWareCoreEditorModule.h"
#endif

// Log Categories
DEFINE_LOG_CATEGORY(LogCriWareAtom);
DEFINE_LOG_CATEGORY(LogCriWareAtomDebug);

// Stats
DEFINE_STAT(STAT_AtomPlaybackInstances);
DEFINE_STAT(STAT_AtomActiveSounds);
DEFINE_STAT(STAT_AtomVirtualLoops);
DEFINE_STAT(STAT_AtomSources);
DEFINE_STAT(STAT_AtomPlaybacksDroppedDueToPriority);
DEFINE_STAT(STAT_AtomEvaluateConcurrency);
DEFINE_STAT(STAT_AtomAudiblePlaybacksDroppedDueToPriority);
DEFINE_STAT(STAT_AtomMaxChannels);
DEFINE_STAT(STAT_AtomMaxReservedSources);
DEFINE_STAT(STAT_AtomMemorySize);
DEFINE_STAT(STAT_AtomGatherPlaybackInstances);
DEFINE_STAT(STAT_AtomStartSources);
DEFINE_STAT(STAT_AtomUpdateSources);
DEFINE_STAT(STAT_AtomSourceInitTime);
DEFINE_STAT(STAT_AtomSoundFinishedDelegatesCalled);
DEFINE_STAT(STAT_AtomSoundFinishedDelegates);
DEFINE_STAT(STAT_AtomFindNearestLocation);

/** CVars */
static int32 AtomDisableStereoSpreadCvar = 0;
FAutoConsoleVariableRef CVarAtomDisableStereoSpread(
	TEXT("atom.DisableStereoSpread"),
	AtomDisableStereoSpreadCvar,
	TEXT("When set to 1, ignores the 3D Stereo Spread property in attenuation settings and instead renders audio from a singular point.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);

static int32 AtomAllowAudioSpatializationCVar = 1;
FAutoConsoleVariableRef CVarAtomAllowAudioSpatializationCVar(
	TEXT("atom.AllowAudioSpatialization"),
	AtomAllowAudioSpatializationCVar,
	TEXT("Controls if we allow spatialization of audio, normally this is enabled. If disabled all audio won't be spatialized, but will have attenuation.\n")
	TEXT("0: Disable, >0: Enable"),
	ECVF_Default);

static int32 AtomBypassPlayWhenSilentCVar = 0;
FAutoConsoleVariableRef CVarAtomBypassPlayWhenSilent(
	TEXT("atom.BypassPlayWhenSilent"),
	AtomBypassPlayWhenSilentCVar,
	TEXT("When set to 1, ignores the Play When Silent flag for non-procedural sources.\n")
	TEXT("0: Honor the Play When Silent flag, 1: stop all silent non-procedural sources."),
	ECVF_Default);

static int32 AtomOcclusionFilterScaleEnabledCVar = 0;
FAutoConsoleVariableRef CVarAtomOcclusionFilterScaleEnabled(
	TEXT("atom.EnableOcclusionFilterScale"),
	AtomOcclusionFilterScaleEnabledCVar,
	TEXT("Whether or not we scale occlusion by 0.25f to compensate for change in filter cutoff frequencies in Atom mixer.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

namespace AtomModulationPrivate
{
	static const FLazyName VolumeParameterName("Volume");
	static const FLazyName PitchParameterName("Pitch");
	static const FLazyName HighpassParameterName("HPFCutoffFrequency");
	static const FLazyName LowpassParameterName("LPFCutoffFrequency");
	static const FLazyName AisacParameterName("AISAC");
}

UClass* GetAtomPluginCustomSettingsClass(EAtomPlugin PluginType)
{
	switch (PluginType)
	{
	case EAtomPlugin::SourceDataOverride:
	{
		if (IAtomSourceDataOverrideFactory* Factory = AtomPluginUtilities::GetDesiredSourceDataOverridePlugin())
		{
			return Factory->GetCustomSourceDataOverrideSettingsClass();
		}
	}
	break;
	case EAtomPlugin::Runtime:
	{
		return nullptr; // per plugins
	}
	case EAtomPlugin::Mana:
	{
		return nullptr; // none
	}
	break;
	default:
		static_assert(static_cast<uint32>(EAtomPlugin::Count) == 3, "Possible missing Atom plugin type case coverage");
		break;
	}

	return nullptr;
}

TArray<UClass*> GetAtomRuntimePluginCustomSettingsClasses()
{
	TArray<UClass*> Classes;

	TArray<IAtomRuntimePluginFactory*> RuntimePluginFactories = AtomPluginUtilities::GetAllDesiredAtomRuntimePlugins();
	for (IAtomRuntimePluginFactory* Factory : RuntimePluginFactories)
	{
		Classes.Add(Factory->GetCustomRuntimeSettingsClass());
	}

	return Classes;
}

/*
 * FAtomNotifyPlaybackFinishedHooks
 *****************************************************************************/

void FAtomNotifyPlaybackFinishedHooks::AddNotify(TScriptInterface<IAtomPlaybackFinishedNotifiedObject> NotifyObject, UPTRINT PlaybackInstanceHash)
{
	Notifies.Add(FNotifyPlaybackDetails(NotifyObject, PlaybackInstanceHash));
}

UPTRINT FAtomNotifyPlaybackFinishedHooks::GetHashForSound(TScriptInterface<IAtomPlaybackFinishedNotifiedObject> NotifyObject) const
{
	for (const FNotifyPlaybackDetails& NotifyDetails : Notifies)
	{
		if (NotifyDetails.NotifyObject == NotifyObject)
		{
			return NotifyDetails.NotifyPlaybackInstanceHash;
		}
	}

	return 0;
}

void FAtomNotifyPlaybackFinishedHooks::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (FNotifyPlaybackDetails& NotifyDetails : Notifies)
	{
		Collector.AddReferencedObject(NotifyDetails.NotifyObject.GetObjectRef());
	}
}

void FAtomNotifyPlaybackFinishedHooks::DispatchNotifies(FAtomPlaybackInstance* PlaybackInstance, const bool bStopped)
{
	for (int32 NotifyIndex = Notifies.Num() - 1; NotifyIndex >= 0; --NotifyIndex)
	{
		// All objects get an opportunity to handle the notify if we're forcefully stopping the sound
		if (Notifies[NotifyIndex].NotifyObject)
		{
			if (Notifies[NotifyIndex].NotifyObject->NotifyPlaybackInstanceFinished(PlaybackInstance) && !bStopped)
			{
				break;
			}
		}
	}
}

FArchive& operator<<(FArchive& Ar, FAtomNotifyPlaybackFinishedHooks& NotifyHook)
{
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		for (FAtomNotifyPlaybackFinishedHooks::FNotifyPlaybackDetails& NotifyDetails : NotifyHook.Notifies)
		{
			//Ar << Cast<UAtomPlaybackFinishedNotifiedObject>(NotifyDetails.NotifyObject);
		}
	}
	return Ar;
}

/*
 * FAtomPlaybackInstance
 *****************************************************************************/

 /** Helper to create good play order for FAtomPlaybackInstance instances */
uint32 FAtomPlaybackInstance::PlayOrderCounter = 0;

FAtomPlaybackInstance::FAtomPlaybackInstance(const UPTRINT InPlaybackInstanceHash, FAtomActiveSound& InActiveSound)
	: SoundData(nullptr)
	, SoundClass(nullptr)
	, SoundRack(nullptr)
	//, SourceEffectChain(nullptr)
	, ActiveSound(&InActiveSound)
	, Volume(0.0f)
	, DistanceAttenuation(1.0f)
	, OcclusionAttenuation(1.0f)
	, VolumeMultiplier(1.0f)
	, EnvelopValue(0.0f)
	, RelativeRenderCost(1.0f)
	, EnvelopeFollowerAttackTime(10)
	, EnvelopeFollowerReleaseTime(100)
	, Priority(1.0f)
	//, VoiceCenterChannelVolume(0.0f)
	//, RadioFilterVolume(0.0f)
	//, RadioFilterVolumeThreshold(0.0f)
	//, LFEBleed(0.0f)
	, LoopingMode(EAtomLoopingMode::LoopNever)
	, StartTime(-1.0f)
	, bEnableSourceBusSends(ActiveSound->bEnableSourceBusSendRoutingOverride)
	, bEnableSoundRack(ActiveSound->bEnableMainRackOutputOverride)
	, bEnableSoundBusSends(ActiveSound->bEnableSoundBusSendRoutingOverride)
	, bEnableSourceDataOverride(false)
	//, bApplyRadioFilter(false)
	, bIsStarted(false)
	, bIsFinished(false)
	, bAlreadyNotifiedHook(false)
	, bUseSpatialization(false)
	, bEnableAmplitudeEnvelope(false)
	, bEnableLowPassFilter(false)
	, bEnableHighPassFilter(false)
	, bIsOccluded(false)
	, bIsUISound(false)
	//, bIsMusic(false)
	//, bReverb(true)
	, bCenterChannelOnly(false)
	, bIsPaused(false)
	, bReportedSpatializationWarning(false)
	, bIsAmbisonics(false)
	, bIsStopping(false)
	, SpatializationMethod(EAtomSpatializationAlgorithm::Default)
	//, SpatializationPluginSettings(nullptr)
	//, OcclusionPluginSettings(nullptr)
	//, ReverbPluginSettings(nullptr)
	, SourceDataOverridePluginSettings(nullptr)
	, CueNextBlockIndex(INDEX_NONE)
	, CueBeatSyncOffset(0)
	//, OutputTarget(EAudioOutputTarget::Speaker)
	, LowPassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, HighPassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
	, SoundClassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, SoundClassHighPassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
	, OcclusionFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, AmbientZoneFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, AttenuationLowpassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, AttenuationHighpassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
	, Pitch(0.0f)
	, Transform(FTransform::Identity)
	, NonSpatializedRadiusStart(0.0f)
	, NonSpatializedRadiusEnd(0.0f)
	, NonSpatializedRadiusMode(EAtomNonSpatializedRadiusSpeakerMapMode::Direct2D)
	, StereoSpread(0.0f)
	, AttenuationDistance(0.0f)
	, ListenerToSoundDistance(0.0f)
	, ListenerToSoundDistanceForPanning(0.0f)
	, AbsoluteAzimuth(0.0f)
	, PlaybackTime(0.0f)
	//, ReverbSendLevel(0.0f)
	, PlayOrder(0)
	, PlaybackInstanceHash(InPlaybackInstanceHash)
	, UserIndex(0)
{
	PlayOrder = ++PlayOrderCounter;
}

bool FAtomPlaybackInstance::IsPlaying() const
{
	check(ActiveSound);

	if (!SoundData)
	{
		return false;
	}

	// TODO: move out of audio.  Subtitle system should be separate and just set VirtualizationMode to PlayWhenSilent
	/*const bool bHasSubtitles = ActiveSound->bHandleSubtitles && (ActiveSound->bHasExternalSubtitles || WaveData->Subtitles.Num() > 0);
	if (bHasSubtitles)
	{
		return true;
	}*/

	if (ActiveSound->IsPlayWhenSilent() && (!AtomBypassPlayWhenSilentCVar || SoundData->bProcedural))
	{
		return true;
	}

	const float PlaybackInstanceVolume = Volume * VolumeMultiplier * GetDistanceAndOcclusionAttenuation() * GetDynamicVolume();
	if (PlaybackInstanceVolume > UE_KINDA_SMALL_NUMBER)
	{
		return true;
	}

	if (ActiveSound->ComponentVolumeFader.IsFadingIn())
	{
		return true;
	}

	return false;
}

/**
 * Notifies the playback instance that it has finished.
 */
void FAtomPlaybackInstance::NotifyFinished(const bool bStopped /* = false */)
{
	if (!bAlreadyNotifiedHook)
	{
		// Can't have a source finishing that hasn't started
		if (!bIsStarted)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Received finished notification from PlaybackInstance that hasn't started!"));
		}

		// We are finished.
		bIsFinished = true;

		// Avoid double notifications.
		bAlreadyNotifiedHook = true;

		NotifyPlaybackFinishedHooks.DispatchNotifies(this, bStopped);

		// notify also the Activesound finish callback if exist
		if (ActiveSound && ActiveSound->OnSoundFinished)
		{
			ActiveSound->ExecuteOnSoundFinishedCallback(bStopped, ActiveSound->OnSoundFinished, ActiveSound->ThreadToCallOnSoundFinishedOn);
		}
	}
}

/**
 * Stops the wave instance without notifying NotifyWaveInstanceFinishedHook. This will NOT stop wave instance
 * if it is set up to loop indefinitely or set to remain active.
 */
void FAtomPlaybackInstance::StopWithoutNotification()
{
	if (LoopingMode == EAtomLoopingMode::LoopForever || ActiveSound->bShouldRemainActiveIfDropped)
	{
		// We don't finish if we're either indefinitely looping or the Atom component explicitly mandates that we should
		// remain active which is e.g. used for engine sounds and such.
		bIsFinished = false;
	}
	else
	{
		// We're finished.
		bIsFinished = true;
	}

	// Always notify ActiveSound finish callback if exist
	if (ActiveSound && ActiveSound->OnSoundFinished)
	{
		ActiveSound->ExecuteOnSoundFinishedCallback(true, ActiveSound->OnSoundFinished, ActiveSound->ThreadToCallOnSoundFinishedOn);
	}
}

FArchive& operator<<(FArchive& Ar, FAtomPlaybackInstance* PlaybackInstance)
{
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << PlaybackInstance->SoundData;
		Ar << PlaybackInstance->SoundClass;
		Ar << PlaybackInstance->NotifyPlaybackFinishedHooks;
	}
	return(Ar);
}

void FAtomPlaybackInstance::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SoundData);

	/*if (USynthSound* SynthSound = Cast<USynthSound>(WaveData))
	{
		if (USynthComponent* SynthComponent = SynthSound->GetOwningSynthComponent())
		{
			Collector.AddReferencedObject(SynthComponent);
		}
	}*/

	for (FAtomAttenuationBusSendSettings& BusSend : BusSendSettings)
	{
		if (BusSend.Bus)
		{
			Collector.AddReferencedObject(BusSend.Bus);
		}
	}

	Collector.AddReferencedObject(SoundClass);
	NotifyPlaybackFinishedHooks.AddReferencedObjects(Collector);
}

float FAtomPlaybackInstance::GetActualVolume() const
{
	// Include all volumes
	float ActualVolume = GetVolume() * GetDistanceAndOcclusionAttenuation();
	if (ActualVolume != 0.0f)
	{
		ActualVolume *= GetDynamicVolume();

		check(ActiveSound);
		if (!ActiveSound->bIsPreviewSound)
		{
			check(ActiveSound->GetAtomRuntime());
			ActualVolume *= ActiveSound->GetAtomRuntime()->GetPrimaryVolume();
		}
	}

	return ActualVolume;
}

float FAtomPlaybackInstance::GetDistanceAndOcclusionAttenuation() const
{
	return DistanceAttenuation * OcclusionAttenuation;
}

float FAtomPlaybackInstance::GetDistanceAttenuation() const
{
	return DistanceAttenuation;
}

float FAtomPlaybackInstance::GetOcclusionAttenuation() const
{
	return OcclusionAttenuation;
}

float FAtomPlaybackInstance::GetDynamicVolume() const
{
	float OutVolume = 1.0f;

	if (GCriWare)
	{
		if (FAtomRuntimeManager* AtomRuntime = GCriWare->GetAtomRuntimeManager())
		{
			//if (SoundData)
			//{
			//	OutVolume *= AtomRuntime->GetDynamicSoundVolume(EAtomSoundType::Playback, SoundData->GetFName());
			//}

			if (ActiveSound)
			{
				if (const UAtomSoundCue* Sound = Cast<UAtomSoundCue>(ActiveSound->GetSound()))
				{
					OutVolume *= AtomRuntime->GetDynamicSoundVolume(EAtomSoundType::Cue, Sound->GetFName());
				}

				if (const UAtomSoundWave* Sound = Cast<UAtomSoundWave>(ActiveSound->GetSound()))
				{
					OutVolume *= AtomRuntime->GetDynamicSoundVolume(EAtomSoundType::Wave, Sound->GetFName());
				}
			}

			if (SoundClass)
			{
				OutVolume *= AtomRuntime->GetDynamicSoundVolume(EAtomSoundType::Class, SoundClass->GetFName());
			}
		}
	}

	return OutVolume;
}

float FAtomPlaybackInstance::GetVolumeWithDistanceAndOcclusionAttenuation() const
{
	return GetVolume() * GetDistanceAndOcclusionAttenuation();
}

float FAtomPlaybackInstance::GetPitch() const
{
	return Pitch;
}

float FAtomPlaybackInstance::GetVolume() const
{
	// Only includes non-attenuation and non-app volumes
	return Volume * VolumeMultiplier;
}

float FAtomPlaybackInstance::GetTimeStretchRatio() const
{
	return SourceVoiceEffect.GetTimeStretchRatio();
}

bool FAtomPlaybackInstance::ShouldStopDueToMaxConcurrency() const
{
	check(ActiveSound);
	return ActiveSound->bShouldStopDueToMaxConcurrency 
		&& !ActiveSound->bVirtualizedDueToMaxConcurrency;
}

float FAtomPlaybackInstance::GetVolumeWeightedPriority() const
{
	// If priority has been set via bAlwaysPlay, it will have a priority larger than ATOM_MAX_SOUND_PRIORITY. If that's the case, we should ignore volume weighting.
	if (Priority > ATOM_MAX_SOUND_PRIORITY)
	{
		return Priority;
	}

	// This will result in zero-volume sounds still able to be sorted due to priority but give non-zero volumes higher priority than 0 volumes
	float ActualVolume = GetVolumeWithDistanceAndOcclusionAttenuation();
	if (ActualVolume > 0.0f)
	{
		// Only check for bypass if the actual volume is greater than 0.0
		if (SoundData)//&& SoundData->bBypassVolumeScaleForPriority)
		{
			return Priority;
		}
		else
		{
			return ActualVolume * Priority;
		}
	}
	else if (IsStopping())
	{
		// Stopping sounds will be sorted above 0-volume sounds
		return ActualVolume * Priority - ATOM_MAX_SOUND_PRIORITY - 1.0f;
	}
	else
	{
		return Priority - 2.0f * ATOM_MAX_SOUND_PRIORITY - 1.0f;
	}
}

bool FAtomPlaybackInstance::IsSeekable() const
{
	check(SoundData);

	if (StartTime == 0.0f)
	{
		return false;
	}

	return SoundData->IsSeekable();
}

bool FAtomPlaybackInstance::IsStreaming() const
{
	return SoundData != nullptr && SoundData->IsStreaming(nullptr);
}

bool FAtomPlaybackInstance::GetUseSpatialization() const
{
	return AtomAllowAudioSpatializationCVar && bUseSpatialization;
}

FString FAtomPlaybackInstance::GetName() const
{
	if (SoundData)
	{
		return SoundData->GetSoundName();
	}
	return TEXT("Null");
}

float FAtomPlaybackInstance::GetEffectiveModulationValue(const EAtomModulationDestination InDestination) const
{
	FAtomSoundModulationSettings Settings;
	float OutValue;
	CalculateInheritedModulationSettingsAndValue(InDestination, Settings, OutValue);

	return OutValue;
}

FAtomSoundModulationSettings FAtomPlaybackInstance::GetEffectiveModulationSettings(const EAtomModulationDestination InDestination) const
{
	FAtomSoundModulationSettings Settings;
	float OutValue;
	CalculateInheritedModulationSettingsAndValue(InDestination, Settings, OutValue);

	return Settings;
}

float FAtomPlaybackInstance::GetEffectiveAisacModulationValue(const FAtomAisacControl& InAisacControl) const
{
	FAtomAisacControlSettings Settings;
	float OutValue;
	CalculateInheritedModulationSettingsAndValue(InAisacControl, Settings, OutValue);

	return OutValue;
}

FAtomAisacControlSettings FAtomPlaybackInstance::GetEffectiveAisacModulationSettings(const FAtomAisacControl& InAisacControl) const
{
	FAtomAisacControlSettings Settings;
	float OutValue;
	CalculateInheritedModulationSettingsAndValue(InAisacControl, Settings, OutValue);

	return Settings;
}

Atom::FModulationParameter FAtomPlaybackInstance::GetParameterBasedOnDestination(const EAtomModulationDestination InDestination)
{
	switch (InDestination)
	{
	case EAtomModulationDestination::Volume:
		if (const Atom::FModulationParameter* Parameter = Atom::GetModulationParameterPtr(AtomModulationPrivate::VolumeParameterName))
		{
			return *Parameter;
		}
		break;
	case EAtomModulationDestination::Pitch:
		if (const Atom::FModulationParameter* Parameter = Atom::GetModulationParameterPtr(AtomModulationPrivate::PitchParameterName))
		{
			return *Parameter;
		}
		break;
	case EAtomModulationDestination::Highpass:
		if (const Atom::FModulationParameter* Parameter = Atom::GetModulationParameterPtr(AtomModulationPrivate::HighpassParameterName))
		{
			return *Parameter;
		}
		break;
	case EAtomModulationDestination::Lowpass:
		if (const Atom::FModulationParameter* Parameter = Atom::GetModulationParameterPtr(AtomModulationPrivate::LowpassParameterName))
		{
			return *Parameter;
		}
		break;
	default:
		break;
	}
	return Atom::GetDefaultModulationParameter();
}

void FAtomPlaybackInstance::CalculateInheritedModulationSettingsAndValue(const FAtomAisacControl& InAisacControl, FAtomAisacControlSettings& InOutSettings, float& OutValue) const
{
	InOutSettings.Control = InAisacControl;

	auto AsiacFindPredicate = [&](const FAtomAisacControlSettings& ControlSettings) { return InAisacControl == ControlSettings.Control; };

	const Atom::FModulationParameter* Parameter = Atom::GetModulationParameterPtr(AtomModulationPrivate::AisacParameterName);
	const Atom::FModulationParameter& InParam = Parameter ? *Parameter : Atom::GetDefaultModulationParameter();
	const FAtomActiveSound& InActiveSound = *ActiveSound;

	const FAtomAisacControlSettings* InstanceSettings = SoundData->GetModulationSettings().AisacModulations.AisacControlModulations.FindByPredicate(AsiacFindPredicate);
	const FAtomAisacControlSettings& InInstanceSettings = InstanceSettings ? *InstanceSettings : FAtomAisacControlSettings();
	const EAtomModulationRouting& InInstanceRouting = InstanceSettings ? EAtomModulationRouting::Union : EAtomModulationRouting::Inherit;

	const FAtomAisacControlSettings* ActiveSettings = ActiveSound->ModulationRouting.AisacModulations.AisacControlModulations.FindByPredicate(AsiacFindPredicate);
	const FAtomAisacControlSettings& InActiveSettings = ActiveSettings ? *ActiveSettings : FAtomAisacControlSettings();
	const EAtomModulationRouting& InActiveRouting = ActiveSettings ? EAtomModulationRouting::Union : EAtomModulationRouting::Inherit;

	const UAtomSoundClass* EffectiveSoundClass = InActiveSound.GetSoundClass();
	if (SoundClass)
	{
		EffectiveSoundClass = SoundClass;
	}

	CalculateInheritedModulationSettingsAndValueInternal(InParam,
		InActiveSettings, InActiveRouting,
		InInstanceSettings, InInstanceRouting,
		EffectiveSoundClass, [&]()
		{
			const FAtomAisacControlSettings* ClassSettings = EffectiveSoundClass->Properties.ModulationSettings.AisacModulations.AisacControlModulations.FindByPredicate(AsiacFindPredicate);
			return ClassSettings ? *ClassSettings : FAtomAisacControlSettings();
		},
		InOutSettings, OutValue);
}

void FAtomPlaybackInstance::CalculateInheritedModulationSettingsAndValue(const EAtomModulationDestination InDestination, FAtomSoundModulationSettings& InOutSettings, float& OutValue) const
{
	const Atom::FModulationParameter& InParam = GetParameterBasedOnDestination(InDestination);
	const FAtomActiveSound& InActiveSound = *ActiveSound;

	const FAtomSoundModulationSettings& InInstanceSettings = SoundData->GetModulationSettings().GetSettingsForDestination(InDestination);
	EAtomModulationRouting InInstanceRouting = SoundData->GetModulationSettings().GetRoutingForDestination(InDestination);

	const FAtomSoundModulationSettings& InActiveSettings = ActiveSound->ModulationRouting.GetSettingsForDestination(InDestination);
	EAtomModulationRouting InActiveRouting = ActiveSound->ModulationRouting.GetRoutingForDestination(InDestination);

	const UAtomSoundClass* EffectiveSoundClass = InActiveSound.GetSoundClass();
	if (SoundClass)
	{
		EffectiveSoundClass = SoundClass;
	}

	CalculateInheritedModulationSettingsAndValueInternal(InParam, 
		InActiveSettings, InActiveRouting, 
		InInstanceSettings, InInstanceRouting, 
		EffectiveSoundClass, [&]() { return EffectiveSoundClass->Properties.ModulationSettings.GetSettingsForDestination(InDestination); },
		InOutSettings, OutValue);
}

void FAtomPlaybackInstance::CalculateInheritedModulationSettingsAndValueInternal(
	const Atom::FModulationParameter& InParam,
	const FAtomSoundModulationSettings& InActiveSettings,
	EAtomModulationRouting InActiveRouting,
	const FAtomSoundModulationSettings& InInstanceSettings,
	EAtomModulationRouting InInstanceRouting,
	const UAtomSoundClass* InSoundClass,
	TFunctionRef<FAtomSoundModulationSettings()> GetClassSettings,
	FAtomSoundModulationSettings& InOutSettings, float& OutValue) const
{
	auto MixInRoutedValue = [&InParam](float& InOutValueA, float InValueB)
	{
		if (InParam.bRequiresConversion)
		{
			InParam.NormalizedFunction(InOutValueA);
			InParam.NormalizedFunction(InValueB);
		}
		InParam.MixFunction(InOutValueA, InValueB);
		if (InParam.bRequiresConversion)
		{
			InParam.UnitFunction(InOutValueA);
		}
	};

	auto UnionSoundClassSettings = [&](FAtomSoundModulationSettings& Settings)
	{
		if (InSoundClass)
		{
			const FAtomSoundModulationSettings& ClassSettings = GetClassSettings();
			OutValue = Settings.Value;
			MixInRoutedValue(OutValue, ClassSettings.Value);
			Settings.Modulators = Settings.Modulators.Union(ClassSettings.Modulators);
		}
	};

	switch (InActiveRouting)
	{
	case EAtomModulationRouting::Union:
	{
		FAtomSoundModulationSettings UnionSettings = InActiveSettings;
		switch (InInstanceRouting)
		{
		case EAtomModulationRouting::Union:
		{
			OutValue = UnionSettings.Value;
			MixInRoutedValue(OutValue, InInstanceSettings.Value);
			UnionSettings.Modulators = UnionSettings.Modulators.Union(InInstanceSettings.Modulators);
			UnionSoundClassSettings(UnionSettings);
		}
		break;

		case EAtomModulationRouting::Inherit:
		{
			UnionSoundClassSettings(UnionSettings);
		}
		break;

		case EAtomModulationRouting::Override:
		{
			OutValue = UnionSettings.Value;
			MixInRoutedValue(OutValue, InInstanceSettings.Value);
			UnionSettings.Modulators = UnionSettings.Modulators.Union(InInstanceSettings.Modulators);
		}
		break;

		case EAtomModulationRouting::DAW:
		{
			// Mix with Atom Craft value
			//todo: get the craft value
			OutValue = UnionSettings.Value;
			float AtomCraftValue = 1.0f;
			MixInRoutedValue(OutValue, AtomCraftValue);
		}
		break;

		case EAtomModulationRouting::Disable:
		default:
			break;
		}

		InOutSettings = UnionSettings;
		OutValue = UnionSettings.Value;
		return;
	}

	case EAtomModulationRouting::Inherit:
	{
		switch (InInstanceRouting)
		{
		case EAtomModulationRouting::Union:
		{
			FAtomSoundModulationSettings UnionSettings = InInstanceSettings;
			UnionSoundClassSettings(UnionSettings);
			InOutSettings = UnionSettings;
			OutValue = UnionSettings.Value;
		}
		break;

		case EAtomModulationRouting::Inherit:
		{
			if (InSoundClass)
			{
				FAtomSoundModulationSettings InheritSettings = GetClassSettings();
				InOutSettings = InheritSettings;
				OutValue = InheritSettings.Value;
			}
			else // nothing to inherit
			{
				InOutSettings = InInstanceSettings;
				OutValue = InInstanceSettings.Value;
			}
		}
		break;

		case EAtomModulationRouting::Override:
		{
			InOutSettings = InInstanceSettings;
			OutValue = InInstanceSettings.Value;
		}

		case EAtomModulationRouting::DAW:
		{
			// Use craft value
			FAtomSoundModulationSettings CraftSettings;
			CraftSettings.Value = InParam.DefaultValue;  // todo use a flag to disable volume mixing or return value from craft.
			InOutSettings = CraftSettings;
			OutValue = CraftSettings.Value;
		}
		break;

		case EAtomModulationRouting::Disable:
		default:
		{
			OutValue = InParam.DefaultValue;
		}
		break;
		}
	}
	break;

	case EAtomModulationRouting::Override:
	{
		FAtomSoundModulationSettings OverrideSettings = InActiveSettings;
		InOutSettings = OverrideSettings;
		OutValue = OverrideSettings.Value;
	}
	break;

	case EAtomModulationRouting::DAW:
	{
		// Use craft value
		FAtomSoundModulationSettings CraftSettings;
		CraftSettings.Value = InParam.DefaultValue;  // todo use a flag to disable volume mixing or return value from craft.
		InOutSettings = CraftSettings;
		OutValue = CraftSettings.Value;
	}
	break;

	case EAtomModulationRouting::Disable:
	default:
	{
		OutValue = InParam.DefaultValue;
	}
	break;
	}
}

/*
 * FAtomSource
 *****************************************************************************/

FAtomSource::~FAtomSource() 
{}

FString FAtomSource::Describe(bool bUseLongName)
{
	return FString::Printf(TEXT("Sound: %s, Volume: %6.2f, Owner: %s"),
		bUseLongName ? *PlaybackInstance->SoundData->GetPathName() : *PlaybackInstance->SoundData->GetName(),
		PlaybackInstance->GetVolume(),
		PlaybackInstance->ActiveSound ? *PlaybackInstance->ActiveSound->GetOwnerName() : TEXT("None"));
}

void FAtomSource::Stop()
{
	if (PlaybackInstance)
	{
		// The sound is stopping, so set the envelope value to 0.0f
		PlaybackInstance->SetEnvelopeValue(0.0f);
		NotifyPlaybackData();

		check(AtomRuntime);
		AtomRuntime->PlaybackInstanceSourceMap.Remove(PlaybackInstance);
		PlaybackInstance->NotifyFinished(true);
		PlaybackInstance = nullptr;
	}

	if (!IsVoiceSource())
	{
		// Set as free reagrdless is fully finished or not
		AtomRuntime->FreeSources.AddUnique(this);
	}
	else
	{
		// Set to remove and delete safely
		AtomRuntime->ToRemoveSources.AddUnique(this);
	}
}

void FAtomSource::SetPauseByGame(bool bInIsPauseByGame)
{
	bIsPausedByGame = bInIsPauseByGame;
	UpdatePause();
}

void FAtomSource::SetPauseManually(bool bInIsPauseManually)
{
	bIsManuallyPaused = bInIsPauseManually;
	UpdatePause();
}

void FAtomSource::UpdatePause()
{
	if (IsPaused() && !bIsPausedByGame && !bIsManuallyPaused)
	{
		Play();
	}
	else if (!IsPaused() && (bIsManuallyPaused || bIsPausedByGame))
	{
		Pause();
	}
}

bool FAtomSource::IsGameOnly() const
{
	return (PlaybackInstance && !PlaybackInstance->bIsUISound);
}

void FAtomSource::SetFilterFrequency()
{
	switch (AtomRuntime->GetMixDebugState())
	{
		case EAtomDebugState::TestLPF:
		{
			LPFFrequency = ATOM_MIN_FILTER_FREQUENCY;
		}
		break;
		case EAtomDebugState::DisableLPF:
		{
			LPFFrequency = ATOM_MAX_FILTER_FREQUENCY;
		}
		break;

		default:
		{
			// compensate for filter coefficient calculation error for occlusion
			float OcclusionFilterScale = 1.0f;
			if (AtomOcclusionFilterScaleEnabledCVar == 1 && !FMath::IsNearlyEqual(PlaybackInstance->OcclusionFilterFrequency, ATOM_MAX_FILTER_FREQUENCY))
			{
				OcclusionFilterScale = 0.25f;
			}

			// Set the LPFFrequency to lowest provided value
			LPFFrequency = PlaybackInstance->OcclusionFilterFrequency * OcclusionFilterScale;

			if (PlaybackInstance->bEnableLowPassFilter)
			{
				LPFFrequency = FMath::Min(LPFFrequency, PlaybackInstance->LowPassFilterFrequency);
			}

			LPFFrequency = FMath::Min(LPFFrequency, PlaybackInstance->AmbientZoneFilterFrequency);
			LPFFrequency = FMath::Min(LPFFrequency, PlaybackInstance->AttenuationLowpassFilterFrequency);
			LPFFrequency = FMath::Min(LPFFrequency, PlaybackInstance->SoundClassFilterFrequency);
		}
		break;
	}

	switch (AtomRuntime->GetMixDebugState())
	{
		case EAtomDebugState::TestHPF:
		{
			HPFFrequency = ATOM_MAX_FILTER_FREQUENCY;
		}
		break;

		case EAtomDebugState::DisableHPF:
		{
			HPFFrequency = ATOM_MIN_FILTER_FREQUENCY;
		}
		break;

		default:
		{
			// Set the HPFFrequency to highest provided value
			HPFFrequency = PlaybackInstance->AttenuationHighpassFilterFrequency;
			HPFFrequency = FMath::Max(HPFFrequency, PlaybackInstance->SoundClassHighPassFilterFrequency);

			if (PlaybackInstance->bEnableHighPassFilter)
			{
				HPFFrequency = FMath::Max(HPFFrequency, PlaybackInstance->HighPassFilterFrequency);
			}
		}
		break;
	}
}

FAtomSpatializationParams FAtomSource::GetSpatializationParams()
{
	FAtomSpatializationParams Params;

	// Put the Atom time stamp on the spatialization params
	Params.AtomClock = AtomRuntime->GetAtomClock();

	if (PlaybackInstance->GetUseSpatialization())
	{
		FVector EmitterPosition = AtomRuntime->GetListenerTransformedDirection(PlaybackInstance->Transform.GetTranslation(), &Params.Distance);
	
		// Independently retrieve the attenuation used for distance in case it was overridden
		Params.AttenuationDistance = AtomRuntime->GetDistanceToNearestListener(PlaybackInstance->Transform.GetTranslation());
	
		// If we are using the Non-spatialized radius feature
		if (PlaybackInstance->NonSpatializedRadiusStart > 0.0f)
		{
			float NonSpatializedRadiusEnd = FMath::Min(PlaybackInstance->NonSpatializedRadiusStart, PlaybackInstance->NonSpatializedRadiusEnd);

			if (Params.Distance > 0.0f)
			{
				// If the user specified a distance below which to be fully 2D
				if (NonSpatializedRadiusEnd > 0.0f)
				{
					// We're in the non-spatialized domain
					if (Params.Distance < PlaybackInstance->NonSpatializedRadiusStart)
					{
						float NonSpatializationRange = PlaybackInstance->NonSpatializedRadiusStart - NonSpatializedRadiusEnd;
						NonSpatializationRange = FMath::Max(NonSpatializationRange, 1.0f);
						Params.NonSpatializedAmount = FMath::Clamp((PlaybackInstance->NonSpatializedRadiusStart - Params.Distance)/ NonSpatializationRange, 0.0f, 1.0f);
					}					
				}
				else
				{
					// Initialize to full omni-directionality (bigger value, more omni)
					static const float MaxNormalizedRadius = 1000000.0f;
					float NormalizedOmniRadus = FMath::Clamp(PlaybackInstance->NonSpatializedRadiusStart / Params.Distance, 0.0f, MaxNormalizedRadius);
					if (NormalizedOmniRadus > 1.0f)
					{
						float NormalizedOmniRadusSquared = NormalizedOmniRadus * NormalizedOmniRadus;
						Params.NonSpatializedAmount = 1.0f - 1.0f / NormalizedOmniRadusSquared;
					}
					else
					{
						Params.NonSpatializedAmount = 0.0f;
					}
				}

				//UE_LOG(LogTemp, Log, TEXT("Distance: %.2f, NonSpatializedRadiusStart: %.2f, NonSpatializedRadiusEnd: %.2f, NonSpatializedAmount: %.2f"), Params.Distance, PlaybackInstance->NonSpatializedRadiusStart, PlaybackInstance->NonSpatializedRadiusEnd, Params.NonSpatializedAmount);
			
			}
		}
		else
		{
			Params.NonSpatializedAmount = 0.0f;
		}

		Params.EmitterPosition = EmitterPosition;

		if (WaveInfo.NumChannels == 2)
		{
			Params.LeftChannelPosition = AtomRuntime->GetListenerTransformedDirection(LeftChannelSourceLocation, nullptr);
			Params.RightChannelPosition = AtomRuntime->GetListenerTransformedDirection(RightChannelSourceLocation, nullptr);
		}
	}
	else
	{
		//Params.NormalizedOmniRadius = 0.0f;
		Params.Distance = 0.0f;
		Params.EmitterPosition = FVector::ZeroVector;
	}
	Params.EmitterWorldPosition = PlaybackInstance->Transform.GetTranslation();

	int32 ListenerIndex = 0;
	if (PlaybackInstance->ActiveSound)
	{
		Params.EmitterWorldRotation = PlaybackInstance->ActiveSound->Transform.GetRotation();
		ListenerIndex = PlaybackInstance->ActiveSound->GetClosestListenerIndex();
	}
	else
	{
		Params.EmitterWorldRotation = FQuat::Identity;
	}

	// Pass the actual listener orientation and position
	FTransform ListenerTransform;
	AtomRuntime->GetListenerTransform(ListenerIndex, ListenerTransform);
	Params.ListenerOrientation = ListenerTransform.GetRotation();
	Params.ListenerPosition = ListenerTransform.GetLocation();

	return Params;
}

void FAtomSource::InitCommon()
{
	PlaybackTime = 0.0f;
	TickCount = 0;

	// Reset pause state
	bIsPausedByGame = false;
	bIsManuallyPaused = false;

#if ENABLE_AUDIO_DEBUG
	DebugInfo = MakeShared<FDebugInfo, ESPMode::ThreadSafe>();
#endif //ENABLE_AUDIO_DEBUG
}

void FAtomSource::UpdateCommon()
{
	check(PlaybackInstance);

	Pitch = PlaybackInstance->GetPitch();

	// Don't apply global pitch scale to UI sounds
	if (!PlaybackInstance->bIsUISound)
	{
		Pitch *= AtomRuntime->GetGlobalPitchScale().GetValue();
	}

	Pitch = AtomRuntime->ClampPitch(Pitch);

	// Track playback time even if the voice is not virtual, it can flip to being virtual while playing.
	const float RuntimeDeltaTime = AtomRuntime->GetRuntimeDeltaTime();

	// Scale the playback time based on the pitch of the sound
	PlaybackTime += RuntimeDeltaTime * Pitch * (1.0f / PlaybackInstance->GetTimeStretchRatio());
}

float FAtomSource::GetPlaybackPercent() const
{
	const float Percentage = PlaybackTime / PlaybackInstance->SoundData->GetDuration();
	if (PlaybackInstance->LoopingMode == EAtomLoopingMode::LoopNever)
	{
		return FMath::Clamp(Percentage, 0.0f, 1.0f);
	}
	else
	{
		// Wrap the playback percent for looping sounds
		return FMath::Fmod(Percentage, 1.0f);
	}

}

void FAtomSource::UpdateStereoEmitterPositions()
{
	// Only call this function if we're told to use spatialization
	check(PlaybackInstance->GetUseSpatialization());
	check(WaveInfo.NumChannels == 2);

	if (!AtomDisableStereoSpreadCvar && PlaybackInstance->StereoSpread > 0.0f)
	{
		// We need to compute the stereo left/right channel positions using the Atom component position and the spread
		FVector ListenerPosition;

		const bool bAllowAttenuationOverride = false;
		const int32 ListenerIndex = PlaybackInstance->ActiveSound ? PlaybackInstance->ActiveSound->GetClosestListenerIndex() : 0;
		AtomRuntime->GetListenerPosition(ListenerIndex, ListenerPosition, bAllowAttenuationOverride);
		FVector ListenerToSourceDir = (PlaybackInstance->Transform.GetLocation() - ListenerPosition).GetSafeNormal();

		float HalfSpread = 0.5f * PlaybackInstance->StereoSpread;

		// Get direction of left emitter from true emitter position (left hand rule)
		FVector LeftEmitterDir = FVector::CrossProduct(ListenerToSourceDir, FVector::UpVector);
		FVector LeftEmitterOffset = LeftEmitterDir * HalfSpread;

		// Get position vector of left emitter by adding to true emitter the dir scaled by half the spread
		LeftChannelSourceLocation = PlaybackInstance->Transform.GetLocation() + LeftEmitterOffset;

		// Right emitter position is same as right but opposite direction
		RightChannelSourceLocation = PlaybackInstance->Transform.GetLocation() - LeftEmitterOffset;
	}
	else
	{
		LeftChannelSourceLocation = PlaybackInstance->Transform.GetLocation();
		RightChannelSourceLocation = PlaybackInstance->Transform.GetLocation();
	}
}

float FAtomSource::GetDebugVolume(const float InVolume)
{
	float OutVolume = InVolume;

#if ENABLE_ATOM_DEBUG

	// Bail if we don't have a device manager.
	if (!GCriWare || !GCriWare->GetAtomRuntimeManager() || !PlaybackInstance || !DebugInfo.IsValid())
	{
		return OutVolume;
	}

	// Solos/Mutes (dev only).
	Atom::FAtomDebugger& Debugger = GCriWare->GetAtomRuntimeManager()->GetDebugger();
	FDebugInfo Info;

	// SoundPlayback Solo/Mutes.
	if (OutVolume != 0.0f)
	{
		Debugger.QuerySoloMuteSoundWave(PlaybackInstance->GetName(), Info.bIsSoloed, Info.bIsMuted, Info.MuteSoloReason);
		if (Info.bIsMuted)
		{
			OutVolume = 0.0f;
		}
	}

	// SoundCues mutes/solos (not strictly just cues but any SoundBase)
	if (OutVolume != 0.0f && PlaybackInstance->ActiveSound)
	{
		if (UAtomSoundBase* Sound = PlaybackInstance->ActiveSound->GetSound())
		{
			Debugger.QuerySoloMuteSoundCue(Sound->GetSoundName(), Info.bIsSoloed, Info.bIsMuted, Info.MuteSoloReason);
			if (Info.bIsMuted)
			{
				OutVolume = 0.0f;
			}
		}
	}

	// SoundClass mutes/solos.
	if (OutVolume != 0.0f && PlaybackInstance->SoundClass)
	{
		FString SoundClassName;
		PlaybackInstance->SoundClass->GetName(SoundClassName);
		Debugger.QuerySoloMuteSoundClass(SoundClassName, Info.bIsSoloed, Info.bIsMuted, Info.MuteSoloReason);
		if (Info.bIsMuted)
		{
			OutVolume = 0.0f;
		}
	}

	// Update State. 
	FScopeLock Lock(&DebugInfo->CS);
	{
		DebugInfo->bIsMuted = Info.bIsMuted;
		DebugInfo->bIsSoloed = Info.bIsSoloed;
		DebugInfo->MuteSoloReason = MoveTemp(Info.MuteSoloReason);
	}

#endif // ENABLE_ATOM_DEBUG

	return OutVolume;
}

void FAtomSource::GetChannelLocations(FVector& Left, FVector& Right) const
{
	Left = LeftChannelSourceLocation;
	Right = RightChannelSourceLocation;
}

void FAtomSource::NotifyPlaybackData()
{
	const uint64 AtomComponentID = PlaybackInstance->ActiveSound->GetAtomComponentID();
	if (AtomComponentID > 0)
	{
		const UAtomSoundBase* SoundBase = PlaybackInstance->SoundData;

		if (PlaybackInstance->ActiveSound->bUpdatePlayPercentage)
		{
			const float PlaybackPercent = GetPlaybackPercent();
			FAtomThread::RunCommandOnGameThread([AtomComponentID, SoundBase, PlaybackPercent]()
			{
				if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
				{
					if (AtomComponent->OnAtomSoundPlaybackPercent.IsBound())
					{
						AtomComponent->OnAtomSoundPlaybackPercent.Broadcast(SoundBase, PlaybackPercent);
					}

					if (AtomComponent->OnAtomSoundPlaybackPercentNative.IsBound())
					{
						AtomComponent->OnAtomSoundPlaybackPercentNative.Broadcast(AtomComponent, SoundBase, PlaybackPercent);
					}
				}
			});
		}

		if (PlaybackInstance->ActiveSound->bUpdateSingleEnvelopeValue)
		{
			const float EnvelopeValue = GetEnvelopeValue();
			FAtomThread::RunCommandOnGameThread([AtomComponentID, SoundBase, EnvelopeValue]()
				{
					if (UAtomComponent* AudioComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
					{
						if (AudioComponent->OnAtomSoundSingleEnvelopeValue.IsBound())
						{
							AudioComponent->OnAtomSoundSingleEnvelopeValue.Broadcast(SoundBase, EnvelopeValue);
						}

						if (AudioComponent->OnAtomSoundSingleEnvelopeValueNative.IsBound())
						{
							AudioComponent->OnAtomSoundSingleEnvelopeValueNative.Broadcast(AudioComponent, SoundBase, EnvelopeValue);
						}
					}
				});
		}

		// We do a broadcast from the active sound in this case, just update the envelope value of the wave instance here
		if (PlaybackInstance->ActiveSound->bUpdateMultiEnvelopeValue)
		{
			const float EnvelopeValue = GetEnvelopeValue();
			PlaybackInstance->SetEnvelopeValue(EnvelopeValue);
		}
	}
}

/*
 * FAtomDynamicParameter
 *****************************************************************************/

FAtomDynamicParameter::FAtomDynamicParameter(float Value)
	: CurrValue(Value)
	, StartValue(Value)
	, DeltaValue(0.0f)
	, CurrTimeSec(0.0f)
	, DurationSec(0.0f)
	, LastTime(0.0f)
	, TargetValue(Value)
{}

void FAtomDynamicParameter::Set(float Value, float InDuration)
{
	if (TargetValue != Value || DurationSec != InDuration)
	{
		TargetValue = Value;
		if (InDuration > 0.0f)
		{
			DeltaValue = Value - CurrValue;
			StartValue = CurrValue;
			DurationSec = InDuration;
			CurrTimeSec = 0.0f;
		}
		else
		{
			StartValue = Value;
			DeltaValue = 0.0f;
			DurationSec = 0.0f;
			CurrValue = Value;
		}
	}
}

void FAtomDynamicParameter::Update(float DeltaTime)
{
	if (DurationSec > 0.0f)
	{
		float TimeFraction = CurrTimeSec / DurationSec;
		if (TimeFraction < 1.0f)
		{
			CurrValue = DeltaValue * TimeFraction + StartValue;
		}
		else
		{
			CurrValue = StartValue + DeltaValue;
			DurationSec = 0.0f;
		}
		CurrTimeSec += DeltaTime;
	}
}
