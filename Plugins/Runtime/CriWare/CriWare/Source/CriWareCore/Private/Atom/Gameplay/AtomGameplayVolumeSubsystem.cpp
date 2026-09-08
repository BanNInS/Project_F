
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"

#include "Misc/CoreMisc.h"

#include "CriWareCorePrivate.h"
#include "Atom/Gameplay/AtomGameplayVolumeLogs.h"
#include "Atom/Gameplay/AtomGameplayVolumeComponent.h"
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"
#include "Atom/Gameplay/AtomGameplayVolumeMutator.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"

namespace AtomGameplayVolumeConsoleVariables
{
	int32 bEnabled = 1;
	FAutoConsoleVariableRef CVarEnabled(
		TEXT("atom.AtomGameplayVolumes.Enabled"),
		bEnabled,
		TEXT("Toggles the Atom Gameplay Volume System on or off.\n0: Disable, 1: Enable (default)"),
		ECVF_Default);

	int32 bUpdateListeners = 1;
	FAutoConsoleVariableRef CVarUpdateListeners(
		TEXT("atom.AtomGameplayVolumes.Listeners.AllowUpdate"),
		bUpdateListeners,
		TEXT("Allows updating of listeners.\n0: Disable, 1: Enable (default)"),
		ECVF_Default);

	float MinUpdateRate = 0.016f;
	float UpdateRate = 0.05f;
	FAutoConsoleVariableRef CVarUpdateInterval(
		TEXT("atom.AtomGameplayVolume.UpdateRate"),
		UpdateRate,
		TEXT("How frequently we check for listener changes with respect to Atom gameplay volumes, in seconds."),
		ECVF_Default);

	float UpdateRateJitterDelta = 0.025f;
	FAutoConsoleVariableRef CVarUpdateRateJitterDelta(
		TEXT("atom.AtomGameplayVolume.UpdateRate.JitterDelta"),
		UpdateRateJitterDelta,
		TEXT("A random delta to add to update rate to avoid performance heartbeats."),
		ECVF_Default);

} // namespace AtomGameplayVolumeConsoleVariables

void FAtomGameplayActiveSoundInfo::Update(double ListenerInteriorStartTime)
{
	// If the interior settings have changed for the listener or the sound, we need to set
	// new interpolation targets.  However, we need to 'initialize' the sounds interior start time to match
	// the listener's interior start time, to prevent new sounds from interpolating until they change interior settings.
	if (FMath::IsNearlyZero(LastUpdateTime))
	{
		InteriorSettings.SetInteriorStartTime(ListenerInteriorStartTime);
	}

	if (LastUpdateTime < ListenerInteriorStartTime || LastUpdateTime < InteriorSettings.GetInteriorStartTime())
	{
		SourceInteriorVolume = CurrentInteriorVolume;
		SourceInteriorLPF = CurrentInteriorLPF;
		LastUpdateTime = FApp::GetCurrentTime();
	}

	InteriorSettings.UpdateInteriorValues();
}

void FAtomProxyMutatorSearchObject::SearchVolumes(const TArray<UAtomGameplayVolumeProxy*>& ProxyVolumes, FAtomProxyMutatorSearchResult& OutResult)
{
	check(IsInAtomThread());
	SCOPED_NAMED_EVENT(FAtomProxyMutatorSearchObject_SearchVolumes, FColor::Blue);

	OutResult.Reset();
	FAtomProxyMutatorPriorities MutatorPriorities;
	MutatorPriorities.PayloadType = PayloadType;
	MutatorPriorities.bFilterPayload = bFilterPayload;

	for (const UAtomGameplayVolumeProxy* ProxyVolume : ProxyVolumes)
	{
		if (!ProxyVolume || ProxyVolume->GetWorldID() != WorldID)
		{
			continue;
		}

		if (bFilterPayload && !ProxyVolume->HasPayloadType(PayloadType))
		{
			continue;
		}

		if (!ProxyVolume->ContainsPosition(Location))
		{
			continue;
		}

		if (bCollectMutators)
		{
			// We only need to calculate priorities for mutators if we're collecting them.
			ProxyVolume->FindMutatorPriority(MutatorPriorities);
		}

		OutResult.VolumeSet.Add(ProxyVolume->GetVolumeID());
	}

	// Use 'world settings' as a starting point
	if (AtomRuntimeHandle.IsValid())
	{
		AtomRuntimeHandle->GetDefaultAtomSettings(WorldID, OutResult.ReverbSettings, OutResult.InteriorSettings);
	}

	if (bCollectMutators)
	{
		for (const UAtomGameplayVolumeProxy* ProxyVolume : ProxyVolumes)
		{
			if (!ProxyVolume)
			{
				continue;
			}

			if (OutResult.VolumeSet.Contains(ProxyVolume->GetVolumeID()))
			{
				ProxyVolume->GatherMutators(MutatorPriorities, OutResult);
			}
		}
	}
}

void FAtomGameplayVolumeProxyInfo::Update(const TArray<FAtomGameplayVolumeListener>& VolumeListeners, FAtomGameplayProxyUpdateResult& OutResult)
{
	check(IsInAtomThread());

	PreviousProxies = MoveTemp(CurrentProxies);
	for (const int32 ListenerIndex : ListenerIndexes)
	{
		if (ListenerIndex <= VolumeListeners.Num())
		{
			CurrentProxies.Append(VolumeListeners[ListenerIndex].GetCurrentProxies());
		}
	}

	ListenerIndexes.Reset();

	// We've entered proxies that are in the current list, but not in previous
	OutResult.EnteredProxies = CurrentProxies.Difference(PreviousProxies);

	// We've exited proxies that are in the previous list, but not in current
	OutResult.ExitedProxies = PreviousProxies.Difference(CurrentProxies);
}

void FAtomGameplayVolumeProxyInfo::AddListenerIndex(int32 ListenerIndex)
{
	ListenerIndexes.Emplace(ListenerIndex);
}

bool FAtomGameplayVolumeProxyInfo::IsVolumeInCurrentList(uint32 VolumeID) const
{
	return CurrentProxies.Contains(VolumeID);
}

UAtomGameplayVolumeSubsystem::UAtomGameplayVolumeSubsystem()
	: UAtomEngineSubsystem()
{
}

bool UAtomGameplayVolumeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UAtomGameplayVolumeSubsystem::Deinitialize()
{
	AGVComponents.Reset();
}

void UAtomGameplayVolumeSubsystem::Update()
{
	check(IsInAtomThread());
	
	// We track and check our previous proxy count to ensure we get one update after all of the volumes have been removed
	if (AtomGameplayVolumeConsoleVariables::bEnabled == 0 || (KnownProxyIDs.Num() == 0 && PreviousProxyCount == 0))
	{
		return;
	}

	// limit updates for perf - not necessary to update every tick
	const float DeltaTime = FApp::GetDeltaTime();
	TimeSinceUpdate += DeltaTime;
	if (TimeSinceUpdate < NextUpdateDeltaTime)
	{
		return;
	}

	const float JitterDelta = FMath::RandRange(0.f, AtomGameplayVolumeConsoleVariables::UpdateRateJitterDelta);
	NextUpdateDeltaTime = FMath::Max(AtomGameplayVolumeConsoleVariables::UpdateRate + JitterDelta, AtomGameplayVolumeConsoleVariables::MinUpdateRate);
	TimeSinceUpdate = 0.f;

	if (bHasStaleProxy)
	{
		FAtomRuntimeHandle AtomRuntimeHandle = GetAtomRuntimeHandle();
		if (AtomRuntimeHandle.IsValid())
		{
			AtomRuntimeHandle->InvalidateCachedInteriorVolumes();
			bHasStaleProxy = false;
		}
	}

	if (AtomGameplayVolumeConsoleVariables::bUpdateListeners != 0)
	{
		UpdateFromListeners();
	}

	PreviousProxyCount = KnownProxyIDs.Num();
}

void UAtomGameplayVolumeSubsystem::GatherInteriorData(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams)
{
	if (AtomGameplayVolumeConsoleVariables::bEnabled == 0)
	{
		return;
	}

	check(IsInAtomThread());

	FAtomProxyMutatorSearchObject MutatorSearch;
	MutatorSearch.WorldID = ActiveSound.GetWorldID();
	MutatorSearch.Location = ParseParams.Transform.GetTranslation();
	MutatorSearch.PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound;
	MutatorSearch.AtomRuntimeHandle = GetAtomRuntimeHandle();

	GenerateVolumeProxyList();
	FAtomProxyMutatorSearchResult Result;
	MutatorSearch.SearchVolumes(TransientProxyList, Result);

	// Save info about this active sound for application.
	FAtomGameplayActiveSoundInfo& ActiveSoundInfo = ActiveSoundData.FindOrAdd(ActiveSound.GetInstanceID());
	ActiveSoundInfo.CurrentMutators = MoveTemp(Result.MatchingMutators);
	ActiveSoundInfo.InteriorSettings.Apply(Result.InteriorSettings);

	// We need to drop references, but we can at least prevent some allocs from using a member variable
	TransientProxyList.Reset();
}

static void AppendAisacControls(FAtomSoundParseParameters& ParseParams, TMap<FAtomAisacControl, float>& AisacControls)
{
	for (auto& [Control, Value] : AisacControls)
	{
		bool bFound = false;
		for (auto& ParamInfo : ParseParams.AisacControlParams)
		{
			if (ParamInfo.Control.ID == Control.ID)
			{
				bFound = true;
				// if exists, only update the value
				ParamInfo.Value = Value;
				break;
			}
		}

		if (!bFound)
		{
			// don't exist, append the modulation
			FAtomAisacParameter Param;
			Param.Control = Control;
			Param.Value = Value;
			ParseParams.AisacControlParams.Add(Param);
		}
	}
}

void UAtomGameplayVolumeSubsystem::ApplyInteriorSettings(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams)
{
	if (AtomGameplayVolumeConsoleVariables::bEnabled == 0)
	{
		return;
	}

	check(IsInAtomThread());

	int32 ListenerIndex = ActiveSound.GetClosestListenerIndex();
	if (ListenerIndex >= AGVListeners.Num())
	{
		return;
	}

	FAtomGameplayActiveSoundInfo* ActiveSoundInfo = ActiveSoundData.Find(ActiveSound.GetInstanceID());
	if (!ActiveSoundInfo)
	{
		return;
	}

	const FAtomGameplayVolumeListener& Listener = AGVListeners[ListenerIndex];
	ActiveSoundInfo->Update(Listener.GetInteriorSettings().GetInteriorStartTime());

	uint32 WorldID = ActiveSound.GetWorldID();
	FAtomProxyActiveSoundParams Params(*ActiveSoundInfo, Listener);
	Params.bAllowSpatialization = ActiveSound.bAllowSpatialization;
	Params.Transform = ActiveSound.Transform;
	Params.MultiPositionType = ActiveSound.MultiPositionType;
	Params.MultiPositions = ActiveSound.MultiPositions;

	for (const TSharedPtr<FAtomProxyVolumeMutator>& SoundMutator : ActiveSoundInfo->CurrentMutators)
	{
		if (SoundMutator.IsValid())
		{
			Params.bListenerInVolume = IsAnyListenerInVolume(WorldID, SoundMutator->VolumeID);
			SoundMutator->Apply(Params);
		}
	}

	// Update interior values
	Params.UpdateInteriorValues();
	ActiveSoundInfo->CurrentInteriorVolume = Params.SourceInteriorVolume;
	ActiveSoundInfo->CurrentInteriorLPF = Params.SourceInteriorLPF;

	// Apply to our parse params
	ParseParams.SoundBusSends.Append(Params.SoundBusSends);
	AppendAisacControls(ParseParams, Params.AisacControls);

	ParseParams.InteriorVolumeMultiplier = ActiveSoundInfo->CurrentInteriorVolume;
	ParseParams.AmbientZoneFilterFrequency = ActiveSoundInfo->CurrentInteriorLPF;
	if (Params.bSoundMovable)
	{
		ParseParams.Transform = Params.Transform;
		ParseParams.MultiPositionType = Params.MultiPositionType;
		ParseParams.MultiPositions = Params.MultiPositions;
	}
}

void UAtomGameplayVolumeSubsystem::NotifyActiveSoundDeleting(const FAtomActiveSound& ActiveSound)
{
	check(IsInAtomThread());
	if (ActiveSound.bApplyInteriorVolumes && ActiveSoundData.Num() > 0)
	{
		ActiveSoundData.Remove(ActiveSound.GetInstanceID());
	}
}

void UAtomGameplayVolumeSubsystem::AddVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent)
{
	if (!VolumeComponent)
	{
		UE_LOG(AtomGameplayVolumeLog, Verbose, TEXT("AtomGameplayVolumeSubsystem - Attempting to add invalid volume component!"));
		return;
	}
	
	if (!DoesSupportWorld(VolumeComponent->GetWorld()))
	{
		return;
	}

	const uint32 ComponentID = VolumeComponent->GetUniqueID();
	UAtomGameplayVolumeProxy* VolumeProxy = VolumeComponent->GetProxy();
	if (VolumeProxy && !AGVComponents.Contains(ComponentID))
	{
		VolumeProxy->InitFromComponent(VolumeComponent);
		AGVComponents.Emplace(ComponentID, VolumeComponent);
		const uint32 WorldID = VolumeProxy->GetWorldID();

		UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("AtomGameplayVolumeComponent %s [%08x] added"), *VolumeComponent->GetFName().ToString(), ComponentID);

		// Copy representation of volume to Atom thread
		FAtomRuntimeId CurrentRuntimeID = GetAtomRuntimeID();
		FAtomThread::RunCommandOnAtomThread([CurrentRuntimeID, ComponentID, WorldID]()
		{
				if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
				{
					FAtomRuntimeHandle RuntimeHandle = AtomRuntimeManager->GetAtomRuntime(CurrentRuntimeID);
					if (RuntimeHandle.IsValid())
					{
						if (UAtomGameplayVolumeSubsystem* AGVSubsystem = RuntimeHandle->GetSubsystem<UAtomGameplayVolumeSubsystem>())
						{
							AGVSubsystem->AddProxy(ComponentID, WorldID);
						}
					}
				}
		});
	}
}

void UAtomGameplayVolumeSubsystem::UpdateVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent)
{
	if (!VolumeComponent || !VolumeComponent->GetWorld())
	{
		UE_LOG(AtomGameplayVolumeLog, Verbose, TEXT("AtomGameplayVolumeSubsystem - Attempting to update invalid volume component!"));
		return;
	}

	const uint32 ComponentID = VolumeComponent->GetUniqueID();
	UAtomGameplayVolumeProxy* VolumeProxy = VolumeComponent->GetProxy();
	if (VolumeProxy && AGVComponents.Contains(ComponentID))
	{
		VolumeProxy->InitFromComponent(VolumeComponent);

		UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("AtomGameplayVolumeComponent %s [%08x] updated"), *VolumeComponent->GetFName().ToString(), ComponentID);

		// Update representation of volume on audio thread
		FAtomRuntimeId CurrentRuntimeID = GetAtomRuntimeID();
		FAtomThread::RunCommandOnAtomThread([CurrentRuntimeID, ComponentID]()
		{
			if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
			{
				FAtomRuntimeHandle RuntimeHandle = AtomRuntimeManager->GetAtomRuntime(CurrentRuntimeID);
				if (RuntimeHandle.IsValid())
				{
					if (UAtomGameplayVolumeSubsystem* AGVSubsystem = RuntimeHandle->GetSubsystem<UAtomGameplayVolumeSubsystem>())
					{
						AGVSubsystem->UpdateProxy(ComponentID);
					}
				}
			}
		});
	}
}

void UAtomGameplayVolumeSubsystem::RemoveVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent)
{
	if (!VolumeComponent || !VolumeComponent->GetWorld())
	{
		UE_LOG(AtomGameplayVolumeLog, Verbose, TEXT("AtomGameplayVolumeSubsystem - Attempting to remove invalid volume component!"));
		return;
	}

	const uint32 ComponentID = VolumeComponent->GetUniqueID();
	if (AGVComponents.Contains(ComponentID))
	{
		AGVComponents.Remove(ComponentID);
		UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("AtomGameplayVolumeComponent %s [%08x] removed"), *VolumeComponent->GetFName().ToString(), ComponentID);
	}

	// Remove representation of volume from Atom thread
	FAtomRuntimeId CurrentRuntimeID = GetAtomRuntimeID();
	FAtomThread::RunCommandOnAtomThread([CurrentRuntimeID, ComponentID]()
	{
		if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
		{
			FAtomRuntimeHandle RuntimeHandle = AtomRuntimeManager->GetAtomRuntime(CurrentRuntimeID);
			if (RuntimeHandle.IsValid())
			{
				if (UAtomGameplayVolumeSubsystem* AGVSubsystem = RuntimeHandle->GetSubsystem<UAtomGameplayVolumeSubsystem>())
				{
					AGVSubsystem->RemoveProxy(ComponentID);
				}
			}
		}
	});
}

UPrimitiveComponent* UAtomGameplayVolumeSubsystem::GetPrimitiveComponent(const UAtomGameplayVolumeComponent* VolumeComponent)
{
	if (const UAtomGameplayVolumePrimitiveComponentProxy* VolumePrimitiveProxy = Cast<const UAtomGameplayVolumePrimitiveComponentProxy>(VolumeComponent->GetProxy()))
	{
		return VolumePrimitiveProxy->GetPrimitiveComponent().Get();
	}

	return nullptr;
}

const UAtomGameplayVolumeComponent* UAtomGameplayVolumeSubsystem::GetVolumeComponent(uint32 VolumeID)
{
	if (AGVComponents.Contains(VolumeID))
	{
		return AGVComponents[VolumeID];
	}

	return nullptr;
}

bool UAtomGameplayVolumeSubsystem::DoesSupportWorld(UWorld* World) const
{
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		return true;
	}

	return false;
}

bool UAtomGameplayVolumeSubsystem::AddProxy(uint32 AtomGameplayVolumeID, uint32 WorldID)
{
	check(IsInAtomThread());

	// Make sure the component is in the map and non-null
	if (!AGVComponents.Contains(AtomGameplayVolumeID) || AGVComponents[AtomGameplayVolumeID] == nullptr)
	{
		return false;
	}

	// Make sure our proxy is valid
	if (AGVComponents[AtomGameplayVolumeID]->GetProxy() == nullptr)
	{
		return false;
	}

	if (KnownProxyIDs.Contains(AtomGameplayVolumeID))
	{
		UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("Attempting to add Proxy [%08x] multiple times"), AtomGameplayVolumeID);
		return false;
	}

	KnownProxyIDs.Add(AtomGameplayVolumeID);
	WorldProxyLists.FindOrAdd(WorldID);
	bHasStaleProxy = true;

	UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("Proxy [%08x] added"), AtomGameplayVolumeID);
	return true;
}

bool UAtomGameplayVolumeSubsystem::UpdateProxy(uint32 AtomGameplayVolumeID)
{
	check(IsInAtomThread());

	if (KnownProxyIDs.Contains(AtomGameplayVolumeID))
	{
		bHasStaleProxy = true;
		return true;
	}

	return false;
}

bool UAtomGameplayVolumeSubsystem::RemoveProxy(uint32 AtomGameplayVolumeID)
{
	check(IsInAtomThread());

	const int32 NumRemoved = KnownProxyIDs.Remove(AtomGameplayVolumeID);
	if (NumRemoved > 0)
	{
		bHasStaleProxy = true;
		UE_LOG(AtomGameplayVolumeLog, VeryVerbose, TEXT("Removed %d Proxies with Id [%08x]"), NumRemoved, AtomGameplayVolumeID);
		return true;
	}

	return false;
}

void UAtomGameplayVolumeSubsystem::GenerateVolumeProxyList()
{
	TransientProxyList.Reset(256);
	Algo::Transform(AGVComponents, TransientProxyList, [](const TPair<uint32, TObjectPtr<const UAtomGameplayVolumeComponent>>& Pair)
	{
		return Pair.Value ? Pair.Value->GetProxy() : nullptr;
	});
}

bool UAtomGameplayVolumeSubsystem::IsAnyListenerInVolume(uint32 WorldID, uint32 VolumeID) const
{
	// We test this by checking to see if the volume id provided is in our world's current proxy list
	if (const FAtomGameplayVolumeProxyInfo* ProxyInfo = WorldProxyLists.Find(WorldID))
	{
		return ProxyInfo->IsVolumeInCurrentList(VolumeID);
	}
	return false;
}

void UAtomGameplayVolumeSubsystem::UpdateComponentsFromProxyInfo(const FAtomGameplayProxyUpdateResult& ProxyResults) const
{
	check(IsInGameThread());
	for (uint32 VolumeID : ProxyResults.EnteredProxies)
	{
		if (!AGVComponents.Contains(VolumeID))
		{
			continue;
		}

		if (const UAtomGameplayVolumeComponent* ProxyComponent = AGVComponents[VolumeID])
		{
			ProxyComponent->EnterProxy();
		}
	}

	for (uint32 VolumeID : ProxyResults.ExitedProxies)
	{
		if (!AGVComponents.Contains(VolumeID))
		{
			continue;
		}

		if (const UAtomGameplayVolumeComponent* ProxyComponent = AGVComponents[VolumeID])
		{
			ProxyComponent->ExitProxy();
		}
	}
}

void UAtomGameplayVolumeSubsystem::UpdateFromListeners()
{
	check(IsInAtomThread());
	SCOPED_NAMED_EVENT(UAtomGameplayVolumeSubsystem_UpdateFromListeners, FColor::Blue);

	FAtomRuntimeHandle RuntimeHandle = GetAtomRuntimeHandle();
	check(RuntimeHandle.IsValid());
	const uint32 AtomRuntimeID = RuntimeHandle.GetRuntimeID();

	const int32 ListenerCount = RuntimeHandle->GetListeners().Num();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	AGVListeners.SetNum(ListenerCount, EAllowShrinking::No);
#else
	AGVListeners.SetNum(ListenerCount, false /* bAllowShrink */);
#endif
	constexpr bool bAllowAttenuationOverride = true;

	// We have to search twice, as we only care about mutators that affect listeners, but we care about ALL proxyVolumes we're a part of
	FAtomProxyMutatorSearchObject ProxySearch;
	ProxySearch.AtomRuntimeHandle = RuntimeHandle;
	ProxySearch.PayloadType = AtomGameplay::EComponentPayload::AGCP_Listener;

	FAtomProxyMutatorSearchResult Result;
	TSet<uint32> TempVolumeSet;
	FTransform ListenerTransform;

	GenerateVolumeProxyList();

	// Grabbing the listeners directly here should be removed when possible - done out of necessity due to the legacy audio volume system
	const TArray<FAtomListener>& AtomListeners = RuntimeHandle->GetListeners();

	// Update our Atom gameplay volume listeners
	for (int32 ListenerIndex = 0; ListenerIndex < ListenerCount; ++ListenerIndex)
	{
		const bool bValidAtomListener = RuntimeHandle->GetListenerTransform(ListenerIndex, ListenerTransform);
		if (bValidAtomListener && !ListenerTransform.Equals(FTransform::Identity))
		{
			// Fill location and worldID
			if (RuntimeHandle->GetListenerWorldID(ListenerIndex, ProxySearch.WorldID) && RuntimeHandle->GetListenerPosition(ListenerIndex, ProxySearch.Location, bAllowAttenuationOverride))
			{
				// Find only the proxy volumes we're inside of, regardless of payload type
				ProxySearch.bFilterPayload = false;
				ProxySearch.bCollectMutators = false;
				ProxySearch.bGetDefaultAtomSettings = false;
				ProxySearch.SearchVolumes(TransientProxyList, Result);

				// Hold on to these
				Swap(TempVolumeSet, Result.VolumeSet);

				// Second search - this time for mutators
				ProxySearch.bFilterPayload = true;
				ProxySearch.bCollectMutators = true;
				ProxySearch.bGetDefaultAtomSettings = true;
				ProxySearch.SearchVolumes(TransientProxyList, Result);

				// Reassign the set of all proxy volumes (regardless of payload type)
				Swap(TempVolumeSet, Result.VolumeSet);
				AGVListeners[ListenerIndex].Update(Result, ProxySearch.Location, AtomRuntimeID);

				if (FAtomGameplayVolumeProxyInfo* ProxyInfo = WorldProxyLists.Find(ProxySearch.WorldID))
				{
					ProxyInfo->AddListenerIndex(ListenerIndex);
				}

				continue;
			}
		}

		// Listener is invalid or uninitialized
		Result.Reset();
		AGVListeners[ListenerIndex].Update(Result, FVector::ZeroVector, AtomRuntimeID);
	}

	// We need to drop references, but we can at least prevent some allocs from using a member variable
	TransientProxyList.Reset();

	FAtomGameplayProxyUpdateResult ProxyUpdateResult;
	TWeakObjectPtr<UAtomGameplayVolumeSubsystem> WeakThis(this);

	// Update our world proxy lists
	for (TPair<uint32, FAtomGameplayVolumeProxyInfo>& WorldProxyInfo : WorldProxyLists)
	{
		WorldProxyInfo.Value.Update(AGVListeners, ProxyUpdateResult);
		if (!ProxyUpdateResult.EnteredProxies.IsEmpty() || !ProxyUpdateResult.ExitedProxies.IsEmpty())
		{
			FAtomThread::RunCommandOnGameThread([WeakThis, ProxyUpdateResult]
			{
				if (WeakThis.IsValid())
				{
					WeakThis->UpdateComponentsFromProxyInfo(ProxyUpdateResult);
				}
			});
		}
	}
}
