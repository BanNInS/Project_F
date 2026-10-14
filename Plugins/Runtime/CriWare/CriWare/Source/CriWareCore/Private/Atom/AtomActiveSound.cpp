/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomActiveSound.cpp
 *
 ****************************************************************************/

#include "Atom/AtomActiveSound.h"

#include "EngineDefines.h"
#include "Misc/App.h"
#include "Async/Async.h"

#include "CriWare.h"
#include "Atom/AtomThread.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundClass.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"
#include "Atom/AtomRack.h"
#include "Atom/Mixer/AtomMixerPlayer.h"

static int32 AtomOcclusionDisabledCvar = 0;
FAutoConsoleVariableRef CVarAtomOcclusionEnabled(
	TEXT("atom.DisableOcclusion"),
	AtomOcclusionDisabledCvar,
	TEXT("Disables (1) or enables (0) Atom sound occlusion.\n"),
	ECVF_Default);

static int32 GatherInteriorDataFromIAtomActiveSoundUpdateCVar = 1;
FAutoConsoleVariableRef CVarGatherInteriorDataFromIAtomActiveSoundUpdate(
	TEXT("atom.InteriorData.UseIAtomActiveSoundUpdate"),
	GatherInteriorDataFromIAtomActiveSoundUpdateCVar,
	TEXT("When set to 1, allows gathering of interior data from subsystems that implement the IAtomActiveSoundUpdate interface.\n")
	TEXT("0: Disabled, 1: Enabled (default)"),
	ECVF_Default);

static int32 AtomInitializeFocusFactorOnFirstUpdateCVar = 1;
FAutoConsoleVariableRef CVarAtomInitializeFocusFactorOnFirstUpdateCVar(
	TEXT("atom.FocusData.InitializeFocusFactorOnFirstUpdate"),
	AtomInitializeFocusFactorOnFirstUpdateCVar,
	TEXT("When set to 1, focus factor will be initialized on first update to the proper value, instead of interpolating from 0 to the proper value.\n")
	TEXT("0: Disabled, 1: Enabled (default)"),
	ECVF_Default);

FTraceDelegate FAtomActiveSound::ActiveSoundTraceDelegate;
TMap<FTraceHandle, FAtomActiveSound::FAsyncTraceDetails> FAtomActiveSound::TraceToActiveSoundMap;

FAtomActiveSound::FAtomActiveSound()
	: World(nullptr)
	, WorldID(0)
	, Sound(nullptr)
	//, SourceEffectChain(nullptr)
    , SoundAttenuation(nullptr)
	, AtomComponentID(0)
	, OwnerID(0)
	, PlayOrder(INDEX_NONE)
	, AtomRuntime(nullptr)
	, SoundClassOverride(nullptr)
	, bForceUseAtomPlayer(false)
	, bHasCheckedOcclusion(false)
	, bAllowSpatialization(true)
	, bHasAttenuationSettings(false)
	, bShouldRemainActiveIfDropped(false)
	, bFinished(false)
	, bIsPaused(false)
	, bShouldStopDueToMaxConcurrency(false)
	, bVirtualizedDueToMaxConcurrency(false)
	, bIsRealizingWithFadeIn(false)
	, bIsPendingVirtualization(false)
	, bHasVirtualized(false)
	//, bRadioFilterSelected(false)
	//, bApplyRadioFilter(false)
	, bLocationDefined(false)
	, bIgnoreForFlushing(false)
	, bAlwaysPlay(false)
	, bIsUISound(false)
	//, bIsMusic(false)
	//, bReverb(false)
	//, bCenterChannelOnly(false)
	, bIsPreviewSound(false)
	, bGotInteriorSettings(false)
	, bApplyInteriorVolumes(false)
#if !(NO_LOGGING || UE_BUILD_SHIPPING || UE_BUILD_TEST)
	, bWarnedAboutOrphanedLooping(false)
#endif
	, bEnableLowPassFilter(false)
	, bEnableHighPassFilter(false)
	, bEnableAmplitudeEnvelope(false)
	, bUpdatePlayPercentage(false)
	, bUpdateSingleEnvelopeValue(false)
	, bUpdateMultiEnvelopeValue(false)
	, bUpdatePlaybackTime(false)
	, bIsPlayingAudio(false)
	, bIsStopping(false)
	, bHasActiveSourceBusSendRoutingOverride(false)
	, bHasActiveMainRackOutputOverride(false)
	, bHasActiveSoundBusSendRoutingOverride(false)
	, bEnableSourceBusSendRoutingOverride(false)
	, bEnableMainRackOutputOverride(false)
	, bEnableSoundBusSendRoutingOverride(false)
	, bIsFirstAttenuationUpdate(true)
	, bStartedWithinNonBinauralRadius(false)
	, bModulationRoutingUpdated(false)
    , bIsAttenuationSettingsOverridden(false)
	, UserIndex()
	, FadeOut(EFadeOut::None)
	, bIsOccluded(false)
	, bAsyncOcclusionPending(false)
	, PlaybackTime(0.0f)
	, PlaybackTimeNonVirtualized(0.0f)
	, MinCurrentPitch(1.0f)
	, RequestedStartTime(0.0f)
	, VolumeMultiplier(1.0f)
	, PitchMultiplier(1.0f)
	, LowPassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, HighPassFilterFrequency(ATOM_MIN_FILTER_FREQUENCY)
	, CurrentOcclusionFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, CurrentOcclusionVolumeAttenuation(1.0f)
	, Priority(1.0f)
	, VolumeConcurrency(0)
	, OcclusionCheckInterval(0.0f)
	, LastOcclusionCheckTime(TNumericLimits<float>::Lowest())
	, MaxDistance(FAtomRuntime::GetMaxWorldDistance())
	, LastLocation(FVector::ZeroVector)
	, MultiPositionType(EAtomMultiPositionType::SingleSource)
	, EnvelopeFollowerAttackTime(10)
	, EnvelopeFollowerReleaseTime(100)
	, bHasNewSourceBusSends(false)
	, OnSoundFinished(nullptr)
	, ThreadToCallOnSoundFinishedOn(ENamedThreads::GameThread)
	, PcmFilterFunction(nullptr)
	, CueFirstBlockIndex(0)
	, CueNextBlockIndex(INDEX_NONE)
	, CueBeatSyncOffset(0)
#if ENABLE_ATOM_DEBUG
	, DebugColor(FColor::Black)
#endif // ENABLE_ATOM_DEBUG
	, ClosestListenerIndex(INDEX_NONE)
{
	static uint32 TotalPlayOrder = 0;
	PlayOrder = TotalPlayOrder++;

	if (!ActiveSoundTraceDelegate.IsBound())
	{
		ActiveSoundTraceDelegate.BindStatic(&OcclusionTraceDone);
	}
}

FAtomActiveSound::FAtomActiveSound(const FAtomActiveSound&) = default;

FAtomActiveSound::~FAtomActiveSound()
{
	ensureMsgf(PlaybackInstances.Num() == 0, TEXT("Destroyed an active sound that had active playback instances."));
	check(CanDelete());
}

FAtomActiveSound* FAtomActiveSound::CreateVirtualCopy(const FAtomActiveSound& InActiveSoundToCopy, FAtomRuntime& InAtomRuntime)
{
	check(!InActiveSoundToCopy.bIsStopping);
	check(!InActiveSoundToCopy.CanVirtualizeWhenSilent());

	FAtomActiveSound* ActiveSound = new FAtomActiveSound(InActiveSoundToCopy);
	ActiveSound->AtomRuntime = &InAtomRuntime;
	ActiveSound->PlaybackTimeNonVirtualized = 0.0f;

	// If we are the restart virtual mode, we need to reset our sound cue parse state and our playback time.
	//UAtomSoundBase* Sound = InActiveSoundToCopy.GetSound();
	//check(Sound);
	//if (Sound->VirtualizationMode == EAtomVirtualizationMode::Restart)
	//{
	//	ActiveSound->SoundNodeOffsetMap.Reset();
	//}

	// If volume concurrency tracking is enabled, reset the value,
	// otherwise keep disabled
	if (InActiveSoundToCopy.VolumeConcurrency >= 0.0f)
	{
		ActiveSound->VolumeConcurrency = 1.0f;
	}

	ActiveSound->ConcurrencyGroupData.Reset();
	ActiveSound->PlaybackInstances.Reset();

	ActiveSound->SetVirtualized(true);
	return ActiveSound;
}

void FAtomActiveSound::SetVirtualized(bool bIsVirtualized)
{
	check(!bIsStopping);

	if (bIsVirtualized)
	{
		bAsyncOcclusionPending = false;
		bHasVirtualized = true;
		bIsPlayingAudio = false;
		bShouldStopDueToMaxConcurrency = false;
		bIsPendingVirtualization = false;
	}
	else
	{
	}

	FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID(), bIsVirtualized]()
	{
		if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
		{
			AtomComponent->SetIsVirtualized(bIsVirtualized);
		}
	});
}

void FAtomActiveSound::SetCueBlockIndex(int32 BlockIndex)
{
	FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID(), BlockIndex]()
	{
		if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
		{
			AtomComponent->SetCueBlockIndex(BlockIndex);
		}
	});
}

void FAtomActiveSound::SetCueBeatSyncInfo(const TSharedPtr<const FAtomBeatSyncInfo>& BeatSyncInfo)
{
	if (BeatSyncInfo.IsValid())
	{
		FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID(), InfoRef = BeatSyncInfo.ToSharedRef()]()
			{
				if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
				{
					AtomComponent->SetCueBeatSyncInfo(InfoRef.Get());
				}
			});
	}
}

void FAtomActiveSound::OnCueBeatSync(const TSharedPtr<const FAtomBeatSyncInfo>& BeatSyncInfo)
{
	if (BeatSyncInfo.IsValid())
	{
		FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID(), InfoRef = BeatSyncInfo.ToSharedRef()]()
		{
			if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
			{
				AtomComponent->OnCueBeatSync(InfoRef.Get());
			}
		});
	}
}

void FAtomActiveSound::OnCueSequencerEvent(const TSharedPtr<const FAtomSequencerEventInfo>& EventInfo)
{
	if (EventInfo.IsValid())
	{
		FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID(), InfoRef = EventInfo.ToSharedRef()]()
		{
			if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
			{
				AtomComponent->OnCueSequencerEvent(InfoRef.Get());
			}
		});
	}
}

FArchive& operator<<(FArchive& Ar, FAtomActiveSound* ActiveSound)
{
	if (!Ar.IsLoading() && !Ar.IsSaving())
	{
		Ar << ActiveSound->Sound;
		Ar << ActiveSound->PlaybackInstances;
	}
	return(Ar);
}

void FAtomActiveSound::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto PlaybackInstanceIt(PlaybackInstances.CreateConstIterator()); PlaybackInstanceIt; ++PlaybackInstanceIt)
	{
		FAtomPlaybackInstance* PlaybackInstance = PlaybackInstanceIt.Value();
		// Avoid recursing back to the playback instance that sourced this active sound
		if (PlaybackInstance)
		{
			PlaybackInstance->AddReferencedObjects(Collector);
		}
	}

	Collector.AddReferencedObject(SoundClassOverride);

	/*Collector.AddReferencedObject(SourceEffectChain);
	if (SourceEffectChain)
	{
		SourceEffectChain->AddReferencedEffects(Collector);
	}*/
    
	Collector.AddReferencedObject(Sound);
	/*if (Sound && Sound->SourceEffectChain)
	{
		Sound->SourceEffectChain->AddReferencedEffects(Collector);
	}*/

    Collector.AddReferencedObject(SoundAttenuation);
    
	for (auto& Concurrency : ConcurrencySet)
	{
		if (Concurrency)
		{
			Collector.AddReferencedObject(Concurrency);
		}
	}

	/*if (InstanceTransmitter.IsValid())
	{
		TArray<UObject*> InstanceReferences = InstanceTransmitter->GetReferencedObjects();
		for (UObject* Object : InstanceReferences)
		{
			Collector.AddReferencedObject(Object);
		}
	}*/

	// Adding ref'd objects in AttenuationSettings
	Collector.AddReferencedObjects(AttenuationSettings.PluginSettings.SourceDataOverridePluginSettingsArray);
	Collector.AddReferencedObjects(AttenuationSettings.PluginSettings.RuntimePluginSettingsArray);

	/*for (FAudioVolumeSubmixSendSettings& SendSettings : AudioVolumeSubmixSendSettings)
	{
		for (FSoundSubmixSendInfo& SendInfo : SendSettings.SubmixSends)
		{
			Collector.AddReferencedObject(SendInfo.SoundSubmix);
		}
	}*/

	/*for (FAudioVolumeSubmixSendSettings& SendSettings : PreviousAudioVolumeSubmixSendSettings)
	{
		for (FSoundSubmixSendInfo& SendInfo : SendSettings.SubmixSends)
		{
			Collector.AddReferencedObject(SendInfo.SoundSubmix);
		}
	}*/

	for (TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>& SourceBusSendTuple : NewSourceBusSends)
	{
		Collector.AddReferencedObject(SourceBusSendTuple.Value.AudioBus);
		Collector.AddReferencedObject(SourceBusSendTuple.Value.SoundSourceBus);
	}

	Collector.AddReferencedObjects(ModulationRouting.VolumeModulation.Modulators);
	Collector.AddReferencedObjects(ModulationRouting.PitchModulation.Modulators);
	Collector.AddReferencedObjects(ModulationRouting.HighpassModulation.Modulators);
	Collector.AddReferencedObjects(ModulationRouting.LowpassModulation.Modulators);
}

int32 FAtomActiveSound::GetPlayCount() const
{
	if (!Sound || !AtomRuntime)
	{
		return 0;
	}

	if (const int32* PlayCount = Sound->CurrentPlayCount.Find(AtomRuntime->GetAtomRuntimeID()))
	{
		return *PlayCount;
	}

	return 0;
}

void FAtomActiveSound::SetPitch(float Value)
{
	PitchMultiplier = Value;
}

void FAtomActiveSound::SetVolume(float Value)
{
	VolumeMultiplier = Value;
}

void FAtomActiveSound::SetWorld(UWorld* InWorld)
{
	World = InWorld;
	WorldID = (InWorld ? InWorld->GetUniqueID() : 0);
}

void FAtomActiveSound::SetSound(UAtomSoundBase* InSound)
{
	Sound = InSound;
	bApplyInteriorVolumes = (SoundClassOverride && SoundClassOverride->Properties.bApplyAmbientVolumes)
		|| (Sound && Sound->ShouldApplyInteriorVolumes());
}

void FAtomActiveSound::SetSoundClass(UAtomSoundClass* SoundClass)
{
	SoundClassOverride = SoundClass;
	bApplyInteriorVolumes = (SoundClassOverride && SoundClassOverride->Properties.bApplyAmbientVolumes)
		|| (Sound && Sound->ShouldApplyInteriorVolumes());
}

void FAtomActiveSound::SetAttenuationSettingsAsset(TObjectPtr<UAtomAttenuation> InSoundAttenuation)
{
    SoundAttenuation = InSoundAttenuation;
}

void FAtomActiveSound::SetAttenuationSettingsOverride(bool bInIsAttenuationSettingsOverride)
{
    bIsAttenuationSettingsOverridden = bInIsAttenuationSettingsOverride;
}

bool FAtomActiveSound::IsPlayWhenSilent() const
{
	return (Sound && Sound->IsPlayWhenSilent()) || (PcmFilterFunction != nullptr);
}

bool FAtomActiveSound::CanVirtualizeWhenSilent() const
{
	if (Sound)
	{
		EAtomVirtualizationMode VirtualizationMode = Sound->GetVirtualizationMode();
		return VirtualizationMode == EAtomVirtualizationMode::PlayWhenSilent
			|| VirtualizationMode == EAtomVirtualizationMode::Retrigger;
	}
	return false;
}

void FAtomActiveSound::ClearAtomComponent()
{
	AtomComponentID = 0;
	AtomComponentUserID = NAME_None;
	AtomComponentName = NAME_None;

	OwnerID = 0;
	OwnerName = NAME_None;
}

void FAtomActiveSound::SetAtomComponent(const FAtomActiveSound& ActiveSound)
{
	AtomComponentID = ActiveSound.AtomComponentID;
	AtomComponentUserID = ActiveSound.AtomComponentUserID;
	AtomComponentName = ActiveSound.AtomComponentName;

	OwnerID = ActiveSound.OwnerID;
	OwnerName = ActiveSound.OwnerName;
}

void FAtomActiveSound::SetAtomComponent(const UAtomComponent& Component)
{
	check(IsInGameThread());

	AActor* Owner = Component.GetOwner();

	AtomComponentID = Component.GetAtomComponentID();
	AtomComponentUserID = Component.GetAtomComponentUserID();
	AtomComponentName = Component.GetFName();

	SetOwner(Owner);
}

void FAtomActiveSound::SetOwner(const AActor* Actor)
{
	if (Actor)
	{
		OwnerID = Actor->GetUniqueID();
		OwnerName = Actor->GetFName();
	}
	else
	{
		OwnerID = 0;
		OwnerName = NAME_None;
	}
}

FString FAtomActiveSound::GetAtomComponentName() const
{
	return (AtomComponentID > 0 ? AtomComponentName.ToString() : TEXT("NO COMPONENT"));
}

FString FAtomActiveSound::GetOwnerName() const
{
	return (OwnerID > 0 ? OwnerName.ToString() : TEXT("None"));
}

UAtomSoundClass* FAtomActiveSound::GetSoundClass() const
{
	if (SoundClassOverride)
	{
		return SoundClassOverride;
	}
	else if (Sound)
	{
		return Sound->GetSoundClass();
	}

	return nullptr;
}

UAtomRackBase* FAtomActiveSound::GetSoundRack() const
{
	return Sound ? Sound->GetAtomRack() : nullptr;
}

void FAtomActiveSound::SetAtomRackSend(const FAtomSoundToRackSend& RackSend)
{
	// Override send level if submix send already included in active sound
	for (FAtomSoundToRackSend& Info : AtomRackSendsOverride)
	{
		if (Info.Rack == RackSend.Rack)
		{
			Info.SendLevel = RackSend.SendLevel;
			return;
		}
	}

	// Otherwise, add it to the bus send overrides
	AtomRackSendsOverride.Add(RackSend);
}

void FAtomActiveSound::GetAtomRackSends(TArray<FAtomSoundToRackSend>& OutSends) const
{
	if (Sound)
	{
		// Get the base sends
		//Sound->GetSoundSubmixSends(OutSends);

		// Loop through the overrides, which may append or override the existing send
		for (const FAtomSoundToRackSend& SendInfo : AtomRackSendsOverride)
		{
			bool bOverridden = false;
			for (FAtomSoundToRackSend& OutSendInfo : OutSends)
			{
				if (OutSendInfo.Rack == SendInfo.Rack)
				{
					OutSendInfo.SendLevel = SendInfo.SendLevel;
					bOverridden = true;
					break;
				}
			}

			if (!bOverridden)
			{
				OutSends.Add(SendInfo);
			}
		}
	}
}

void FAtomActiveSound::SetAtomBusSend(const FAtomSoundToBusSend& BusSend)
{
	// Override send level if submix send already included in active sound
	for (FAtomSoundToBusSend& Info : AtomBusSendsOverride)
	{
		if (Info.Bus == BusSend.Bus)
		{
			Info.SendLevel = BusSend.SendLevel;
			return;
		}
	}

	// Otherwise, add it to the bus send overrides
	AtomBusSendsOverride.Add(BusSend);
}

void FAtomActiveSound::GetAtomBusSends(TArray<FAtomSoundToBusSend>& OutSends) const
{
	if (Sound)
	{
		// Get the base sends
		Sound->GetAtomBusSends(OutSends);

		// Loop through the overrides, which may append or override the existing send
		for (const FAtomSoundToBusSend& SendInfo : AtomBusSendsOverride)
		{
			bool bOverridden = false;
			for (FAtomSoundToBusSend& OutSendInfo : OutSends)
			{
				if (OutSendInfo.Bus == SendInfo.Bus)
				{
					OutSendInfo.SendLevel = SendInfo.SendLevel;
					bOverridden = true;
					break;
				}
			}

			if (!bOverridden)
			{
				OutSends.Add(SendInfo);
			}
		}
	}
}

void FAtomActiveSound::SetSourceBusSend(EAtomBusSendStage BusSendStage, const FAtomSoundSourceBusSendInfo& SendInfo)
{
	// Override send level if the source bus send is already included in active sound
	for (FAtomSoundSourceBusSendInfo& Info : SourceBusSendsOverride[(int32)BusSendStage])
	{
		if (Info.SoundSourceBus == SendInfo.SoundSourceBus || Info.AudioBus == SendInfo.AudioBus)
		{
			Info.SendLevel = SendInfo.SendLevel;

			bHasNewSourceBusSends = true;
			NewSourceBusSends.Add(TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>(BusSendStage, SendInfo));
			return;
		}
	}

	// Otherwise, add it to the source bus send overrides
	SourceBusSendsOverride[(int32)BusSendStage].Add(SendInfo);

	bHasNewSourceBusSends = true;
	NewSourceBusSends.Add(TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>(BusSendStage, SendInfo));
}

void FAtomActiveSound::GetSourceBusSends(EAtomBusSendStage BusSendStage, TArray<FAtomSoundSourceBusSendInfo>& OutSends) const
{
	if (Sound)
	{
		// Get the base sends
		Sound->GetSoundSourceBusSends(BusSendStage, OutSends);

		// Loop through the overrides, which may append or override the existing send
		for (const FAtomSoundSourceBusSendInfo& SendInfo : SourceBusSendsOverride[(int32)BusSendStage])
		{
			bool bOverridden = false;
			for (FAtomSoundSourceBusSendInfo& OutSendInfo : OutSends)
			{
				const bool bSameSourceBus = (OutSendInfo.SoundSourceBus != nullptr) && (OutSendInfo.SoundSourceBus == SendInfo.SoundSourceBus);
				const bool bSameAudioBus = (OutSendInfo.AudioBus != nullptr) && (OutSendInfo.AudioBus == SendInfo.AudioBus);

				if (bSameSourceBus || bSameAudioBus)
				{
					OutSendInfo.SendLevel = SendInfo.SendLevel;
					bOverridden = true;
					break;
				}
			}

			if (!bOverridden)
			{
				OutSends.Add(SendInfo);
			}
		}
	}
}

bool FAtomActiveSound::HasNewSourceBusSends() const
{
	return bHasNewSourceBusSends;
}

TArray<TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>> const& FAtomActiveSound::GetNewSourceBusSends() const
{
	return NewSourceBusSends;
}

void FAtomActiveSound::ResetNewSourceBusSends()
{
	NewSourceBusSends.Empty();
	bHasNewSourceBusSends = false;
}

void FAtomActiveSound::SetNewModulationRouting(const FAtomSoundModulationRoutingSettings& NewRouting)
{
	ModulationRouting = NewRouting;
	bModulationRoutingUpdated = true;
}

void FAtomActiveSound::SetMultiplePositions(const TArray<FTransform>& InPositions, EAtomMultiPositionType InMultiPositionType)
{
	if (Sound)
	{
		MultiPositionType = InMultiPositionType;
		MultiPositions = InPositions;
	}
}

void FAtomActiveSound::GetMultiplePositions(TArray<FTransform>& OutPositions)
{
	if (Sound)
	{
		if (MultiPositionType == EAtomMultiPositionType::SingleSource && !MultiPositions.IsEmpty())
		{
			OutPositions.Reset();
			OutPositions.Add(MultiPositions[0]);
		}
		else
		{
			OutPositions = MultiPositions;
		}
	}
}

void FAtomActiveSound::Stop()
{
	if (AtomRuntime)
	{
		AtomRuntime->AddSoundToStop(this);
	}
}

int32 FAtomActiveSound::FindClosestListener(const TArray<struct FAtomListener>& InListeners) const
{
	return AtomRuntime ? AtomRuntime->FindClosestListenerIndex(Transform, InListeners) : INDEX_NONE;
}

int32 FAtomActiveSound::FindClosestListener() const
{
	return AtomRuntime ? AtomRuntime->FindClosestListenerIndex(Transform) : INDEX_NONE;
}

void FAtomActiveSound::GetConcurrencyHandles(TArray<FAtomConcurrencyHandle>& OutConcurrencyHandles) const
{
	OutConcurrencyHandles.Reset();

	const UCriWareCoreSettings* AtomSettings = GetDefault<UCriWareCoreSettings>();
	check(AtomSettings);

	if (ConcurrencySet.Num() > 0)
	{
		for (const UAtomConcurrency* Concurrency : ConcurrencySet)
		{
			if (Concurrency)
			{
				OutConcurrencyHandles.Emplace(*Concurrency);
			}
		}
	}
	else if (Sound)
	{
		Sound->GetConcurrencyHandles(OutConcurrencyHandles);
	}
	else if (const UAtomConcurrency* DefaultConcurrency = AtomSettings->GetDefaultSoundConcurrency())
	{
		OutConcurrencyHandles.Emplace(*DefaultConcurrency);
	}
}

bool FAtomActiveSound::GetConcurrencyFadeDuration(float& OutFadeDuration) const
{
	OutFadeDuration = -1.0f;
	TArray<FAtomConcurrencyHandle> Handles;
	GetConcurrencyHandles(Handles);
	for (FAtomConcurrencyHandle& Handle : Handles)
	{
		// Resolution rules that don't support eviction (effectively requiring a sound to start before culling)
		// can spam if a looping ActiveSound isn't active longer than a virtualization update period, which
		// can happen when a concurrency group is maxed and constantly evicting.  If the voice steal fade time is particularly
		// long, this can flood the active sound count. Therefore, only use the voice steal fade time if the sound has been
		// active for a sufficient period of time.
		if (!Handle.Settings.IsEvictionSupported() && IsLooping() && FMath::IsNearlyZero(PlaybackTimeNonVirtualized, 0.1f))
		{
			OutFadeDuration = 0.0f;
			return false;
		}

		OutFadeDuration = OutFadeDuration < 0.0f
			? Handle.Settings.VoiceStealReleaseTime
			: FMath::Min(Handle.Settings.VoiceStealReleaseTime, OutFadeDuration);
	}

	// Negative if no handles are found, so return no fade required.
	if (OutFadeDuration <= 0.0f)
	{
		OutFadeDuration = 0.0f;
		return false;
	}

	return true;
}

void FAtomActiveSound::SetAisacControl(const FAtomAisacParameter& Parameter)
{
	// Override modulation value if modulation route already included in active sound
	for (FAtomAisacParameter& Param : AisacParametersOverride)
	{
		if (Param.Control.Name == Parameter.Control.Name)
		{
			Param.Value = Parameter.Value;
			return;
		}
	}

	// Otherwise, add it to the AISAC modulation overrides
	AisacParametersOverride.Add(Parameter);
}

void FAtomActiveSound::GetAisacControls(TArray<FAtomAisacParameter>& OutParameters) const
{
	// Loop through the sound aisac values, which may append existing controls

	// N/A
	
	// Loop through the overrides, which may append or override the existing send
	for (auto& Param : AisacParametersOverride)
	{
		bool bOverridden = false;
		for (FAtomAisacParameter& OutParam : OutParameters)
		{
			if (OutParam.Control.Name == Param.Control.Name)
			{
				OutParam.Value = Param.Value;
				bOverridden = true;
				break;
			}
		}

		if (!bOverridden)
		{
			FAtomAisacParameter OutParam;
			OutParam.Control = Param.Control;
			OutParam.Value = Param.Value;
			OutParameters.Add(OutParam);
		}
	}
}

void FAtomActiveSound::SetCueSelectorLabel(const FAtomSelectorParam& SelectorParam)
{
	// Override selector label if already included in active sound
	for (FAtomSelectorParam& Param : CueSelectorParams)
	{
		if (Param.Name == SelectorParam.Name)
		{
			Param.Label = SelectorParam.Label;
			return;
		}
	}

	// Otherwise, add it to the selectors
	CueSelectorParams.Add(SelectorParam);
}

void FAtomActiveSound::RemoveCueSelectorLabel(const FAtomSelectorParam& SelectorParam)
{
	CueSelectorParams.RemoveAll([SelectorParam](const FAtomSelectorParam& Param)
	{ 
		return Param.Name == SelectorParam.Name;
	});
}

void FAtomActiveSound::ClearAllCueSelectorLabels()
{
	CueSelectorParams.Reset();
}

void FAtomActiveSound::GetCueSelectorParameters(TArray<FAtomSelectorParam>& OutParameters) const
{
	OutParameters.Append(CueSelectorParams);
}

void FAtomActiveSound::SetCueNextBlockIndex(int32 BlockIndex)
{
	CueNextBlockIndex = BlockIndex;
}

void FAtomActiveSound::SetCueBeatSyncOffset(int32 Offset)
{
	CueBeatSyncOffset = Offset;
}

void FAtomActiveSound::UpdatePlaybackInstances(TArray<FAtomPlaybackInstance*>& InPlaybackInstances, const float DeltaTime)
{
	// Reset whether or not the active sound is playing audio.
	bIsPlayingAudio = false;

	// Reset the active sound's min current pitch value. This is updated as sounds try to play and determine their pitch values.
	MinCurrentPitch = 1.0f;

	// Early outs.
	if (!AtomRuntime || Sound == nullptr || !Sound->IsPlayable())
	{
#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomDebug, Warning, TEXT("UpdatePlaybackInstances -> Sound skipped because not playable"));
#endif
		return;
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_AtomFindNearestLocation);
		ClosestListenerIndex = AtomRuntime->FindClosestListenerIndex(Transform);
	}

	// update ex player events
	bool bIsInstancePlayerFinished = true;
	if (InstancePlayer.IsValid())
	{
		// update virtualization
		// may add instances to active sound
		// check if instance still playing 
		if (InstancePlayer->Update())
		{
			bIsInstancePlayerFinished = false;
		}
	}

	FocusData.PriorityHighest = 1.0f;

	FAtomSoundParseParameters ParseParams;
	ParseParams.Transform = Transform;
	ParseParams.StartTime = RequestedStartTime;

	// Report back to component if necessary once initial fade is complete
	const bool bIsInitFade = PlaybackTimeNonVirtualized < ComponentVolumeFader.GetFadeDuration();
	if (bIsInitFade)
	{
		bool bWasFading = ComponentVolumeFader.IsFadingIn();
		ComponentVolumeFader.Update(DeltaTime);
		const bool bIsFading = ComponentVolumeFader.IsFading();
		if (bWasFading && !bIsFading)
		{
			FAtomThread::RunCommandOnGameThread([AtomComponentID = GetAtomComponentID()]()
			{
				if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentID))
				{
					AtomComponent->SetFadeInComplete();
				}
			});
		}
	}
	else
	{
		ComponentVolumeFader.Update(DeltaTime);
	}

	// Reset Fader when fade-out or fade-in is over
	// as this can cause ping-ponging to occur with virtualization and loops
	if (bVirtualizedDueToMaxConcurrency && FadeOut == EFadeOut::Concurrency && !ComponentVolumeFader.IsFadingOut())
	{
		FadeOut = EFadeOut::None;
		ComponentVolumeFader.SetVolume(1.0f);
	}
	else if (bIsRealizingWithFadeIn && !ComponentVolumeFader.IsFadingIn())
	{
		bIsRealizingWithFadeIn = false;
	}

	ParseParams.VolumeMultiplier = GetVolume();

	ParseParams.Priority = Priority;
	//ParseParams.Pitch *= GetPitch() * Sound->GetPitchMultiplier();  // UE
	ParseParams.Pitch = GetPitch() * Sound->GetPitchMultiplier(); // Atom - not node based sounds
	ParseParams.bEnableLowPassFilter = bEnableLowPassFilter;
	ParseParams.LowPassFilterFrequency = LowPassFilterFrequency;
	ParseParams.bEnableHighPassFilter = bEnableHighPassFilter;
	ParseParams.HighPassFilterFrequency = HighPassFilterFrequency;
	ParseParams.bEnableAmplitudeEnvelope = bEnableAmplitudeEnvelope;
	ParseParams.AmplitudeEnvelope = AmplitudeEnvelope;
	ParseParams.SourceVoiceEffect = SourceVoiceEffect;
	ParseParams.bIsPaused = bIsPaused;

	ParseParams.SoundRack = GetSoundRack();
	GetAtomBusSends(ParseParams.SoundBusSends);

	ParseParams.CategoryNames = CategoryNames;
	ParseParams.SoundClass = GetSoundClass();

	ParseParams.AdditionalAisacPatchNames = AdditionalAisacPatchNames;
	GetAisacControls(ParseParams.AisacControlParams);

	GetCueSelectorParameters(ParseParams.CueSelectorParams);
	ParseParams.CueNextBlockIndex = CueNextBlockIndex;
	ParseParams.CueBeatSyncOffset = CueBeatSyncOffset;

	ParseParams.MultiPositionType = MultiPositionType;
	GetMultiplePositions(ParseParams.MultiPositions);

	ParseParams.bEnableSourceBusSends = Sound->bEnableSourceBusSends;
	ParseParams.bEnableSoundRack = Sound->bEnableBaseRack;
	ParseParams.bEnableSoundBusSends = Sound->bEnableBusSends;

	for (int32 BusSendStage = 0; BusSendStage < (int32)EAtomBusSendStage::Count; ++BusSendStage)
	{
		GetSourceBusSends((EAtomBusSendStage)BusSendStage, ParseParams.SourceBusSends[BusSendStage]);
	}

	// Set up the base source effect chain.
	//ParseParams.SourceEffectChain = GetSourceEffectChain();

	// Setup the envelope attack and release times
	ParseParams.EnvelopeFollowerAttackTime = EnvelopeFollowerAttackTime;
	ParseParams.EnvelopeFollowerReleaseTime = EnvelopeFollowerReleaseTime;

	if (bApplyInteriorVolumes)
	{
		// Additional inside/outside processing for ambient sounds
		// If we aren't in a world there is no interior volumes to be handled.
		const bool bNeedsInteriorUpdate = (!bGotInteriorSettings || (ParseParams.Transform.GetTranslation() - LastLocation).SizeSquared() > KINDA_SMALL_NUMBER);
		const bool bUseActiveSoundUpdate = GatherInteriorDataFromIAtomActiveSoundUpdateCVar != 0;

		// Gather data from interior spaces
		if (bNeedsInteriorUpdate)
		{
			if (bUseActiveSoundUpdate)
			{
				AtomRuntime->GatherInteriorData(*this, ParseParams);
			}

			bGotInteriorSettings = true;
		}

		// Apply data to the wave instances
		if (bUseActiveSoundUpdate)
		{
			AtomRuntime->ApplyInteriorSettings(*this, ParseParams);
		}
	}

	// for velocity-based effects like doppler
	if (DeltaTime > 0.f)
	{
		ParseParams.Velocity = (ParseParams.Transform.GetTranslation() - LastLocation) / DeltaTime;
		LastLocation = ParseParams.Transform.GetTranslation();
	}

	TArray<FAtomPlaybackInstance*> ThisSoundsPlaybackInstances;

	// Recurse nodes, have SoundWave's create new wave instances and update bFinished unless we finished fading out.
	bFinished = true;
	if (FadeOut == EFadeOut::None || FadeOut == EFadeOut::Concurrency || ComponentVolumeFader.IsActive())
	{
		//bool bReverbSendLevelWasSet = false;
		if (bHasAttenuationSettings)
		{
			UpdateAttenuation(DeltaTime, ParseParams, ClosestListenerIndex);
			//bReverbSendLevelWasSet = true;
		}
		//else
		//{
		//	ParseParams.ReverbSendMethod = EReverbSendMethod::Manual;
		//	if (ParseParams.SoundClass)
		//	{
		//		ParseParams.ManualReverbSendLevel = ParseParams.SoundClass->Properties.Default2DReverbSendAmount;
		//	}
		//	else
		//	{
		//		ParseParams.ManualReverbSendLevel = AudioDevice->GetDefaultReverbSendLevel();
		//	}
		//}

		Sound->Parse(AtomRuntime, 0, *this, ParseParams, ThisSoundsPlaybackInstances);

		// Track this active sound's min pitch value. This is used to scale it's possible duration value.
		if (ParseParams.Pitch < MinCurrentPitch)
		{
			MinCurrentPitch = ParseParams.Pitch;
		}
	}

	if (bFinished)
	{
		bFinished = bIsInstancePlayerFinished;
	}

	if (bFinished)
	{
#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomDebug, Warning, TEXT("UpdatePlaybackInstances -> Sound finished"));
#endif
		AtomRuntime->AddSoundToStop(this);
	}
	else if (ThisSoundsPlaybackInstances.Num() > 0)
	{
		// Let the playback instance know that this active sound is stopping. 
		// This will result in the playback instance getting a lower sort for voice prioritization.
		if (IsStopping())
		{
#ifdef ATOM_SOUND_DEV_DEBUG
			UE_LOG(LogCriWareAtomDebug, Warning, TEXT("UpdatePlaybackInstances -> Sound stopping"));
#endif
			for (FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
			{
				PlaybackInstance->SetStopping(true);
			}
		}

		// each wave instance needs its own copy of the quantization command
		for (FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
		{
			check(PlaybackInstance);

			if (!PlaybackInstance->QuantizedRequestData && QuantizedRequestData.QuantizedCommandPtr)
			{
				// shallow copy of the FQuartzQuantizedRequestData struct
				PlaybackInstance->QuantizedRequestData = MakeUnique<Atom::FQuartzQuantizedRequestData>(QuantizedRequestData);

				// manually deep copy the QuantizedCommandPtr object itself
				PlaybackInstance->QuantizedRequestData->QuantizedCommandPtr = QuantizedRequestData.QuantizedCommandPtr->GetDeepCopyOfDerivedObject();
			}

			// each wave instance needs its own copy of the source buffer listener.
			//PlaybackInstance->SourceBufferListener = SourceBufferListener;
			//PlaybackInstance->bShouldSourceBufferListenerZeroBuffer = bShouldSourceBufferListenerZeroBuffer;
		}

		// If the concurrency volume is negative (as set by ConcurrencyManager on creation),
		// skip updating as its been deemed unnecessary
		if (VolumeConcurrency >= 0.0f)
		{
			// Now that we have this sound's active wave instances, lets find the loudest active wave instance to represent the "volume" of this active sound
			VolumeConcurrency = 0.0f;
			for (const FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
			{
				check(PlaybackInstance);

				float PlaybackInstanceVolume = PlaybackInstance->GetVolumeWithDistanceAndOcclusionAttenuation() * PlaybackInstance->GetDynamicVolume();
				if (PlaybackInstanceVolume > VolumeConcurrency)
				{
					VolumeConcurrency = PlaybackInstanceVolume;
				}
			}

			// Remove concurrency volume scalars as this can cause ping-ponging to occur with virtualization and loops
			// utilizing concurrency with rules that don't support eviction (removal from concurrency system prior to playback).
			const float VolumeScale = GetTotalConcurrencyVolumeScale();
			if (VolumeScale > UE_SMALL_NUMBER)
			{
				VolumeConcurrency /= VolumeScale;
			}
			else
			{
				VolumeConcurrency = 0.0f;
			}

			// Remove concurrency volume fader as this can cause ping-ponging to occur with virtualization and loops
			if (bIsRealizingWithFadeIn || (bVirtualizedDueToMaxConcurrency && FadeOut == EFadeOut::Concurrency))
			{
				const float FadeVolume = ComponentVolumeFader.GetVolume();
				if (FadeVolume > UE_SMALL_NUMBER)
				{
					VolumeConcurrency /= FadeVolume;
				}
				else
				{
					VolumeConcurrency = 0.0f;
				}
			}
		}

		// Check to see if we need to broadcast the envelope value of sounds playing with this active sound
		if (AtomComponentID > 0)
		{
			if (bUpdateMultiEnvelopeValue)
			{
				int32 NumPlaybackInstances = ThisSoundsPlaybackInstances.Num();

				// Add up the envelope value for every wave instance so we get a sum of the envelope value for all sources.
				float EnvelopeValueSum = 0.0f;
				float MaxEnvelopeValue = 0.0f;
				for (FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
				{
					const float PlaybackInstanceEnvelopeValue = PlaybackInstance->GetEnvelopeValue();
					EnvelopeValueSum += PlaybackInstanceEnvelopeValue;
					MaxEnvelopeValue = FMath::Max(PlaybackInstanceEnvelopeValue, MaxEnvelopeValue);
				}

				// Now divide by the number of instances to get the average
				float AverageEnvelopeValue = EnvelopeValueSum / NumPlaybackInstances;
				uint64 AtomComponentIDCopy = AtomComponentID;
				FAtomThread::RunCommandOnGameThread([AtomComponentIDCopy, AverageEnvelopeValue, MaxEnvelopeValue, NumPlaybackInstances]()
				{
					if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentIDCopy))
					{
						if (AtomComponent->OnAtomSoundMultiEnvelopeValue.IsBound())
						{
							AtomComponent->OnAtomSoundMultiEnvelopeValue.Broadcast(AverageEnvelopeValue, MaxEnvelopeValue, NumPlaybackInstances);
						}

						if (AtomComponent->OnAtomSoundMultiEnvelopeValueNative.IsBound())
						{
							AtomComponent->OnAtomSoundMultiEnvelopeValueNative.Broadcast(AtomComponent, AverageEnvelopeValue, MaxEnvelopeValue, NumPlaybackInstances);
						}
					}
				});
			}

			if (bUpdatePlaybackTime)
			{
				TMap<uint32, float> PlaybackInstancePlaybackTimes;

				// Update each of the playabck instances playback time based on delta time and the wave instances pitch value.
				for (FAtomPlaybackInstance* PlaybackInstance : ThisSoundsPlaybackInstances)
				{
					PlaybackInstance->PlaybackTime += DeltaTime * PlaybackInstance->Pitch;

					// For looping sounds, we need to check the wrapping condition
					if (PlaybackInstance->LoopingMode != EAtomLoopingMode::LoopNever)
					{
						float Duration = PlaybackInstance->SoundData->Duration;
						if (PlaybackInstance->PlaybackTime > Duration)
						{
							PlaybackInstance->PlaybackTime = 0.0f;
						}
					}
					PlaybackInstancePlaybackTimes.Add(PlaybackInstance->SoundData->GetUniqueID(), PlaybackInstance->PlaybackTime);
				}
				uint64 AtomComponentIDCopy = AtomComponentID;
				FAtomThread::RunCommandOnGameThread([AtomComponentIDCopy, PlaybackInstancePlaybackTimes]()
				{
					if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(AtomComponentIDCopy))
					{
						AtomComponent->SetPlaybackTimes(PlaybackInstancePlaybackTimes);
					}
				});
			}
		}
	}

#if ENABLE_ATOM_DEBUG
	if (DebugColor == FColor::Black)
	{
		DebugColor = FColor::MakeRandomColor();
	}
	Atom::FAtomDebugger::DrawDebugInfo(*this, ThisSoundsPlaybackInstances, DeltaTime);
#endif // ENABLE_ATOM_DEBUG

	InPlaybackInstances.Append(ThisSoundsPlaybackInstances);
}

void FAtomActiveSound::MarkPendingDestroy(bool bDestroyNow)
{
	check(AtomRuntime);

	bool bWasStopping = bIsStopping;

	if (Sound && !bIsStopping)
	{
		int32* PlayCount = AtomRuntime ? Sound->CurrentPlayCount.Find(AtomRuntime->GetAtomRuntimeID()) : nullptr;
		if (PlayCount)
		{
			*PlayCount = FMath::Max(*PlayCount - 1, 0);
			if (*PlayCount == 0)
			{
				Sound->CurrentPlayCount.Remove(AtomRuntime->GetAtomRuntimeID());
			}
		}
	}

	TArray<FAtomPlaybackInstance*> ToDelete;
	for (TPair<UPTRINT, FAtomPlaybackInstance*> PlaybackInstanceIt : PlaybackInstances)
	{
		FAtomPlaybackInstance* PlaybackInstance = PlaybackInstanceIt.Value;

		// Stop the owning sound source
		FAtomSource* Source = AtomRuntime->GetSoundSource(PlaybackInstance);
		if (Source)
		{
			bool bStopped = false;
			//if (AudioDevice->IsStoppingVoicesEnabled())
			{
				if (bDestroyNow /* || !AudioDevice->GetNumFreeSources()*/)
				{
					Source->StopNow();
					bStopped = true;
				}
			}

			if (!bStopped)
			{
				Source->Stop();
			}
		}

		/*if (!bIsStopping)
		{
			// Dequeue subtitles for this sounds on the game thread
			DECLARE_CYCLE_STAT(TEXT("FGameThreadAudioTask.KillSubtitles"), STAT_AudioKillSubtitles, STATGROUP_TaskGraphTasks);
			const PTRINT WaveInstanceID = (PTRINT)WaveInstance;
			FAudioThread::RunCommandOnGameThread([WaveInstanceID]()
				{
					FSubtitleManager::GetSubtitleManager()->KillSubtitles(WaveInstanceID);
				}, GET_STATID(STAT_AudioKillSubtitles));
		}*/

		if (Source)
		{
			if (!Source->IsStopping())
			{
				Source->StopNow();

				ToDelete.Add(PlaybackInstance);
			}
			else
			{
				// This source is doing a fade out, so stopping. Can't remove the wave instance yet.
				bIsStopping = true;
			}
		}
		else
		{
			// Have a playback instance but no source.
			ToDelete.Add(PlaybackInstance);
		}
	}

	for (FAtomPlaybackInstance* PlaybackInstance : ToDelete)
	{
		RemovePlaybackInstance(PlaybackInstance->PlaybackInstanceHash);
	}

	if (bDestroyNow)
	{
		bIsStopping = false;
	}

	if (!bWasStopping)
	{
		AtomRuntime->RemoveActiveSound(this);
	}
}

bool FAtomActiveSound::UpdateStoppingSources(uint64 CurrentTick, bool bEnsureStopped)
{
	// If we're not stopping, then just return true (we can be cleaned up)
	if (!bIsStopping)
	{
		return true;
	}

	bIsStopping = false;

	TArray<FAtomPlaybackInstance*> ToDelete;
	for (TPair<UPTRINT, FAtomPlaybackInstance*> PlaybackInstanceIt : PlaybackInstances)
	{
		FAtomPlaybackInstance* PlaybackInstance = PlaybackInstanceIt.Value;

		// Some playback instances in the list here may be nullptr if some sounds have already stopped or didn't need to do a stop
		if (PlaybackInstance)
		{
			// Stop the owning sound source
			FAtomSource* Source = AtomRuntime->GetSoundSource(PlaybackInstance);
			if (Source)
			{
				// The source has finished (totally faded out)
				if (Source->IsFinished() || bEnsureStopped)
				{
					Source->StopNow();

					// Delete the playback instance
					ToDelete.Add(PlaybackInstance);
				}
				else
				{
					// We are not finished yet so touch it
					Source->LastUpdate = CurrentTick;
					Source->LastHeardUpdate = CurrentTick;

					// flag that we're still stopping (return value)
					bIsStopping = true;
				}
			}
			else
			{
				// We have a playback instance but no source for it, so just delete it.
				ToDelete.Add(PlaybackInstance);
			}
		}
	}

	for (FAtomPlaybackInstance* PlaybackInstance : ToDelete)
	{
		RemovePlaybackInstance(PlaybackInstance->PlaybackInstanceHash);
	}

	// Return true to indicate this active sound can be cleaned up
	// If we've reached this point, all sound waves have stopped so we can clear this wave instance out.
	if (!bIsStopping)
	{
		check(PlaybackInstances.Num() == 0);
		return true;
	}

	// still stopping!
	return false;
}

FAtomPlaybackInstance* FAtomActiveSound::FindPlaybackInstance(const UPTRINT PlaybackInstanceHash)
{
	return PlaybackInstances.FindRef(PlaybackInstanceHash);
}

void FAtomActiveSound::RemovePlaybackInstance(const UPTRINT PlaybackInstanceHash)
{
	if (FAtomPlaybackInstance* PlaybackInstance = PlaybackInstances.FindRef(PlaybackInstanceHash))
	{
		PlaybackInstances.Remove(PlaybackInstanceHash);
		delete PlaybackInstance;
	}
}

void FAtomActiveSound::OcclusionTraceDone(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	// Look for any results that resulted in a blocking hit
	bool bFoundBlockingHit = false;
	for (const FHitResult& HitResult : TraceDatum.OutHits)
	{
		if (HitResult.bBlockingHit)
		{
			bFoundBlockingHit = true;
			break;
		}
	}

	FAsyncTraceDetails TraceDetails;
	if (TraceToActiveSoundMap.RemoveAndCopyValue(TraceHandle, TraceDetails))
	{
		if (FAtomRuntimeManager* AtomRuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(TraceDetails.AtomRuntimeID))
			{
				FAtomActiveSound* ActiveSound = TraceDetails.ActiveSound;

				DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.OcclusionTraceDone"), STAT_OcclusionTraceDone, STATGROUP_AtomThreadCommands);

				FAtomThread::RunCommandOnAtomThread([AtomRuntime, ActiveSound, bFoundBlockingHit]()
				{
					AtomRuntime->NotifyActiveSoundOcclusionTraceDone(ActiveSound, bFoundBlockingHit);
				}, GET_STATID(STAT_OcclusionTraceDone));
			}
		}
	}
}

void FAtomActiveSound::CheckOcclusion(const FVector ListenerLocation, const FVector SoundLocation, const FAtomAttenuationSettings* AttenuationSettingsPtr)
{
	check(AttenuationSettingsPtr);
	check(AttenuationSettingsPtr->bEnableOcclusion);

	float InterpolationTime = 0.0f;

	// If occlusion is disabled by cvar, we're always going to be not occluded
	if (AtomOcclusionDisabledCvar == 1)
	{
		bIsOccluded = false;
	}
	else
	{
		if (!bAsyncOcclusionPending && (PlaybackTime - LastOcclusionCheckTime) > OcclusionCheckInterval)
		{
			LastOcclusionCheckTime = PlaybackTime;

			const bool bUseComplexCollisionForOcclusion = AttenuationSettingsPtr->bUseComplexCollisionForOcclusion;
			const ECollisionChannel OcclusionTraceChannel = AttenuationSettingsPtr->OcclusionTraceChannel;

			if (!bHasCheckedOcclusion)
			{
				FCollisionQueryParams Params(SCENE_QUERY_STAT(AtomOcclusion), bUseComplexCollisionForOcclusion);
				if (OwnerID > 0)
				{
					Params.AddIgnoredActor(OwnerID);
				}

				if (UWorld* WorldPtr = World.Get())
				{
					// LineTraceTestByChannel is generally threadsafe, but there is a very narrow race condition here
					// if World goes invalid before the scene lock and queries begin.
					bIsOccluded = WorldPtr->LineTraceTestByChannel(SoundLocation, ListenerLocation, OcclusionTraceChannel, Params);
				}
			}
			else
			{
				bAsyncOcclusionPending = true;

				const uint32 SoundOwnerID = OwnerID;
				TWeakObjectPtr<UWorld> SoundWorld = World;
				FAsyncTraceDetails TraceDetails;
				TraceDetails.AtomRuntimeID = AtomRuntime->GetAtomRuntimeID();
				TraceDetails.ActiveSound = this;

				FAtomThread::RunCommandOnGameThread([SoundWorld, SoundLocation, ListenerLocation, OcclusionTraceChannel, SoundOwnerID, bUseComplexCollisionForOcclusion, TraceDetails]
				{
					if (UWorld* WorldPtr = SoundWorld.Get())
					{
						FCollisionQueryParams Params(SCENE_QUERY_STAT(AtomOcclusion), bUseComplexCollisionForOcclusion);
						if (SoundOwnerID > 0)
						{
							Params.AddIgnoredActor(SoundOwnerID);
						}

						FTraceHandle TraceHandle = WorldPtr->AsyncLineTraceByChannel(EAsyncTraceType::Test, SoundLocation, ListenerLocation, OcclusionTraceChannel, Params, FCollisionResponseParams::DefaultResponseParam, &ActiveSoundTraceDelegate);
						TraceToActiveSoundMap.Add(TraceHandle, TraceDetails);
					}
				});
			}
		}

		// Update the occlusion values
		if (bHasCheckedOcclusion)
		{
			InterpolationTime = AttenuationSettingsPtr->OcclusionInterpolationTime;
		}
		bHasCheckedOcclusion = true;
	}

	if (bIsOccluded)
	{
		if (CurrentOcclusionFilterFrequency.GetTargetValue() > AttenuationSettingsPtr->OcclusionLowPassFilterFrequency)
		{
			CurrentOcclusionFilterFrequency.Set(AttenuationSettingsPtr->OcclusionLowPassFilterFrequency, InterpolationTime);
		}

		if (CurrentOcclusionVolumeAttenuation.GetTargetValue() > AttenuationSettingsPtr->OcclusionVolumeAttenuation)
		{
			CurrentOcclusionVolumeAttenuation.Set(AttenuationSettingsPtr->OcclusionVolumeAttenuation, InterpolationTime);
		}
	}
	else
	{
		CurrentOcclusionFilterFrequency.Set(MAX_FILTER_FREQUENCY, InterpolationTime);
		CurrentOcclusionVolumeAttenuation.Set(1.0f, InterpolationTime);
	}

	const float DeltaTime = FApp::GetDeltaTime();
	CurrentOcclusionFilterFrequency.Update(DeltaTime);
	CurrentOcclusionVolumeAttenuation.Update(DeltaTime);
}

FAtomPlaybackInstance& FAtomActiveSound::AddPlaybackInstance(const UPTRINT PlaybackInstanceHash)
{
	FAtomPlaybackInstance* PlaybackInstance = new FAtomPlaybackInstance(PlaybackInstanceHash, *this);
	PlaybackInstances.Add(PlaybackInstanceHash, PlaybackInstance);
	return *PlaybackInstance;
}

/*void FAtomActiveSound::ApplyRadioFilter(const FSoundParseParameters& ParseParams)
{
	check(AudioDevice);
	if (AudioDevice->GetMixDebugState() != DEBUGSTATE_DisableRadio)
	{
		// Make sure the radio filter is requested
		if (ParseParams.SoundClass)
		{
			const float RadioFilterVolumeThreshold = ParseParams.VolumeMultiplier * ParseParams.SoundClass->Properties.RadioFilterVolumeThreshold;
			if (RadioFilterVolumeThreshold > KINDA_SMALL_NUMBER)
			{
				bApplyRadioFilter = (ParseParams.Volume < RadioFilterVolumeThreshold);
			}
		}
	}
	else
	{
		bApplyRadioFilter = false;
	}

	bRadioFilterSelected = true;
}*/

float FAtomActiveSound::GetVolume() const
{
	const float Volume = VolumeMultiplier * ComponentVolumeFader.GetVolume() * GetTotalConcurrencyVolumeScale();
	return Sound ? Volume * Sound->GetVolumeMultiplier() : Volume;
}

float FAtomActiveSound::GetTotalConcurrencyVolumeScale() const
{
	float OutVolume = 1.0f;

	for (const TPair<FAtomConcurrencyGroupId, FAtomConcurrencySoundData>& ConcurrencyPair : ConcurrencyGroupData)
	{
		OutVolume *= ConcurrencyPair.Value.GetVolume();
	}

	return OutVolume;
}

void FAtomActiveSound::UpdateConcurrencyVolumeScalars(const float DeltaTime)
{
	for (TPair<FAtomConcurrencyGroupId, FAtomConcurrencySoundData>& ConcurrencyPair : ConcurrencyGroupData)
	{
		ConcurrencyPair.Value.Update(DeltaTime);
	}
}

void FAtomActiveSound::CollectAttenuationShapesForVisualization(TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap) const
{
	if (bHasAttenuationSettings)
	{
        if (bIsAttenuationSettingsOverridden)
        {
            AttenuationSettings.CollectAttenuationShapesForVisualization(ShapeDetailsMap);
        }
        else if (SoundAttenuation)
        {
            SoundAttenuation->Attenuation.CollectAttenuationShapesForVisualization(ShapeDetailsMap);
        }
	}
}

float FAtomActiveSound::GetAttenuationFrequency(const FAtomAttenuationSettings* Settings, const FAtomAttenuationListenerData& ListenerData, const FVector2D& FrequencyRange, const FRuntimeFloatCurve& CustomCurve)
{
	float OutputFrequency = 0.0f;

	// If the frequency mapping is the same no matter what, no need to do any mapping
	if (FrequencyRange.X == FrequencyRange.Y)
	{
		OutputFrequency = FrequencyRange.X;
	}
	// If the transition band is instantaneous, just set it to before/after frequency value
	else if (Settings->LPFRadiusMin == Settings->LPFRadiusMax)
	{
		if (ListenerData.AttenuationDistance > Settings->LPFRadiusMin)
		{
			OutputFrequency = FrequencyRange.Y;
		}
		else
		{
			OutputFrequency = FrequencyRange.X;
		}
	}
	else if (Settings->AbsorptionMethod == EAtomAirAbsorptionMethod::Linear)
	{
		FVector2D AbsorptionDistanceRange = { Settings->LPFRadiusMin, Settings->LPFRadiusMax };

		// Do log-scaling if we've been told to do so. This applies a log function to perceptually smooth filter frequency between target frequency ranges
		if (Settings->bEnableLogFrequencyScaling)
		{
			OutputFrequency = Atom::GetLogFrequencyClamped(ListenerData.AttenuationDistance, AbsorptionDistanceRange, FrequencyRange);
		}
		else
		{
			OutputFrequency = FMath::GetMappedRangeValueClamped(AbsorptionDistanceRange, FrequencyRange, ListenerData.AttenuationDistance);
		}
	}
	else
	{
		// In manual absorption mode, the frequency ranges are interpreted as a true "range"
		FVector2D ActualFreqRange(FMath::Min(FrequencyRange.X, FrequencyRange.Y), FMath::Max(FrequencyRange.X, FrequencyRange.Y));

		// Normalize the distance values to a value between 0 and 1
		FVector2f AbsorptionDistanceRange = { Settings->LPFRadiusMin, Settings->LPFRadiusMax };
		check(AbsorptionDistanceRange.Y != AbsorptionDistanceRange.X);
		const float Alpha = FMath::Clamp<float>((ListenerData.AttenuationDistance - AbsorptionDistanceRange.X) / (AbsorptionDistanceRange.Y - AbsorptionDistanceRange.X), 0.0f, 1.0f);

		// Perform the curve mapping
		const float MappedFrequencyValue = FMath::Clamp<float>(CustomCurve.GetRichCurveConst()->Eval(Alpha), 0.0f, 1.0f);

		if (Settings->bEnableLogFrequencyScaling)
		{
			// Use the mapped value in the log scale mapping
			OutputFrequency = Atom::GetLogFrequencyClamped(MappedFrequencyValue, FVector2D(0.0f, 1.0f), ActualFreqRange);
		}
		else
		{
			// Do a straight linear interpolation between the absorption frequency ranges
			OutputFrequency = FMath::GetMappedRangeValueClamped(FVector2f(0.0f, 1.0f), FVector2f(ActualFreqRange), MappedFrequencyValue);
		}
	}

	return FMath::Clamp<float>(OutputFrequency, MIN_FILTER_FREQUENCY, MAX_FILTER_FREQUENCY);
}

bool FAtomActiveSound::GetAlwaysPlay() const
{
	if (bAlwaysPlay)
	{
		return true;
	}

	for (const TPair<UPTRINT, FAtomPlaybackInstance*>& Pair : PlaybackInstances)
	{
		if (Pair.Value && Pair.Value->SoundClass)
		{
			const FAtomSoundClassProperties* SoundClassProperties = AtomRuntime->GetSoundClassCurrentProperties(Pair.Value->SoundClass);
			check(SoundClassProperties);
			if (SoundClassProperties->bAlwaysPlay)
			{
				return true;
			}
		}
	}

	return false;
}

float FAtomActiveSound::GetHighestPriority(bool bIgnoreAlwaysPlay) const
{
	if (!bIgnoreAlwaysPlay)
	{
		if (GetAlwaysPlay())
		{
			static constexpr float AlwaysPlayPriority = TNumericLimits<float>::Max();
			return AlwaysPlayPriority;
		}
	}

	const float HighestPriority = FocusData.PriorityHighest * FocusData.PriorityScale;
	return FMath::Clamp(HighestPriority, 0.0f, 100.0f);
}

void FAtomActiveSound::UpdateFocusData(float DeltaTime, const FAtomAttenuationListenerData& ListenerData, FAtomAttenuationFocusData* OutFocusData)
{
	FAtomAttenuationFocusData* FocusDataToUpdate = OutFocusData ? OutFocusData : &FocusData;

	AtomRuntime->GetAzimuth(ListenerData, FocusDataToUpdate->Azimuth, FocusDataToUpdate->AbsoluteAzimuth);

	FocusDataToUpdate->DistanceScale = 1.0f;
	FocusDataToUpdate->PriorityScale = 1.0f;

	if (!ListenerData.AttenuationSettings->bEnableListenerFocus)
	{
		return;
	}

	if (!ListenerData.AttenuationSettings->bSpatialize)
	{
		return;
	}

	const FAtomGlobalFocusSettings& FocusSettings = AtomRuntime->GetGlobalFocusSettings();
	const float TargetFocusFactor = AtomRuntime->GetFocusFactor(FocusDataToUpdate->Azimuth, *ListenerData.AttenuationSettings);

	// Enabling InitializeFocusVolumeBeforeInterpCVar will fix a bug related to focus factor always needing to interpolate up from 0 to the correct value.
	// Could affect game mix, so enable with caution.
	const bool bShouldInterpolateFocusFactor = (!FocusDataToUpdate->bFirstFocusUpdate || !AtomInitializeFocusFactorOnFirstUpdateCVar);

	// User opt-in for focus interpolation
	if (ListenerData.AttenuationSettings->bEnableFocusInterpolation && bShouldInterpolateFocusFactor)
	{
		// Determine which interpolation speed to use (attack/release)
		float InterpSpeed;
		if (TargetFocusFactor <= FocusDataToUpdate->FocusFactor)
		{
			InterpSpeed = ListenerData.AttenuationSettings->FocusAttackInterpSpeed;
		}
		else
		{
			InterpSpeed = ListenerData.AttenuationSettings->FocusReleaseInterpSpeed;
		}

		FocusDataToUpdate->FocusFactor = FMath::FInterpTo(FocusDataToUpdate->FocusFactor, TargetFocusFactor, DeltaTime, InterpSpeed);
	}
	else
	{
		// Set focus directly to target value
		FocusDataToUpdate->FocusFactor = TargetFocusFactor;
	}

	// No longer first update
	FocusDataToUpdate->bFirstFocusUpdate = false;

	// Scale the volume-weighted priority scale value we use for sorting this sound for voice-stealing
	FocusDataToUpdate->PriorityScale = ListenerData.AttenuationSettings->GetFocusPriorityScale(FocusSettings, FocusDataToUpdate->FocusFactor);
	FocusDataToUpdate->DistanceScale = ListenerData.AttenuationSettings->GetFocusDistanceScale(FocusSettings, FocusDataToUpdate->FocusFactor);
	FocusDataToUpdate->VolumeScale = ListenerData.AttenuationSettings->GetFocusAttenuation(FocusSettings, FocusDataToUpdate->FocusFactor);
}

void FAtomActiveSound::ParseAttenuation(FAtomSoundParseParameters& OutParseParams, int32 ListenerIndex, const FAtomAttenuationSettings& InAttenuationSettings)
{
	UpdateAttenuation(0.0f, OutParseParams, ListenerIndex, &InAttenuationSettings);
}

void FAtomActiveSound::UpdateAttenuation(float DeltaTime, FAtomSoundParseParameters& ParseParams, int32 ListenerIndex, const FAtomAttenuationSettings* SettingsAttenuationNode)
{
    // We default to using the copied off "overridden" settings (or default constructed settings)
    const FAtomAttenuationSettings* Settings = &AttenuationSettings;

    // Get the attenuation settings to use for this application to the active sound
    // Use the passed-in attenuation settings
    if (!bIsAttenuationSettingsOverridden)
    {
        if (SettingsAttenuationNode)
        {
            Settings = SettingsAttenuationNode;
        }
        // We fallback to using the asset's settings directly
        else if (SoundAttenuation)
        {
            Settings = &SoundAttenuation->Attenuation;
        }
    }

	// Reset Focus data and recompute if necessary
	FAtomAttenuationFocusData FocusDataToApply;
	FocusDataToApply.PriorityHighest = FocusData.PriorityHighest;

	/*if (Settings->bEnableReverbSend)
	{
		ParseParams.ReverbSendMethod = Settings->ReverbSendMethod;
		ParseParams.ManualReverbSendLevel = Settings->ManualReverbSendLevel;
		ParseParams.CustomReverbSendCurve = Settings->CustomReverbSendCurve;
		ParseParams.ReverbSendLevelRange = { Settings->ReverbWetLevelMin, Settings->ReverbWetLevelMax };
		ParseParams.ReverbSendLevelDistanceRange = { Settings->ReverbDistanceMin, Settings->ReverbDistanceMax };
	}*/

	if (Settings->bEnableBusSends)
	{
		for (const FAtomAttenuationBusSendSettings& SendSettings : Settings->BusSendSettings)
		{
			if (SendSettings.Bus)
			{
				ParseParams.BusSendSettings.Add(SendSettings);
			}
		}
	}

	check(AtomRuntime);
	auto ListenerData = FAtomAttenuationListenerData::Create(*AtomRuntime, ClosestListenerIndex, Transform, AttenuationSettings);

	// Apply priority attenuation if it's enabled
	if (Settings->bEnablePriorityAttenuation)
	{
		float PriorityScale = 1.0f;
		if (Settings->PriorityAttenuationMethod == EAtomPriorityAttenuationMethod::Manual)
		{
			PriorityScale = Settings->ManualPriorityAttenuation;
		}
		else
		{
			const float Denom = FMath::Max(Settings->PriorityAttenuationDistanceMax - Settings->PriorityAttenuationDistanceMin, SMALL_NUMBER);
			const float Alpha = FMath::Clamp((ListenerData.ListenerToSoundDistance - Settings->PriorityAttenuationDistanceMin) / Denom, 0.0f, 1.0f);

			if (Settings->PriorityAttenuationMethod == EAtomPriorityAttenuationMethod::Linear)
			{
				PriorityScale = FMath::Max(FMath::Lerp(Settings->PriorityAttenuationMin, Settings->PriorityAttenuationMax, Alpha), 0.0f);
			}
			else
			{
				PriorityScale = FMath::Max(Settings->CustomPriorityAttenuationCurve.GetRichCurveConst()->Eval(Alpha), 0.0f);
			}
		}

		ParseParams.Priority *= FMath::Max(PriorityScale, 0.0f);
	}

	if (Settings->bSpatialize || Settings->bEnableListenerFocus)
	{
		// Feed prior focus factor on update to allow for proper interpolation.
		FocusDataToApply.FocusFactor = FocusData.FocusFactor;

		// Feed first update flag
		FocusDataToApply.bFirstFocusUpdate = FocusData.bFirstFocusUpdate;

		// Update azimuth angles prior to updating focus as it uses this in calculating
		// in and out of focus values.
		UpdateFocusData(DeltaTime, ListenerData, &FocusDataToApply);

		// Update FocusData's highest priority copy prior to applying cached scalar immediately following
		// to avoid applying scalar twice
		FocusDataToApply.PriorityHighest = FMath::Max(FocusDataToApply.PriorityHighest, ParseParams.Priority);

		ParseParams.Volume *= FocusDataToApply.VolumeScale;
		ParseParams.Priority *= FocusDataToApply.PriorityScale;
		if (Settings->bSpatialize)
		{
			ParseParams.AttenuationDistance = ListenerData.AttenuationDistance;
			ParseParams.ListenerToSoundDistance = ListenerData.ListenerToSoundDistance;
			ParseParams.ListenerToSoundDistanceForPanning = ListenerData.ListenerToSoundDistanceForPanning;
			ParseParams.AbsoluteAzimuth = FocusDataToApply.AbsoluteAzimuth;
		}
	}

	// Attenuate the volume based on the model. Note we don't apply the distance attenuation to the sound.
	// to feed source audio through source effects and buses.
	// ==>> Atom will applies its own distance-based attenuation as a separate stage using its positionning internal system 
	if (Settings->bAttenuate)
	{
		// Apply the sound-class-based distance scale
		if (ParseParams.SoundClass)
		{
			FAtomSoundClassDynamicProperties* DynamicSoundClassProperties = AtomRuntime->GetSoundClassDynamicProperties(ParseParams.SoundClass);
			if (DynamicSoundClassProperties)
			{
				FocusDataToApply.DistanceScale *= FMath::Max(DynamicSoundClassProperties->AttenuationScaleParam.GetValue(), 0.0f);
			}
		}

		/*if (Settings->AttenuationShape == EAttenuationShape::Sphere)
		{
			// Update attenuation data in-case it hasn't been updated
			ParseParams.DistanceAttenuation *= Settings->AttenuationEval(ListenerData.AttenuationDistance, Settings->FalloffDistance, FocusDataToApply.DistanceScale);
		}
		else
		{
			const FVector ListenerTranslation = ListenerData.ListenerTransform.GetTranslation();
			ParseParams.DistanceAttenuation *= Settings->Evaluate(ParseParams.Transform, ListenerTranslation, FocusDataToApply.DistanceScale);
		}*/

		// ===> GET info from ATOM SYSTEM HERE -> debug_get_distance

		//FCriWareApi::criAtomExDebug_GetResourcesInfo ....
	}


	// Only do occlusion traces if the sound is audible and we're not using a occlusion plugin
	if (Settings->bEnableOcclusion)
	{
		// If we've got a occlusion plugin settings, then the plugin will handle occlusion calculations
		/*if (Settings->PluginSettings.OcclusionPluginSettingsArray.Num())
		{
			UClass* PluginClass = GetAudioPluginCustomSettingsClass(EAudioPlugin::OCCLUSION);
			if (PluginClass)
			{
				for (UOcclusionPluginSourceSettingsBase* SettingsBase : Settings->PluginSettings.OcclusionPluginSettingsArray)
				{
					if (SettingsBase != nullptr && SettingsBase->IsA(PluginClass))
					{
						ParseParams.OcclusionPluginSettings = SettingsBase;
						break;
					}
				}
			}
		}
		else*/ if (ParseParams.Volume > 0.0f && !AtomRuntime->IsAtomRuntimeMuted())
		{
			check(ClosestListenerIndex != INDEX_NONE);
			FVector ListenerPosition;
			const bool bAllowOverride = false;
			AtomRuntime->GetListenerPosition(ClosestListenerIndex, ListenerPosition, bAllowOverride);
			CheckOcclusion(ListenerPosition, ParseParams.Transform.GetTranslation(), Settings);

			// Apply the volume attenuation due to occlusion (using the interpolating dynamic parameter)
			ParseParams.OcclusionAttenuation = CurrentOcclusionVolumeAttenuation.GetValue();
			ParseParams.bIsOccluded = bIsOccluded;
			ParseParams.OcclusionFilterFrequency = CurrentOcclusionFilterFrequency.GetValue();
		}
	}

	// Figure out which attenuation settings to use ( DEPRECATED in unreal )
	/*if (Settings->PluginSettings.SpatializationPluginSettingsArray.Num() > 0)
	{
		UClass* PluginClass = GetAudioPluginCustomSettingsClass(EAudioPlugin::SPATIALIZATION);
		if (PluginClass)
		{
			for (USpatializationPluginSourceSettingsBase* SettingsBase : Settings->PluginSettings.SpatializationPluginSettingsArray)
			{
				if (SettingsBase != nullptr && SettingsBase->IsA(PluginClass))
				{
					ParseParams.SpatializationPluginSettings = SettingsBase;
					break;
				}
			}
		}
	}

	if (Settings->PluginSettings.ReverbPluginSettingsArray.Num() > 0)
	{
		UClass* PluginClass = GetAudioPluginCustomSettingsClass(EAudioPlugin::REVERB);
		if (PluginClass)
		{
			for (UReverbPluginSourceSettingsBase* SettingsBase : Settings->PluginSettings.ReverbPluginSettingsArray)
			{
				if (SettingsBase != nullptr && SettingsBase->IsA(PluginClass))
				{
					ParseParams.ReverbPluginSettings = SettingsBase;
					break;
				}
			}
		}
	}*/

	if (Settings->PluginSettings.SourceDataOverridePluginSettingsArray.Num() > 0)
	{
		UClass* PluginClass = GetAtomPluginCustomSettingsClass(EAtomPlugin::SourceDataOverride);
		if (PluginClass)
		{
			for (UAtomSourceDataOverridePluginSourceSettingsBase* SettingsBase : Settings->PluginSettings.SourceDataOverridePluginSettingsArray)
			{
				if (SettingsBase != nullptr && SettingsBase->IsA(PluginClass))
				{
					ParseParams.SourceDataOverridePluginSettings = SettingsBase;
					break;
				}
			}
		}
	}

	if (Settings->PluginSettings.RuntimePluginSettingsArray.Num() > 0)
	{
		TArray<UClass*> PluginClasses = GetAtomRuntimePluginCustomSettingsClasses();
		for (UClass* PluginClass : PluginClasses)
		{
			if (PluginClass)
			{
				for (UAtomRuntimePluginSettingsBase* SettingsBase : Settings->PluginSettings.RuntimePluginSettingsArray)
				{
					if (SettingsBase != nullptr && SettingsBase->IsA(PluginClass))
					{
						ParseParams.RuntimePluginSettingsArray.Add(SettingsBase);
						break;
					}
				}
			}
		}
	}

	// Attenuate with the absorption filter if necessary
	if (Settings->bAttenuateWithLPF)
	{
		FVector2D AbsorptionLowPassFrequencyRange = { Settings->LPFFrequencyAtMin, Settings->LPFFrequencyAtMax };
		FVector2D AbsorptionHighPassFrequencyRange = { Settings->HPFFrequencyAtMin, Settings->HPFFrequencyAtMax };
		const float AttenuationLowpassFilterFrequency = GetAttenuationFrequency(Settings, ListenerData, AbsorptionLowPassFrequencyRange, Settings->CustomLowpassAirAbsorptionCurve);
		const float AttenuationHighPassFilterFrequency = GetAttenuationFrequency(Settings, ListenerData, AbsorptionHighPassFrequencyRange, Settings->CustomHighpassAirAbsorptionCurve);

		// Only apply the attenuation filter frequency if it results in a lower attenuation filter frequency than is already being used by ParseParams (the struct pass into the sound cue node tree)
		// This way, subsequently chained attenuation nodes in a sound cue will only result in the lowest frequency of the set.
		if (AttenuationLowpassFilterFrequency < ParseParams.AttenuationLowpassFilterFrequency)
		{
			ParseParams.AttenuationLowpassFilterFrequency = AttenuationLowpassFilterFrequency;
		}

		// Same with high pass filter frequency
		if (AttenuationHighPassFilterFrequency > ParseParams.AttenuationHighpassFilterFrequency)
		{
			ParseParams.AttenuationHighpassFilterFrequency = AttenuationHighPassFilterFrequency;
		}
	}

	ParseParams.NonSpatializedRadiusStart = Settings->NonSpatializedRadiusStart;
	ParseParams.NonSpatializedRadiusEnd = Settings->NonSpatializedRadiusEnd;
	ParseParams.NonSpatializedRadiusMode = Settings->NonSpatializedRadiusMode;
	ParseParams.StereoSpread = Settings->StereoSpread;
	ParseParams.bApplyNormalizationToStereoSounds = Settings->bApplyNormalizationToStereoSounds;
	ParseParams.bUseSpatialization |= Settings->bSpatialize;
	ParseParams.bEnableSourceDataOverride |= Settings->bEnableSourceDataOverride;

	// Check the binaural radius to determine if we're going to HRTF spatialize, cache the result
	if (bIsFirstAttenuationUpdate)
	{
		bStartedWithinNonBinauralRadius = ListenerData.ListenerToSoundDistance < Settings->BinauralRadius;
	}

	if (bStartedWithinNonBinauralRadius)
	{
		ParseParams.SpatializationMethod = EAtomSpatializationAlgorithm::Default;
	}
	else
	{
		if (Settings->SpatializationAlgorithm == EAtomSpatializationAlgorithm::Default && AtomRuntime->IsHRTFEnabledForAll())
		{
			ParseParams.SpatializationMethod = EAtomSpatializationAlgorithm::Binaural;
		}
		else if (Settings->SpatializationAlgorithm == EAtomSpatializationAlgorithm::Binaural && AtomRuntime->IsHRTFDisabled())
		{
			ParseParams.SpatializationMethod = EAtomSpatializationAlgorithm::Default;
		}
		else
		{
			ParseParams.SpatializationMethod = Settings->SpatializationAlgorithm;
		}

		ParseParams.bSpatializationIsExternalSend = true;// AtomRuntime->GetCurrentSpatializationPluginInterfaceInfo().bSpatializationIsExternalSend;;
	}
	
	// If not overriding from a node, set focus data
	if (!SettingsAttenuationNode)
	{
		FocusData = FocusDataToApply;
	}
	// Make sure to always update highest priority
	else
	{
		FocusData.PriorityHighest = FocusDataToApply.PriorityHighest;
	}

	bIsFirstAttenuationUpdate = false;
}

void FAtomActiveSound::ExecuteOnSoundFinishedCallback(bool bEndReached, const TFunction<void(bool)>& OnSoundFinished, const ENamedThreads::Type& CallbackThread)
{
	if (CallbackThread == ENamedThreads::AnyThread)
	{
		OnSoundFinished(bEndReached);
	}
	else
	{
		// Dispatch an async notify.
		AsyncTask(CallbackThread, [OnSoundFinished, bEndReached]()
		{
			OnSoundFinished(bEndReached);
		});
	}
}
