
#include "Atom/Modulation/AtomModulationSystem.h"

#include "Async/Async.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTLS.h"
#include "Misc/CoreDelegates.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/UObjectIterator.h"
#include "UObject/WeakObjectPtr.h"
#include "Audio/AudioAddressPattern.h"

//#include "AudioModulationSettings.h"
#include "Atom/Modulation/AtomModulationProfileSerializer.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationGenerator.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"
#include "AtomModulationControlBusProxy.h"
#include "AtomModulationGeneratorProxy.h"
#include "AtomModulationControlBusMixProxy.h"
#include "AtomModulationPatchProxy.h"
#include "AtomModulationProxy.h"
#include "Atom/Mixer/AtomMixerTrace.h"

#if !UE_BUILD_SHIPPING
#include "AtomModulationDebug.h"
#endif

DECLARE_DWORD_COUNTER_STAT(TEXT("Bus Count"), STAT_AtomModulationBusCount, STATGROUP_AtomModulation)
DECLARE_DWORD_COUNTER_STAT(TEXT("Generator Count"), STAT_AtomModulationGeneratorCount, STATGROUP_AtomModulation)
DECLARE_DWORD_COUNTER_STAT(TEXT("Mix Count"),	STAT_AtomModulationMixCount, STATGROUP_AtomModulation)
DECLARE_DWORD_COUNTER_STAT(TEXT("Patch Count"), STAT_AtomModulationPatchCount, STATGROUP_AtomModulation)
DECLARE_DWORD_COUNTER_STAT(TEXT("Render Queue Commands Processed"), STAT_AtomModulationProcQueueCount, STATGROUP_AtomModulation)

#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN(CriWareAtom, ControlBusUpdate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, ControlBusID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, ParamName)
UE_TRACE_EVENT_FIELD(float, Value)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, BusMixRegisterBus)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(uint32, ModulatingSourceID)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, BusName)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, BusMixActivate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, BusMixUpdate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(uint32[], BusIDs)
UE_TRACE_EVENT_FIELD(float[], BusValues)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, GeneratorUpdate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(uint32[], BusIDs)
UE_TRACE_EVENT_FIELD(float, GeneratorValue)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, BusFinalValuesUpdate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(uint32[], BusIDs)
UE_TRACE_EVENT_FIELD(float[], BusValues)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_DEFINE(CriWareAtom, ModulatingSourceDeactivate)
#endif // ATOM_PROFILERTRACE_ENABLED

namespace AtomModulation
{
	enum class EAtomModulatorType : Atom::FModulatorTypeId
	{
		Patch,
		Bus,
		Generator,

		Count
	};

	FAtomModulationSystem::~FAtomModulationSystem()
	{
#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.RemoveAll(this);
#endif // ATOM_PROFILERTRACE_ENABLED
	}

	void FAtomModulationSystem::Initialize(FAtomRuntimeId RumtimeID)
	{
#if !UE_BUILD_SHIPPING
		Debugger = MakeShared<FAtomModulationDebugger>();
#endif // !UE_BUILD_SHIPPING

#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.AddRaw(this, &FAtomModulationSystem::OnTraceStarted);
#endif // ATOM_PROFILERTRACE_ENABLED

		AtomRuntimeID = RumtimeID;
	}

	void FAtomModulationSystem::OnAuditionEnd()
	{
		DeactivateAllBusMixes();
	}

#if !UE_BUILD_SHIPPING
	bool FAtomModulationSystem::OnPostHelp(FCommonViewportClient* ViewportClient, const TCHAR* Stream)
	{
		check(IsInGameThread());
		return ViewportClient ? Debugger->OnPostHelp(*ViewportClient, Stream) : true;
	}

	int32 FAtomModulationSystem::OnRenderStat(UViewportStatsSubsystem* ViewportSubSystem, int32 OffsetX, int32 OffsetY)
	{
		check(IsInGameThread());
		return ViewportSubSystem ? Debugger->OnRenderStat(ViewportSubSystem, OffsetX, OffsetY) : OffsetY;
	}

	int32 FAtomModulationSystem::OnRenderStat(FViewport* Viewport, FCanvas* Canvas, int32 X, int32 Y, const UFont& Font)
	{
		check(IsInGameThread());
		return Canvas ? Debugger->OnRenderStat(*Canvas, X, Y, Font) : Y;
	}

	bool FAtomModulationSystem::OnToggleStat(FCommonViewportClient* ViewportClient, const TCHAR* Stream)
	{
		check(IsInGameThread());
		return ViewportClient ? Debugger->OnToggleStat(*ViewportClient, Stream) : true;
	}
#endif // !UE_BUILD_SHIPPING

	void FAtomModulationSystem::ActivateBus(const UAtomModulationControlBus& InBus)
	{
		RunCommandOnProcessingThread([this, Settings = FControlBusSettings(InBus)]() mutable
		{
			FBusHandle BusHandle = FBusHandle::Create(MoveTemp(Settings), RefProxies.Buses, *this);
			ManuallyActivatedBuses.Add(MoveTemp(BusHandle));
		});
	}

	void FAtomModulationSystem::ActivateBusMix(FModulatorBusMixSettings&& InSettings)
	{
		ActiveBusMixIDs.Add(InSettings.GetId());

		RunCommandOnProcessingThread([this, Settings = MoveTemp(InSettings)]() mutable
		{
			FBusMixHandle BusMixHandle = FBusMixHandle::Get(Settings.GetId(), RefProxies.BusMixes);
			if (BusMixHandle.IsValid())
			{
				BusMixHandle.FindProxy().SetMixDataAndEnable(MoveTemp(Settings));
			}
			else
			{
				BusMixHandle = FBusMixHandle::Create(MoveTemp(Settings), RefProxies.BusMixes, *this);
			}

			ManuallyActivatedBusMixes.Add(MoveTemp(BusMixHandle));

#if ATOM_PROFILERTRACE_ENABLED
			for (const FModulatorBusMixStageSettings& StageSetting : Settings.Stages)
			{
				UE_TRACE_LOG(CriWareAtom, BusMixRegisterBus, AtomChannel)
					<< BusMixRegisterBus.RuntimeID(AtomRuntimeID)
					<< BusMixRegisterBus.SourceID(StageSetting.BusSettings.GetId())
					<< BusMixRegisterBus.Timestamp(FPlatformTime::Cycles64())
					<< BusMixRegisterBus.ModulatingSourceID(Settings.GetId())
					<< BusMixRegisterBus.BusName(*(StageSetting.BusSettings.GetName().ToString()));
			}

			UE_TRACE_LOG(CriWareAtom, BusMixActivate, AtomChannel)
				<< BusMixActivate.RuntimeID(AtomRuntimeID)
				<< BusMixActivate.SourceID(Settings.GetId())
				<< BusMixActivate.Timestamp(FPlatformTime::Cycles64())
				<< BusMixActivate.Name(*Settings.GetName().ToString());
#endif
		});
	}

	void FAtomModulationSystem::ActivateBusMix(const UAtomModulationControlBusMix& InBusMix)
	{
		ActivateBusMix(FModulatorBusMixSettings(InBusMix));
	}

	void FAtomModulationSystem::ActivateGenerator(const UAtomModulationGenerator& InGenerator)
	{
		RunCommandOnProcessingThread([this, Settings = FModulationGeneratorSettings(InGenerator)]() mutable
		{
			FGeneratorHandle GeneratorHandle = FGeneratorHandle::Get(Settings.GetId(), RefProxies.Generators);
			if (GeneratorHandle.IsValid())
			{
				ManuallyActivatedGenerators.Add(MoveTemp(GeneratorHandle));
			}
			else
			{
				GeneratorHandle = FGeneratorHandle::Create(MoveTemp(Settings), RefProxies.Generators, *this);
				GeneratorHandle.FindProxy().Init(AtomRuntimeID);
			}
		});
	}

	void FAtomModulationSystem::AddReferencedObjects(FReferenceCollector& Collector)
	{
		TArray<TObjectPtr<UAtomModulationControlBusMix>> GlobalBusMixes;
		ActiveGlobalBusValueMixes.GenerateValueArray(GlobalBusMixes);
		Collector.AddReferencedObjects(GlobalBusMixes);
	}

	FString FAtomModulationSystem::GetReferencerName() const
	{
		return TEXT("FAtomModulationSystem");
	}

	bool FAtomModulationSystem::CalculateModulationValue(FModulationPatchProxy& OutProxy, float& OutValue) const
	{
		check(IsInProcessingThread());
		if (OutProxy.IsBypassed())
		{
			return false;
		}

		const float InitValue = OutValue;
		OutProxy.Update();
		OutValue = OutProxy.GetValue();
		return !FMath::IsNearlyEqual(InitValue, OutValue);
	}

	void FAtomModulationSystem::DeactivateBus(const UAtomModulationControlBus& InBus)
	{
		ClearGlobalBusMixValue(InBus);

		RunCommandOnProcessingThread([this, BusId = static_cast<FBusId>(InBus.GetUniqueID())]()
		{
			FBusHandle BusHandle = FBusHandle::Get(BusId, RefProxies.Buses);
			if (BusHandle.IsValid())
			{
				ManuallyActivatedBuses.Remove(BusHandle);
			}
		});
	}

	void FAtomModulationSystem::DeactivateBusMix(const UAtomModulationControlBusMix& InBusMix)
	{
		ActiveBusMixIDs.Remove(InBusMix.GetUniqueID());

		RunCommandOnProcessingThread([this, BusMixId = static_cast<FBusMixId>(InBusMix.GetUniqueID())]()
		{
			FBusMixHandle MixHandle = FBusMixHandle::Get(BusMixId, RefProxies.BusMixes);
			if (MixHandle.IsValid())
			{
				FModulatorBusMixProxy& MixProxy = MixHandle.FindProxy();
				MixProxy.SetStopping();
			}
		});
	}

	void FAtomModulationSystem::DeactivateAllBusMixes()
	{
		ClearAllGlobalBusMixValues();

		ActiveBusMixIDs.Empty();

		RunCommandOnProcessingThread([this]()
		{
			for (TPair<FBusMixId, FModulatorBusMixProxy>& Pair : RefProxies.BusMixes)
			{
				Pair.Value.SetStopping();
			}
		});
	}	

	void FAtomModulationSystem::DeactivateGenerator(const UAtomModulationGenerator& InGenerator)
	{
		RunCommandOnProcessingThread([this, GeneratorId = static_cast<FGeneratorId>(InGenerator.GetUniqueID())]()
		{
			FGeneratorHandle GeneratorHandle = FGeneratorHandle::Get(GeneratorId, RefProxies.Generators);
			if (GeneratorHandle.IsValid())
			{
				ManuallyActivatedGenerators.Remove(GeneratorHandle);
			}
		});
	}

#if !UE_BUILD_SHIPPING
	void FAtomModulationSystem::SetDebugBusFilter(const FString* InFilter)
	{
		Debugger->SetDebugBusFilter(InFilter);
	}

	void FAtomModulationSystem::SetDebugGeneratorsEnabled(bool bInIsEnabled)
	{
		Debugger->SetDebugGeneratorsEnabled(bInIsEnabled);
	}

	void FAtomModulationSystem::SetDebugGeneratorFilter(const FString* InFilter)
	{
		Debugger->SetDebugGeneratorFilter(InFilter);
	}

	void FAtomModulationSystem::SetDebugGeneratorTypeFilter(const FString* InFilter, bool bInEnabled)
	{
		Debugger->SetDebugGeneratorTypeFilter(InFilter, bInEnabled);
	}

	void FAtomModulationSystem::SetDebugMatrixEnabled(bool bInIsEnabled)
	{
		Debugger->SetDebugMatrixEnabled(bInIsEnabled);
	}

	void FAtomModulationSystem::SetDebugMixFilter(const FString* InNameFilter)
	{
		Debugger->SetDebugMixFilter(InNameFilter);
	}
#endif // !UE_BUILD_SHIPPING

	void FAtomModulationSystem::SetGlobalBusMixValue(UAtomModulationControlBus& InBus, float InValue, float InFadeTime)
	{
		if (ActiveGlobalBusValueMixes.Contains(InBus.GetUniqueID()))
		{
			TObjectPtr<UAtomModulationControlBusMix> GlobalMix = ActiveGlobalBusValueMixes.FindRef(InBus.GetUniqueID());
		
			if (ensure(!GlobalMix->MixStages.IsEmpty()))
			{
				GlobalMix->MixStages[0].Value.TargetValue = InValue;
				UpdateMix(*GlobalMix, InFadeTime);

				UE_LOG(LogAtomModulation, VeryVerbose, TEXT("GlobalBusMix for ControlBus '%s' updated, target set to '%0.4f'."), *InBus.GetName(), InValue);
			}
		}
		else
		{
			const FName MixName(*FString::Printf(TEXT("%s_GlobalMix"), *InBus.GetName()));
			if (TObjectPtr<UAtomModulationControlBusMix> NewGlobalMix = CreateBusMixFromValue(MixName, { &InBus }, InValue, InFadeTime))
			{
				ActiveGlobalBusValueMixes.Add(InBus.GetUniqueID(), NewGlobalMix);
				UE_LOG(LogAtomModulation, VeryVerbose, TEXT("GlobalBusMix for ControlBus '%s' activated, target set to '%0.4f'."), *InBus.GetName(), InValue);
				ActivateBusMix(*NewGlobalMix);
			}
			else
			{
				UE_LOG(LogAtomModulation, Warning, TEXT("Could not set Global Bus Mix value: failed to allocate new Global Bus Mix for bus %s."), *InBus.GetName());
			}
		}

	}

	void FAtomModulationSystem::ClearGlobalBusMixValue(const UAtomModulationControlBus& InBus, float InFadeTime)
	{
		const uint32 BusID = InBus.GetUniqueID();
		if (TObjectPtr<UAtomModulationControlBusMix> GlobalMix = ActiveGlobalBusValueMixes.FindRef(BusID))
		{
			if (ensure(!GlobalMix->MixStages.IsEmpty()))
			{
				GlobalMix->MixStages[0].Value.ReleaseTime = InFadeTime;
				DeactivateBusMix(*GlobalMix);
				ActiveGlobalBusValueMixes.Remove(BusID);
				UE_LOG(LogAtomModulation, VeryVerbose, TEXT("GlobalBusMix for ControlBus '%s' cleared."), *InBus.GetName());
			}
		}
		else
		{
			UE_LOG(LogAtomModulation, VeryVerbose, TEXT("GlobalBusMix for ControlBus '%s' not active, ignoring clear request."), *InBus.GetName());
		}
	}

	void FAtomModulationSystem::ClearAllGlobalBusMixValues(float InFadeTime)
	{
		TArray<TObjectPtr<UAtomModulationControlBusMix>> GlobalBusMixes;
		ActiveGlobalBusValueMixes.GenerateValueArray(GlobalBusMixes);
		for (const TObjectPtr<UAtomModulationControlBusMix>& BusMix : GlobalBusMixes)
		{
			if (ensure(!BusMix->MixStages.IsEmpty()))
			{
				ClearGlobalBusMixValue(*BusMix->MixStages[0].Bus, InFadeTime);
			}
		}

		ActiveGlobalBusValueMixes.Reset();
	}

	UAtomModulationControlBusMix* FAtomModulationSystem::CreateBusMixFromValue(FName Name, const TArray<UAtomModulationControlBus*>& Buses, float Value, float AttackTime, float ReleaseTime)
	{
		if (TObjectPtr<UAtomModulationControlBusMix> NewGlobalMix = NewObject<UAtomModulationControlBusMix>(GetTransientPackage(), Name))
		{
			for (UAtomModulationControlBus* Bus : Buses)
			{
				if (Bus)
				{
					FAtomModulationMixValue MixValue;
					MixValue.TargetValue = Value;

					if (AttackTime >= 0.0f)
					{
						MixValue.AttackTime = AttackTime;
					}

					if (ReleaseTime >= 0.0f)
					{
						MixValue.ReleaseTime = ReleaseTime;
					}

					FAtomModulationControlBusMixStage MixStage;
					MixStage.Bus = Bus;
					MixStage.Value = MixValue;

					NewGlobalMix->MixStages.Emplace(MoveTemp(MixStage));
				}
			}
			return NewGlobalMix;
		}

		return nullptr;
	}

	bool FAtomModulationSystem::GetModulatorValue(const Atom::FModulatorHandle& InModulatorHandle, float& OutValue) const
	{
		const EAtomModulatorType ModulatorType = static_cast<EAtomModulatorType>(InModulatorHandle.GetTypeId());

		switch (ModulatorType)
		{
		case EAtomModulatorType::Patch:
		{
			// Direct access preferred vs through handles here as its impossible for proxies to be destroyed
			// in look-up and speed is key as this is possibly being queried often in the audio render pass.
			if (const FModulationPatchRefProxy* PatchProxy = RefProxies.Patches.Find(static_cast<FPatchId>(InModulatorHandle.GetModulatorId())))
			{
				if (!PatchProxy->IsBypassed())
				{
					OutValue = PatchProxy->GetValue();
					return true;
				}
			}
		}
		break;

		case EAtomModulatorType::Bus:
		{
			if (const FControlBusProxy* BusProxy = RefProxies.Buses.Find(static_cast<FBusId>(InModulatorHandle.GetModulatorId())))
			{
				if (!BusProxy->IsBypassed())
				{
					OutValue = BusProxy->GetValue();
					return true;
				}
			}
		}
		break;

		case EAtomModulatorType::Generator:
		{
			if (const FModulatorGeneratorProxy* GeneratorProxy = RefProxies.Generators.Find(static_cast<FGeneratorId>(InModulatorHandle.GetModulatorId())))
			{
				if (!GeneratorProxy->IsBypassed())
				{
					OutValue = GeneratorProxy->GetValue();
					return true;
				}
			}
		}
		break;

		default:
		{
			static_assert(static_cast<uint32>(EAtomModulatorType::Count) == 3, "Possible missing modulator type coverage in switch statement");
		}
		break;
		}

		return false;
	}

	bool FAtomModulationSystem::GetModulatorValueThreadSafe(const Atom::FModulatorHandle& InModulatorHandle, float& OutValue) const
	{
		FScopeLock Lock(&ThreadSafeModValueCritSection);

		if (const float* Value = ThreadSafeModValueMap.Find(InModulatorHandle.GetModulatorId()))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}

	bool FAtomModulationSystem::GetModulatorValueThreadSafe(uint32 ModulatorID, float& OutValue) const
	{
		FScopeLock Lock(&ThreadSafeModValueCritSection);

		if (const float* Value = ThreadSafeModValueMap.Find(ModulatorID))
		{
			OutValue = *Value;
			return true;
		}

		return false;
	}

	FAtomRuntimeId FAtomModulationSystem::GetAtomRuntimeID() const
	{
		return AtomRuntimeID;
	}

	bool FAtomModulationSystem::IsInProcessingThread() const
	{
		return ProcessingThreadId == FPlatformTLS::GetCurrentThreadId();
	}

	void FAtomModulationSystem::ProcessModulators(const double InElapsed)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FAtomModulationSystem::ProcessModulators);

		// The processing thread can get updated between frames. As modulation
		// processing should be first call in frame, update the threadId accordingly
		ProcessingThreadId = FPlatformTLS::GetCurrentThreadId();

		int32 CommandsProcessed = 0;
		TUniqueFunction<void()> Command;
		while (ProcessingThreadCommandQueue.Dequeue(Command))
		{
			Command();
			++CommandsProcessed;
		}

		TMap<Atom::FModulatorId, float> NewModulatorValues;

		// Update Generators (prior to bus mixing to avoid single-frame latency)
		for (TPair<FGeneratorId, FModulatorGeneratorProxy>& Pair : RefProxies.Generators)
		{
			Pair.Value.PumpCommands();
			Pair.Value.Update(InElapsed);
			NewModulatorValues.Add(static_cast<Atom::FModulatorId>(Pair.Key), Pair.Value.GetValue());
		}

		// Reset buses & refresh cached Generator
		for (TPair<FBusId, FControlBusProxy>& Pair : RefProxies.Buses)
		{
			Pair.Value.Reset();
			Pair.Value.MixGenerators();
		}

		// Update mix values and apply to prescribed buses.
		// Track bus mixes ready to remove
		TSet<FBusMixId> StoppedMixIds;
		for (TPair<FBusMixId, FModulatorBusMixProxy>& Pair : RefProxies.BusMixes)
		{
			const FModulatorBusMixProxy::EStatus LastStatus = Pair.Value.GetStatus();
			Pair.Value.Update(InElapsed, RefProxies.Buses);
			const FModulatorBusMixProxy::EStatus CurrentStatus = Pair.Value.GetStatus();

			switch (CurrentStatus)
			{
			case FModulatorBusMixProxy::EStatus::Enabled:
			case FModulatorBusMixProxy::EStatus::Stopping:
				break;

			case FModulatorBusMixProxy::EStatus::Stopped:
			{
				if (LastStatus != CurrentStatus)
				{
					UE_LOG(LogAtomModulation, Verbose, TEXT("Atom modulation mix '%s' stopped."), *Pair.Value.GetName().ToString());
				}
				StoppedMixIds.Add(Pair.Key);
			}
			break;

			default:
			{
				checkf(false, TEXT("Invalid or unsupported BusMix EStatus state advancement."));
			}
			break;
			}
		}

		// Destroy mixes that have stopped (must be done outside mix update
		// loop above to avoid destroying while iterating, which can occur
		// when update moves bus mix from 'stopping' status to 'stopped')
		for (const FBusMixId& MixId : StoppedMixIds)
		{
			FBusMixHandle MixHandle = FBusMixHandle::Get(MixId, RefProxies.BusMixes);

			// Expected to be valid given the fact that the proxy is available in the prior loop
			check(MixHandle.IsValid());

			// Expected to only have two references (one for transient 'MixHandle' and one in
			// ManuallyActivated set). Nothing else should be keeping mixes active.
			check(MixHandle.FindProxy().GetRefCount() == 2);

			ManuallyActivatedBusMixes.Remove(MoveTemp(MixHandle));
		}

		for (TPair<FPatchId, FModulationPatchRefProxy>& Pair : RefProxies.Patches)
		{
			FModulationPatchRefProxy& PatchProxy = Pair.Value;
			if (!PatchProxy.IsBypassed())
			{
				PatchProxy.Update();
				NewModulatorValues.Add(static_cast<Atom::FModulatorId>(Pair.Key), PatchProxy.GetValue());
			}
		}

		for (const TPair<FBusId, FControlBusProxy>& Pair : RefProxies.Buses)
		{
			NewModulatorValues.Add(static_cast<Atom::FModulatorId>(Pair.Key), Pair.Value.GetValue());
		}

		{
			FScopeLock Lock(&ThreadSafeModValueCritSection);
			ThreadSafeModValueMap = MoveTemp(NewModulatorValues);
		}

		// Log stats
		SET_DWORD_STAT(STAT_AtomModulationBusCount, RefProxies.Buses.Num());
		SET_DWORD_STAT(STAT_AtomModulationMixCount, RefProxies.BusMixes.Num());
		SET_DWORD_STAT(STAT_AtomModulationGeneratorCount, RefProxies.Generators.Num());
		SET_DWORD_STAT(STAT_AtomModulationPatchCount, RefProxies.Patches.Num());
		SET_DWORD_STAT(STAT_AtomModulationProcQueueCount, CommandsProcessed);

#if ATOM_PROFILERTRACE_ENABLED
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
		if (bChannelEnabled)
		{
			TMap<FGeneratorId, TSet<FBusId>> GeneratorIdToBusIdsMap;
			TArray<FBusId, TInlineAllocator<32>> BusIdsFinalValues;
			TArray<float, TInlineAllocator<32>> BusFinalValues;

			const int32 NumBuses = RefProxies.Buses.Num();
			BusIdsFinalValues.Reserve(NumBuses);
			BusFinalValues.Reserve(NumBuses);

			// Control Bus message trace
			for (const auto& [BusId, ControlBusProxy] : RefProxies.Buses)
			{
				UE_TRACE_LOG(CriWareAtom, ControlBusUpdate, AtomChannel)
					<< ControlBusUpdate.RuntimeID(AtomRuntimeID)
					<< ControlBusUpdate.ControlBusID(BusId)
					<< ControlBusUpdate.Timestamp(FPlatformTime::Cycles64())
					<< ControlBusUpdate.Name(*(ControlBusProxy.GetName().ToString()))
					<< ControlBusUpdate.ParamName(*(ControlBusProxy.GetParameterName().ToString()))
					<< ControlBusUpdate.Value(ControlBusProxy.GetValue());

				// Collect the final value of this control bus
				BusIdsFinalValues.Emplace(BusId);
				BusFinalValues.Emplace(ControlBusProxy.GetValue());

				// Collect generators set in this control bus
				for (const FGeneratorHandle& GeneratorHandle : ControlBusProxy.GetGeneratorHandles())
				{
					GeneratorIdToBusIdsMap.FindOrAdd(GeneratorHandle.GetId()).Emplace(BusId);
				}
			}

			// Control Bus Mix message trace
			for (const auto& [BusMixId, BusMixProxy] : RefProxies.BusMixes)
			{
				const int32 NumStages = BusMixProxy.Stages.Num();

				TArray<FBusId, TInlineAllocator<32>> BusIds;
				TArray<float, TInlineAllocator<32>> BusMixCurrentValues;

				BusIds.Reserve(NumStages);
				BusMixCurrentValues.Reserve(NumStages);

				for (const auto& [BusId, BusMixStageProxy] : BusMixProxy.Stages)
				{
					BusIds.Emplace(BusId);
					BusMixCurrentValues.Emplace(BusMixStageProxy.Value.GetCurrentValue());
				}

				UE_TRACE_LOG(CriWareAtom, BusMixUpdate, AtomChannel)
					<< BusMixUpdate.RuntimeID(AtomRuntimeID)
					<< BusMixUpdate.SourceID(BusMixId)
					<< BusMixUpdate.Timestamp(FPlatformTime::Cycles64())
					<< BusMixUpdate.BusIDs(BusIds.GetData(), BusIds.Num())
					<< BusMixUpdate.BusValues(BusMixCurrentValues.GetData(), BusMixCurrentValues.Num());
			}

			// Generator message trace
			for (const auto& [GeneratorId, BusIds] : GeneratorIdToBusIdsMap)
			{
				if (const FModulatorGeneratorProxy* FoundGeneratorProxyPtr = RefProxies.Generators.Find(GeneratorId))
				{
					UE_TRACE_LOG(CriWareAtom, GeneratorUpdate, AtomChannel)
						<< GeneratorUpdate.RuntimeID(AtomRuntimeID)
						<< GeneratorUpdate.SourceID(GeneratorId)
						<< GeneratorUpdate.Timestamp(FPlatformTime::Cycles64())
						<< GeneratorUpdate.BusIDs(BusIds.Array().GetData(), BusIds.Num())
						<< GeneratorUpdate.GeneratorValue(FoundGeneratorProxyPtr->GetValue());
				}
			}

			// Bus Final values message trace
			UE_TRACE_LOG(CriWareAtom, BusFinalValuesUpdate, AtomChannel)
				<< BusFinalValuesUpdate.RuntimeID(AtomRuntimeID)
				<< BusFinalValuesUpdate.SourceID(INDEX_NONE)
				<< BusFinalValuesUpdate.Timestamp(FPlatformTime::Cycles64())
				<< BusFinalValuesUpdate.BusIDs(BusIdsFinalValues.GetData(), BusIdsFinalValues.Num())
				<< BusFinalValuesUpdate.BusValues(BusFinalValues.GetData(), BusFinalValues.Num());
		}
#endif // UE_AUDIO_PROFILERTRACE_ENABLED
#if !UE_BUILD_SHIPPING
		Debugger->UpdateDebugData(InElapsed, RefProxies);
#endif // !UE_BUILD_SHIPPING
	}

	bool FAtomModulationSystem::IsControlBusMixActive(const UAtomModulationControlBusMix& InBusMix)
	{
		FBusMixId BusMixId = static_cast<FBusMixId>(InBusMix.GetUniqueID());
		return ActiveBusMixIDs.Contains(BusMixId);
	}

	void FAtomModulationSystem::SaveMixToProfile(const UAtomModulationControlBusMix& InBusMix, const int32 InProfileIndex)
	{
		check(IsInGameThread());

		RunCommandOnProcessingThread([this, MixToSerialize = TWeakObjectPtr<const UAtomModulationControlBusMix>(&InBusMix), InProfileIndex]()
		{
			if (!MixToSerialize.IsValid())
			{
				return;
			}

			const FBusMixId MixId = static_cast<FBusMixId>(MixToSerialize->GetUniqueID());
			const FString   MixName = MixToSerialize->GetName();

			FBusMixHandle MixHandle = FBusMixHandle::Get(MixId, RefProxies.BusMixes);
			const bool bIsActive = MixHandle.IsValid();
			if (!MixHandle.IsValid())
			{
				UE_LOG(LogAtomModulation, Display, TEXT("Mix '%s' is inactive, saving default object to profile '%i'."), *MixName, InProfileIndex);
				AsyncTask(ENamedThreads::GameThread, [this, MixToSerialize, InProfileIndex]()
				{
					AtomModulation::FProfileSerializer::Serialize(*MixToSerialize.Get(), InProfileIndex);
				});
				return;
			}

			UE_LOG(LogAtomModulation, Display, TEXT("Mix '%s' is active, saving current mix proxy state to profile '%i'."), *MixName, InProfileIndex);
			AtomModulation::FModulatorBusMixProxy& MixProxy = MixHandle.FindProxy();
			TMap<FBusId, FAtomModulationMixValue> PassedStageInfo;
			for (TPair<FBusId, FModulatorBusMixStageProxy>& Pair : MixProxy.Stages)
			{
				FModulatorBusMixStageProxy& Stage = Pair.Value;
				PassedStageInfo.Add(Pair.Key, Stage.Value);
			}

			AsyncTask(ENamedThreads::GameThread, [this, PassedStageInfo, MixToSerialize, InProfileIndex]()
			{
				if (!MixToSerialize.IsValid())
				{
					return;
				}

				TMap<FBusId, FAtomModulationMixValue> StageInfo = PassedStageInfo;
				UAtomModulationControlBusMix* TempMix = NewObject<UAtomModulationControlBusMix>(GetTransientPackage(), *FGuid::NewGuid().ToString(EGuidFormats::Short));

				// Buses on proxy may differ than those on uobject definition, so iterate and find by cached ids
				// and add to temp mix to be serialized.
				for (TObjectIterator<UAtomModulationControlBus> Itr; Itr; ++Itr)
				{
					if (UAtomModulationControlBus* Bus = *Itr)
					{
						FBusId ItrBusId = static_cast<FBusId>(Bus->GetUniqueID());
						if (FAtomModulationMixValue* Value = StageInfo.Find(ItrBusId))
						{
							FAtomModulationControlBusMixStage BusMixStage;
							BusMixStage.Bus = Bus;
							BusMixStage.Value = *Value;
							TempMix->MixStages.Add(MoveTemp(BusMixStage));
						}
					}
				}

				const FString MixPath = MixToSerialize->GetPathName();
				AtomModulation::FProfileSerializer::Serialize(*TempMix, InProfileIndex, &MixPath);
			});
		});
	}

	TArray<FAtomModulationControlBusMixStage> FAtomModulationSystem::LoadMixFromProfile(const int32 InProfileIndex, UAtomModulationControlBusMix& OutBusMix)
	{
		const FString TempName = FGuid::NewGuid().ToString(EGuidFormats::Short);
		if (UAtomModulationControlBusMix* TempMix = NewObject<UAtomModulationControlBusMix>(GetTransientPackage(), *TempName))
		{
			const FString MixPath = OutBusMix.GetPathName();
			AtomModulation::FProfileSerializer::Deserialize(InProfileIndex, *TempMix, &MixPath);
			UpdateMix(TempMix->MixStages, OutBusMix);
			return TempMix->MixStages;
		}

		return TArray<FAtomModulationControlBusMixStage>();
	}

	void FAtomModulationSystem::RunCommandOnProcessingThread(TUniqueFunction<void()> Cmd)
	{
		if (IsInProcessingThread())
		{
			Cmd();
		}
		else
		{
			ProcessingThreadCommandQueue.Enqueue(MoveTemp(Cmd));
		}
	}

	void FAtomModulationSystem::OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination)
	{
#if ATOM_PROFILERTRACE_ENABLED
		RunCommandOnProcessingThread([this]()
		{
			for (const auto& [BusId, ControlBusProxy] : RefProxies.Buses)
			{
				UE_TRACE_LOG(CriWareAtom, BusMixRegisterBus, AtomChannel)
					<< BusMixRegisterBus.RuntimeID(AtomRuntimeID)
					<< BusMixRegisterBus.SourceID(ControlBusProxy.GetId())
					<< BusMixRegisterBus.Timestamp(FPlatformTime::Cycles64())
					<< BusMixRegisterBus.ModulatingSourceID(BusId)
					<< BusMixRegisterBus.BusName(*(ControlBusProxy.GetName().ToString()));

				ControlBusProxy.OnTraceStarted(*this);
			}

			for (const auto& [BusMixId, ModulatorBusMixProxy] : RefProxies.BusMixes)
			{
				UE_TRACE_LOG(CriWareAtom, BusMixActivate, AtomChannel)
					<< BusMixActivate.RuntimeID(AtomRuntimeID)
					<< BusMixActivate.SourceID(BusMixId)
					<< BusMixActivate.Timestamp(FPlatformTime::Cycles64())
					<< BusMixActivate.Name(*(ModulatorBusMixProxy.GetName().ToString()));
			}
		});
#endif // ATOM_PROFILERTRACE_ENABLED
	}

	Atom::FModulatorTypeId FAtomModulationSystem::RegisterModulator(Atom::FModulatorHandleId InHandleId, const FControlBusSettings& InSettings)
	{
		FControlBusSettings CachedSettings = InSettings;
		RegisterModulator(InHandleId, MoveTemp(CachedSettings), RefProxies.Buses, RefModulators.BusMap);
		return static_cast<Atom::FModulatorTypeId>(EAtomModulatorType::Bus);
	}

	Atom::FModulatorTypeId FAtomModulationSystem::RegisterModulator(Atom::FModulatorHandleId InHandleId, const FModulationGeneratorSettings& InSettings)
	{
		FModulationGeneratorSettings CachedSettings = InSettings;
		RegisterModulator(InHandleId, MoveTemp(CachedSettings), RefProxies.Generators, RefModulators.GeneratorMap, [this](FGeneratorHandle& NewHandle)
		{
			NewHandle.FindProxy().Init(AtomRuntimeID);
		});
		return static_cast<Atom::FModulatorTypeId>(EAtomModulatorType::Generator);
	}

	Atom::FModulatorTypeId FAtomModulationSystem::RegisterModulator(Atom::FModulatorHandleId InHandleId, const FModulationPatchSettings& InSettings)
	{
		FModulationPatchSettings CachedSettings = InSettings;
		RegisterModulator(InHandleId, MoveTemp(CachedSettings), RefProxies.Patches, RefModulators.PatchMap);
		return static_cast<Atom::FModulatorTypeId>(EAtomModulatorType::Patch);
	}

	void FAtomModulationSystem::RegisterModulator(Atom::FModulatorHandleId InHandleId, Atom::FModulatorId InModulatorId)
	{
		RunCommandOnProcessingThread([this, InHandleId, InModulatorId]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(FAtomModulationSystem::RegisterModulator);

			FPatchHandle PatchHandle = FPatchHandle::Get(static_cast<FPatchId>(InModulatorId), RefProxies.Patches);
			if (PatchHandle.IsValid())
			{
				if (FModulatorHandleSet* HandleSet = RefModulators.PatchMap.Find(PatchHandle))
				{
					HandleSet->Add(InHandleId);
				}
				return;
			}

			FBusHandle BusHandle = FBusHandle::Get(static_cast<FBusId>(InModulatorId), RefProxies.Buses);
			if (BusHandle.IsValid())
			{
				if (FModulatorHandleSet* HandleSet = RefModulators.BusMap.Find(BusHandle))
				{
					HandleSet->Add(InHandleId);
				}
				return;
			}

			FGeneratorHandle GeneratorHandle = FGeneratorHandle::Get(static_cast<FGeneratorId>(InModulatorId), RefProxies.Generators);
			if (GeneratorHandle.IsValid())
			{
				if (FModulatorHandleSet* HandleSet = RefModulators.GeneratorMap.Find(GeneratorHandle))
				{
					HandleSet->Add(InHandleId);
				}
				return;
			}

			ensureAlwaysMsgf(false, TEXT("Failed to register modulator handle '%i' with pre-existing modulator '%i'"), static_cast<int32>(InHandleId), static_cast<int32>(InModulatorId));
		});
	}

	void FAtomModulationSystem::SoloBusMix(const UAtomModulationControlBusMix& InBusMix)
	{
		RunCommandOnProcessingThread([this, BusMixSettings = FModulatorBusMixSettings(InBusMix)]() mutable
		{
			bool bMixActive = false;
			for (TPair<FBusMixId, FModulatorBusMixProxy>& Pair : RefProxies.BusMixes)
			{
				if (Pair.Key == BusMixSettings.GetId())
				{
					bMixActive = true;
				}
				else
				{
					Pair.Value.SetStopping();
				}
			}

			if (!bMixActive)
			{
				ActivateBusMix(MoveTemp(BusMixSettings));
			}
		});
	}

	void FAtomModulationSystem::UnregisterModulator(const Atom::FModulatorHandle& InHandle)
	{
		RunCommandOnProcessingThread([this, ModId = InHandle.GetModulatorId(), HandleId = InHandle.GetHandleId()]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(FAtomModulationSystem::UnregisterModulator);

			FPatchHandle PatchHandle = FPatchHandle::Get(static_cast<FPatchId>(ModId), RefProxies.Patches);
			if (UnregisterModulator<FPatchHandle>(PatchHandle, RefModulators.PatchMap, HandleId))
			{
				return;
			}

			FBusHandle BusHandle = FBusHandle::Get(static_cast<FBusId>(ModId), RefProxies.Buses);
			if (UnregisterModulator<FBusHandle>(BusHandle, RefModulators.BusMap, HandleId))
			{
#if ATOM_PROFILERTRACE_ENABLED
				if (RefModulators.BusMap.Find(BusHandle) == nullptr)
				{
					FControlBusProxy& ControlBusProxy = BusHandle.FindProxy();

					for (const FGeneratorHandle& GeneratorHandle : ControlBusProxy.GetGeneratorHandles())
					{
						UE_TRACE_LOG(CriWareAtom, ModulatingSourceDeactivate, AtomChannel)
							<< ModulatingSourceDeactivate.RuntimeID(AtomRuntimeID)
							<< ModulatingSourceDeactivate.SourceID(GeneratorHandle.GetId())
							<< ModulatingSourceDeactivate.Timestamp(FPlatformTime::Cycles64());
					}
				}
#endif
				return;
			}

			FGeneratorHandle GeneratorHandle = FGeneratorHandle::Get(static_cast<FGeneratorId>(ModId), RefProxies.Generators);
			if (UnregisterModulator<FGeneratorHandle>(GeneratorHandle, RefModulators.GeneratorMap, HandleId))
			{
#if ATOM_PROFILERTRACE_ENABLED
				UE_TRACE_LOG(CriWareAtom, ModulatingSourceDeactivate, AtomChannel)
					<< ModulatingSourceDeactivate.RuntimeID(AtomRuntimeID)
					<< ModulatingSourceDeactivate.SourceID(ModId)
					<< ModulatingSourceDeactivate.Timestamp(FPlatformTime::Cycles64());
#endif
				return;
			}
		});
	}

	void FAtomModulationSystem::UpdateMix(const TArray<FAtomModulationControlBusMixStage>& InStages, UAtomModulationControlBusMix& InOutMix, bool bInUpdateObject, float InFadeTime, double Duration, bool bRetriggerOnActivation)
	{
		if (bInUpdateObject)
		{
			TMap<uint32, const FAtomModulationControlBusMixStage*> UpdatedStageBuses;
			for (const FAtomModulationControlBusMixStage& Stage : InStages)
			{
				if (Stage.Bus)
				{
					UpdatedStageBuses.Add(Stage.Bus->GetUniqueID(), &Stage);
				}
			}

			bool bMarkDirty = false;
			for (FAtomModulationControlBusMixStage& Stage : InOutMix.MixStages)
			{
				if (!Stage.Bus)
				{
					continue;
				}

				if (const FAtomModulationControlBusMixStage* BusStage = UpdatedStageBuses.FindRef(Stage.Bus->GetUniqueID()))
				{
					Stage = *BusStage;
					bMarkDirty = true;
				}
			}
			InOutMix.MarkPackageDirty();
		}

		const FBusMixId MixId = static_cast<FBusMixId>(InOutMix.GetUniqueID());

		TArray<FModulatorBusMixStageSettings> StageSettings;
		for (const FAtomModulationControlBusMixStage& Stage : InStages)
		{
			if (Stage.Bus)
			{
				StageSettings.Emplace(Stage);
			}
		}

		const FString BusMixName = InOutMix.GetName();
		RunCommandOnProcessingThread([this, MixId, StageSettings, InFadeTime, BusMixName, Duration, bRetriggerOnActivation]()
		{
			if (FModulatorBusMixProxy* BusMixes = RefProxies.BusMixes.Find(MixId))
			{
				BusMixes->SetMixData(StageSettings, InFadeTime, BusMixName, Duration, bRetriggerOnActivation);
			}
			else
			{
				UE_LOG(LogAtomModulation, Display, TEXT("Could not update mix '%s' because the mix is not currently active."), *BusMixName);
			}
		});
	}

	void FAtomModulationSystem::UpdateMixByFilter(
		const FString& InAddressFilter,
		const TSubclassOf<UAtomModulationParameter>& InParamClassFilter,
		UAtomModulationParameter* InParamFilter,
		float InValue,
		float InFadeTime,
		UAtomModulationControlBusMix& InOutMix,
		bool bInUpdateObject)
	{
		const uint32 ParamClassId = InParamClassFilter ? InParamClassFilter->GetUniqueID() : INDEX_NONE;
		const uint32 ParamId = InParamFilter ? InParamFilter->GetUniqueID() : INDEX_NONE;

		if (bInUpdateObject)
		{
			bool bMarkDirty = false;
			for (FAtomModulationControlBusMixStage& Stage : InOutMix.MixStages)
			{
				if (!Stage.Bus)
				{
					continue;
				}

				if (UAtomModulationParameter* Parameter = Stage.Bus->Parameter)
				{
					if (ParamId != INDEX_NONE && ParamId != Parameter->GetUniqueID())
					{
						continue;
					}

					if (UClass* Class = Parameter->GetClass())
					{
						if (ParamClassId != INDEX_NONE && ParamClassId != Class->GetUniqueID())
						{
							continue;
						}
					}
				}

				if (!FAudioAddressPattern::PartsMatch(InAddressFilter, Stage.Bus->Address))
				{
					continue;
				}

				Stage.Value.TargetValue = InValue;
				Stage.Value.SetActiveFade(FAtomModulationMixValue::EActiveFade::Override, InFadeTime);
				bMarkDirty = true;
			}

			if (bMarkDirty)
			{
				InOutMix.MarkPackageDirty();
			}
		}

		const FString	AddressFilter = InAddressFilter;
		const FBusMixId MixId = static_cast<FBusMixId>(InOutMix.GetUniqueID());
		RunCommandOnProcessingThread([this, ParamClassId, ParamId, MixId, AddressFilter, InValue, InFadeTime]()
		{
			if (FModulatorBusMixProxy* MixProxy = RefProxies.BusMixes.Find(MixId))
			{
				MixProxy->SetMixByFilter(AddressFilter, ParamClassId, ParamId, InValue, InFadeTime);
			}
		});
	}

	void FAtomModulationSystem::UpdateMix(const UAtomModulationControlBusMix& InMix, float InFadeTime)
	{
		RunCommandOnProcessingThread([this, MixSettings = FModulatorBusMixSettings(InMix), InFadeTime]() mutable
		{
			FBusMixHandle BusMixHandle = FBusMixHandle::Get(MixSettings.GetId(), RefProxies.BusMixes);
			if (BusMixHandle.IsValid())
			{
				FModulatorBusMixProxy& MixProxy = BusMixHandle.FindProxy();
				if (MixProxy.GetStatus() == FModulatorBusMixProxy::EStatus::Enabled)
				{
					MixProxy = MoveTemp(MixSettings);
					for (TPair<FBusId, FModulatorBusMixStageProxy>& Stage : MixProxy.Stages)
					{
						Stage.Value.Value.SetActiveFade(FAtomModulationMixValue::EActiveFade::Override, InFadeTime);
					}
				}
			}
#if !UE_BUILD_SHIPPING
			else
			{
				UE_LOG(LogAtomModulation, Verbose, TEXT("Update to '%s' Ignored: Control Bus Mix is inactive."), *MixSettings.GetName().ToString());
			}
#endif // !UE_BUILD_SHIPPING
		});
	}

	void FAtomModulationSystem::UpdateModulator(const UAtomModulatorBase& InModulator)
	{
		if (const UAtomModulationGenerator* InGenerator = Cast<UAtomModulationGenerator>(&InModulator))
		{
			RunCommandOnProcessingThread([this, GeneratorSettings = FModulationGeneratorSettings(*InGenerator)]() mutable
			{
				FGeneratorHandle GeneratorHandle = FGeneratorHandle::Get(GeneratorSettings.GetId(), RefProxies.Generators);
				if (GeneratorHandle.IsValid())
				{
					GeneratorHandle.FindProxy() = MoveTemp(GeneratorSettings);
				}
#if !UE_BUILD_SHIPPING
				else
				{
					UE_LOG(LogAtomModulation, Verbose, TEXT("Update to '%s' Ignored: Generator is inactive."), *GeneratorSettings.GetName().ToString());
				}
#endif // !UE_BUILD_SHIPPING
			});
		}

		if (const UAtomModulationControlBus* InBus = Cast<UAtomModulationControlBus>(&InModulator))
		{
			RunCommandOnProcessingThread([this, BusSettings = FControlBusSettings(*InBus)]() mutable
			{
				FBusHandle BusHandle = FBusHandle::Get(BusSettings.GetId(), RefProxies.Buses);
				if (BusHandle.IsValid())
				{
					FControlBusProxy& BusProxy = BusHandle.FindProxy();
					BusProxy = MoveTemp(BusSettings);
				}
#if !UE_BUILD_SHIPPING
				else
				{
					UE_LOG(LogAtomModulation, Verbose, TEXT("Update to '%s' Ignored: Control Bus is inactive."), *BusSettings.GetName().ToString());
				}
#endif // !UE_BUILD_SHIPPING
			});
		}

		if (const UAtomModulationPatch* InPatch = Cast<UAtomModulationPatch>(&InModulator))
		{
			RunCommandOnProcessingThread([this, PatchSettings = FModulationPatchSettings(*InPatch)]() mutable
			{
				FPatchHandle PatchHandle = FPatchHandle::Get(PatchSettings.GetId(), RefProxies.Patches);
				if (PatchHandle.IsValid())
				{
					FModulationPatchRefProxy& PatchProxy = PatchHandle.FindProxy();
					PatchProxy = MoveTemp(PatchSettings);
				}
#if !UE_BUILD_SHIPPING
				else
				{
					UE_LOG(LogAtomModulation, Verbose, TEXT("Update to '%s' Ignored: Patch is inactive."), *PatchSettings.GetName().ToString());
				}
#endif // !UE_BUILD_SHIPPING
			});
		}
	}
} // namespace
