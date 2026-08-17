
#include "Atom/AtomSoundProxy.h"

#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundClass.h"
#include "Atom/AtomAttenuation.h"
 
UAtomSoundProxy::UAtomSoundProxy(const FObjectInitializer& ObjectInitializer)
	: UAtomSoundBase(ObjectInitializer)
	, bIsPlayableGuard(false)
	, bAttenuationSettingsToApplyGuard(false)
    , bGetMaxDistanceGuard(false)
	, bGetDurationGuard(false)
	, bGetVirtualizationModeGuard(false)
	, bGetResourceGuard(false)
	, bGetSoundClassGuard(false)
	, bCommonGuard(false)
	, bLeafSound(false)
{}
 
bool UAtomSoundProxy::IsPlayable() const
{
	// Guard against reentrant calls.
	if (bIsPlayableGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsPlayable - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bIsPlayableGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsPlayable() : false;
}

bool UAtomSoundProxy::IsStreaming(const TCHAR* PlatformName) const
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsStreaming - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsStreaming(PlatformName) : false;
}

bool UAtomSoundProxy::IsSeekable() const
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsSeekable - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsSeekable() : false;
}

const FAtomAttenuationSettings* UAtomSoundProxy::GetAttenuationSettingsToApply() const
{
	// overrided
	if (const FAtomAttenuationSettings* Settings = Super::GetAttenuationSettingsToApply())
	{
		return Settings;
	}

	// Guard against reentrant calls.
	if (bAttenuationSettingsToApplyGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetAttenuationSettingsToApply - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return nullptr;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bAttenuationSettingsToApplyGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetAttenuationSettingsToApply() : nullptr;
}

EAtomVirtualizationMode UAtomSoundProxy::GetVirtualizationMode() const
{
	// Override
	if (bOverrideVirtualizationMode)
	{
		return Super::GetVirtualizationMode();
	}

	// Guard against reentrant calls.
	if (bGetVirtualizationModeGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetVirtualizationMode - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return EAtomVirtualizationMode::Normal;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bGetVirtualizationModeGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetVirtualizationMode() : EAtomVirtualizationMode::Normal;
}

bool UAtomSoundProxy::IsPlayWhenSilent() const
{
	// Override
	if (bOverrideVirtualizationMode)
	{
		// todo check with leaf type !! 
		return Super::IsPlayWhenSilent();
	}

	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsPlayWhenSilent - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsPlayWhenSilent() : false;
}

float UAtomSoundProxy::GetMaxDistance() const
{
	// Override
	if (const FAtomAttenuationSettings* Settings = Super::GetAttenuationSettingsToApply())
	{
		return Super::GetMaxDistance();
	}

    // Guard against reentrant calls.
	if (bGetMaxDistanceGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetMaxDistance - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingMaxDistanceGuard(bGetMaxDistanceGuard, true);

    UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetMaxDistance() : 0.0f;
}

float UAtomSoundProxy::GetDuration() const
{
	// Guard against reentrant calls.
	if (bGetDurationGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetDuration - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingDurationGuard(bGetDurationGuard, true);

    UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetDuration() : 0.0f;
}

float UAtomSoundProxy::GetVolumeMultiplier()
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetVolumeMultiplier - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return 1.0f;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetVolumeMultiplier() : 1.0f;
}

float UAtomSoundProxy::GetPitchMultiplier()
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetPitchMultiplier - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return 1.0f;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetPitchMultiplier() : 1.0f;
}

bool UAtomSoundProxy::ShouldApplyInteriorVolumes()
{
	// Override
	if (UAtomSoundClass* SoundClass = Super::GetSoundClass())
	{
		return SoundClass->Properties.bApplyAmbientVolumes;
	}

	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::ShouldApplyInteriorVolumes - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->ShouldApplyInteriorVolumes() : false;
}

IAtomSoundResource* UAtomSoundProxy::GetSoundResource()
{
	// Guard against reentrant calls.
	if (bGetResourceGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetSoundResource - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return nullptr;
	}
	TGuardValue<bool> GettingResourceGuard(bGetResourceGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetSoundResource() : nullptr;
}

void UAtomSoundProxy::GetAllSoundResources(TArray<IAtomSoundResource*>& Resources)
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetAllSoundResources - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return;
	}
	TGuardValue<bool> GettingResourceGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	if (CurrentProxy != nullptr)
	{
		CurrentProxy->GetAllSoundResources(Resources);
	}
}

bool UAtomSoundProxy::IsLooping() const
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsLooping - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsLooping() : false;
}

bool UAtomSoundProxy::IsOneShot() const
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::IsOneShot - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->IsOneShot() : false;
}

void UAtomSoundProxy::Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& OutPlaybackInstances)
{
	if (UAtomSoundBase* CurrentProxy = GetSound())
    {
        CurrentProxy->Parse(AtomRuntime, PlaybackInstanceHash, ActiveSound, ParseParams, OutPlaybackInstances);
    }
}

UAtomSoundClass* UAtomSoundProxy::GetSoundClass() const
{
	// overrided
	if (UAtomSoundClass* SoundClass = Super::GetSoundClass())
	{
		return SoundClass;
	}

	// Guard against reentrant calls.
	if (bGetSoundClassGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetSoundClass - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return nullptr;
	}
	TGuardValue<bool> GettingResourceGuard(bGetSoundClassGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetSoundClass() : nullptr;
}

UAtomRackBase* UAtomSoundProxy::GetAtomRack() const
{
	// Override
	if (UAtomRackBase* Rack = Super::GetAtomRack())
	{
		return Rack;
	}

	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetAtomRack - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return nullptr;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr && CurrentProxy->bEnableBaseRack) ? CurrentProxy->GetAtomRack() : nullptr;
}

void UAtomSoundProxy::GetAtomBusSends(TArray<FAtomSoundToBusSend>& OutSends) const
{
	// Override
	Super::GetAtomBusSends(OutSends);

	if (OutSends.IsEmpty())
	{
		// Guard against reentrant calls.
		if (bCommonGuard)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetAtomRack - Reentrant calls are not supported. Asset: %s"), *GetPathName());
			return;
		}
		TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

		if (UAtomSoundBase* CurrentProxy = GetSound())
		{
			if (CurrentProxy->bEnableBusSends)
			{
				CurrentProxy->GetAtomBusSends(OutSends);
			}
		}
	}
}

void UAtomSoundProxy::GetSoundSourceBusSends(EAtomBusSendStage BusSendStage, TArray<FAtomSoundSourceBusSendInfo>& OutSends) const
{
	// Override
	Super::GetSoundSourceBusSends(BusSendStage, OutSends);

	if (OutSends.IsEmpty())
	{
		// Guard against reentrant calls.
		if (bCommonGuard)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetSoundSourceBusSends - Reentrant calls are not supported. Asset: %s"), *GetPathName());
			return;
		}
		TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

		if (UAtomSoundBase* CurrentProxy = GetSound())
		{
			if (CurrentProxy->bEnableSourceBusSends)
			{
				CurrentProxy->GetSoundSourceBusSends(BusSendStage, OutSends);
			}
		}
	}
}

void UAtomSoundProxy::GetConcurrencyHandles(TArray<FAtomConcurrencyHandle>& OutConcurrencyHandles) const
{
	OutConcurrencyHandles.Reset();
	if (bOverrideConcurrency)
	{
		OutConcurrencyHandles.Add(ConcurrencyOverrides);
	}
	else if (!ConcurrencySet.IsEmpty())
	{
		for (const UAtomConcurrency* Concurrency : ConcurrencySet)
		{
			if (Concurrency)
			{
				OutConcurrencyHandles.Emplace(*Concurrency);
			}
		}
	}
	else if (UAtomSoundBase* CurrentProxy = GetSound())
	{
		// Guard against reentrant calls.
		if (bCommonGuard)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetConcurrencyHandles - Reentrant calls are not supported. Asset: %s"), *GetPathName());
			return;
		}
		TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

		CurrentProxy->GetConcurrencyHandles(OutConcurrencyHandles);
	}
}

float UAtomSoundProxy::GetPriority() const
{
	// Override
	if (bOverridePriority)
	{
		return Super::GetPriority();
	}

	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetPriority - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return 0;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetPriority() : 0;
}

const FAtomSoundModulationRoutingSettings& UAtomSoundProxy::GetModulationSettings() const
{
	if (bOverrideModulationSettings)
	{
		return Super::GetModulationSettings();
	}

	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetModulationSettings - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return ModulationSettings;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetModulationSettings() : ModulationSettings;
}

#if WITH_EDITOR
bool UAtomSoundProxy::GetWaveInfo(FAtomResourceHandle& InHandle, FAtomWaveInfo& OutWaveInfo) const
{
	// Guard against reentrant calls.
	if (bCommonGuard)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetWaveInfo - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return false;
	}
	TGuardValue<bool> GettingIsPlayableGuard(bCommonGuard, true);

	UAtomSoundBase* CurrentProxy = GetSound();
	return (CurrentProxy != nullptr) ? CurrentProxy->GetWaveInfo(InHandle, OutWaveInfo) : false;
}
#endif

UAtomSoundBase* UAtomSoundProxy::GetSound() const
{
	return DynamicProxy ? DynamicProxy : Proxy;
}

UAtomSoundBase* UAtomSoundProxy::GetLeafSound() const
{
	// Guard against reentrant calls.
	if (bLeafSound)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("UAtomSoundProxy::GetLeafSound - Reentrant calls are not supported. Asset: %s"), *GetPathName());
		return nullptr;
	}
	TGuardValue<bool> ValidatingGuard(bLeafSound, true);

	UAtomSoundBase* Sound = GetSound();
	if (UAtomSoundProxy* ProxySound = Cast<UAtomSoundProxy>(Sound))
	{
		Sound = ProxySound->GetLeafSound();
	}
	return Sound;
}

bool UAtomSoundProxy::IsProxyValid() const
{
	return GetLeafSound() != nullptr;
}


void UAtomSoundProxy::SetDynamicSound(UAtomSoundBase* InProxy)
{
	DynamicProxy = InProxy;
}


#if WITH_EDITOR
void UAtomSoundProxy::SetSound(UAtomSoundBase* InProxy)
{
	Proxy = InProxy;
}
#endif

// Customized sound

bool UMySound::IsPlayable() const { return Cue || Cue2; }
float UMySound::GetMaxDistance() const { return Cue ? Cue->GetMaxDistance() : 0.0f; }
float UMySound::GetDuration() const { return Cue ? Cue->GetDuration() : 0.0f; }

void UMySound::Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& OutPlaybackInstances)
{
	if (Cue)
	{
		Cue->Parse(AtomRuntime, 0, ActiveSound, ParseParams, OutPlaybackInstances);
	}

	OutPlaybackInstances.Empty();

	// cue2 starts when cue1 finished in any way stoped or ended
	if (auto PlaybackInstance = ActiveSound.FindPlaybackInstance(0)) // cue1
	{
		if (PlaybackInstance->bIsStarted && PlaybackInstance->bIsFinished)
		{
			if (Cue2)
			{
				ActiveSound.RemovePlaybackInstance(0);
				Cue2->Parse(AtomRuntime, 1, ActiveSound, ParseParams, OutPlaybackInstances);
			}
		}
	}

	if (auto PlaybackInstance = ActiveSound.FindPlaybackInstance(1)) // cue2
	{
		Cue2->Parse(AtomRuntime, 1, ActiveSound, ParseParams, OutPlaybackInstances);
	}

	OutPlaybackInstances.Empty();

	for (auto& Pair : ActiveSound.GetPlaybackInstances())
	{
		FAtomPlaybackInstance* PlaybackInstance = Pair.Value;
		OutPlaybackInstances.Add(PlaybackInstance);
	}
}
