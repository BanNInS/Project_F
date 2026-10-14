#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "HAL/CriticalSection.h"
#include "UObject/GCObject.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "AtomModulation.h"
#include "Atom/Modulation/AtomModulationControlBusProxy.h"
#include "Atom/Modulation/AtomModulationControlBusMixProxy.h"
#include "Atom/Modulation/AtomModulationGeneratorProxy.h"
#include "Atom/Modulation/AtomModulationPatchProxy.h"

// Forward Declarations
class UAtomModulationControlBus;
class UAtomModulationControlBusMix;
class UAtomModulationGenerator;
class UViewportStatsSubsystem;

namespace AtomModulation
{
	struct FControlBusSettings;
	struct FModulationGeneratorSettings;
	struct FModulationPatchSettings;

	class FControlBusProxy;
	class FModulationInputProxy;
	class FModulationPatchProxy;
	class FModulationPatchRefProxy;
	class FModulatorBusMixStageProxy;

	using FModulatorHandleSet = TSet<Atom::FModulatorHandleId>;

	struct FReferencedProxies
	{
		FBusMixProxyMap BusMixes;
		FBusProxyMap Buses;
		FGeneratorProxyMap Generators;
		FPatchProxyMap Patches;
	};

	struct FReferencedModulators
	{
		TMap<FPatchHandle, FModulatorHandleSet> PatchMap;
		TMap<FBusHandle, FModulatorHandleSet> BusMap;
		TMap<FGeneratorHandle, FModulatorHandleSet> GeneratorMap;
	};

#if !UE_BUILD_SHIPPING
	class FAtomModulationDebugger;
#endif
}

#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN_EXTERN(CriWareAtom, ModulatingSourceDeactivate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_END()
#endif

namespace AtomModulation
{
	class FAtomModulationSystem : public TSharedFromThis<FAtomModulationSystem>, public FGCObject
	{
	public:
		~FAtomModulationSystem();

		void Initialize(FAtomRuntimeId RumtimeID);

		UE_DEPRECATED(5.0, "Activation of modulators in this manner is now deprecated. Use USoundModulationDestinations to safely activate and track a given modulator")
		void ActivateBus(const UAtomModulationControlBus& InBus);

		void ActivateBusMix(FModulatorBusMixSettings&& InSettings);
		void ActivateBusMix(const UAtomModulationControlBusMix& InBusMix);

		UE_DEPRECATED(5.0, "Activation of modulators in this manner is now deprecated. Use USoundModulationDestinations to safely activate and track a given modulator")
		void ActivateGenerator(const UAtomModulationGenerator& InGenerator);

		// FGCObject interface
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
		virtual FString GetReferencerName() const override;
		// End of FGCObject interface

		/**
		 * Deactivates respectively typed (i.e. BusMix, Bus, Generator, etc.) object proxy if no longer referenced.
		 * If still referenced, will wait until references are finished before destroying.
		 */
		UE_DEPRECATED(5.0, "Deactivation of modulators in this manner is now deprecated. Use USoundModulationDestinations to safely activate and track a given modulator")
		void DeactivateBus(const UAtomModulationControlBus& InBus);

		/** Deactivates given bus mix */
		void DeactivateBusMix(const UAtomModulationControlBusMix& InBusMix);

		/** Deactivates all bus mixes */
		void DeactivateAllBusMixes();

		UE_DEPRECATED(5.0, "Deactivation of modulators in this manner is now deprecated. Use USoundModulationDestinations to safely activate and track a given modulator")
		void DeactivateGenerator(const UAtomModulationGenerator& InGenerator);

		void ProcessModulators(const double InElapsed);
		void SoloBusMix(const UAtomModulationControlBusMix& InBusMix);

		FAtomRuntimeId GetAtomRuntimeID() const;

		/* Register new handle with given a given modulator that may or may not already be active (i.e. registered).
		 * If already registered, depending on modulator type, may or may not refresh proxy based on provided settings.
		 */
		Atom::FModulatorTypeId RegisterModulator(Atom::FModulatorHandleId InHandleId, const FControlBusSettings& InSettings);
		Atom::FModulatorTypeId RegisterModulator(Atom::FModulatorHandleId InHandleId, const FModulationGeneratorSettings& InSettings);
		Atom::FModulatorTypeId RegisterModulator(Atom::FModulatorHandleId InHandleId, const FModulationPatchSettings& InSettings);

		/* Register new handle with given Id with a modulator that is already active (i.e. registered). Used primarily for copying modulation handles. */
		void RegisterModulator(Atom::FModulatorHandleId InHandleId, Atom::FModulatorId InModulatorId);

		/* Attempts to get the modulator value from the AudioRender Thread. */
		bool GetModulatorValue(const Atom::FModulatorHandle& ModulatorHandle, float& OutValue) const;

		/* Attempts to get the modulator value from any thread (lock contentious).*/
		bool GetModulatorValueThreadSafe(const Atom::FModulatorHandle& ModulatorHandle, float& OutValue) const;

		/* Attempts to get the modulator value from any thread (lock contentious).*/
		bool GetModulatorValueThreadSafe(uint32 ModulatorID, float& OutValue) const;

		void UnregisterModulator(const Atom::FModulatorHandle& InHandle);

		bool IsControlBusMixActive(const UAtomModulationControlBusMix& InBusMix);

		/* Saves mix to .ini profile for fast iterative development that does not require re-cooking a mix */
		void SaveMixToProfile(const UAtomModulationControlBusMix& InBusMix, const int32 InProfileIndex);

		/* Loads mix from .ini profile for iterative development that does not require re-cooking a mix. Returns copy
		 * of mix stage values saved in profile. */
		TArray<FAtomModulationControlBusMixStage> LoadMixFromProfile(const int32 InProfileIndex, UAtomModulationControlBusMix& OutBusMix);

		 /*
		  * Updates mix/mix by filter, modifying the mix instance if it is active. If bInUpdateObject is true,
		  * updates UObject definition in addition to proxy.
		  */
		void UpdateMix(const TArray<FAtomModulationControlBusMixStage>& InStages, UAtomModulationControlBusMix& InOutMix, bool bInUpdateObject = false, float InFadeTime = -1.0f, double Duration = -1.0, bool bRetriggerOnActivation = false);
		void UpdateMixByFilter(const FString& InAddressFilter, const TSubclassOf<UAtomModulationParameter>& InParamClassFilter, UAtomModulationParameter* InParamFilter, float Value, float FadeTime, UAtomModulationControlBusMix& InOutMix, bool bInUpdateObject = false);

		/*
		 * Commits any changes from a mix applied to a UObject definition to mix instance if active.
		 */
		void UpdateMix(const UAtomModulationControlBusMix& InMix, float InFadeTime = -1.0f);

		/* Sets the global bus mix value if over the prescribed time. If FadeTime is non-positive, applies the value immediately. */
		void SetGlobalBusMixValue(UAtomModulationControlBus& Bus, float Value, float FadeTime = -1.0f);

		/* Clears the global bus mix value over the prescribed FadeTime. If FadeTime is non-positive, returns to the bus's respective parameter default immediately. */
		void ClearGlobalBusMixValue(const UAtomModulationControlBus& InBus, float FadeTime = -1.0f);

		/* Clears all global bus mix values over the prescribed FadeTime. If FadeTime is non-positive, returns to the bus's respective parameter default immediately. */
		void ClearAllGlobalBusMixValues(float FadeTime = -1.0f);

		/* Create a mix from an array of buses where a supplied default value and associated timing parameters are applied for each bus's stage */
		UAtomModulationControlBusMix* CreateBusMixFromValue(FName Name, const TArray<UAtomModulationControlBus*>& Buses, float Value, float AttackTime = -1.0f, float ReleaseTime = -1.0f);

		/*
		 * Commits any changes from a modulator type applied to a UObject definition
		 * to modulator instance if active (i.e. Control Bus, Control Bus Modulator)
		 */
		void UpdateModulator(const UAtomModulatorBase& InModulator);

		void OnAuditionEnd();

	private:
		/* Calculates modulation value, storing it in the provided float reference and returns if value changed */
		bool CalculateModulationValue(FModulationPatchProxy& OutProxy, float& OutValue) const;

		/* Whether or not caller is in processing thread or not */
		bool IsInProcessingThread() const;

		/* Runs the provided command on the audio render thread (at the beginning of the ProcessModulators call) */
		void RunCommandOnProcessingThread(TUniqueFunction<void()> Cmd);

		void OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination);

		/* Template for register calls that move the modulator settings objects onto the Audio Processing Thread & create proxies.
		 * If the proxy is already found, adds provided HandleId as reference to given proxy. Does *not* update the proxy with the
		 * given settings.  If update is desired on an existing proxy, UpdateModulator must be explicitly called.
		 * @tparam THandleType Type of modulator handle used to access the proxy on the Processing Thread
		 * @tparam TModSettings Modulation settings to move and use if proxy construction is required on the Audio Processing Thread
		 * @tparam TMapType MapType used to cache the corresponding proxy id & proxy
		 * @tparam TInitFunction (Optional) Function type used to call on the AudioProcessingThread immediately following proxy construction
		 * @param InHandleId HandleId associated with the proxy to be retrieved/generated.
		 * @param InModSettings ModulatorSettings to be used if construction of proxy is required on Audio Processing Thread
		 * @param OutProxyMap Map to find or add proxy to if constructed
		 * @param InInitFunction Function type used to call on the AudioProcessingThread immediately following proxy generation
		 */
		template <typename THandleType, typename TModSettings, typename TMapType, typename TInitFunction = TUniqueFunction<void(THandleType)>>
		void RegisterModulator(Atom::FModulatorHandleId InHandleId, TModSettings&& InModSettings, TMapType& OutProxyMap, TMap<THandleType, FModulatorHandleSet>& OutModMap, TInitFunction InInitFunction = TInitFunction())
		{
			check(InHandleId != INDEX_NONE);

			RunCommandOnProcessingThread([
				this,
				InHandleId,
				ModSettings = MoveTemp(InModSettings),
				InitFunction = MoveTemp(InInitFunction),
				PassedProxyMap = &OutProxyMap,
				PassedModMap = &OutModMap
			]() mutable
				{
					check(PassedProxyMap);
					check(PassedModMap);

					THandleType Handle = THandleType::Create(MoveTemp(ModSettings), *PassedProxyMap, *this);
					PassedModMap->FindOrAdd(Handle).Add(InHandleId);
				});
		}

		template <typename THandleType>
		bool UnregisterModulator(THandleType InModHandle, TMap<THandleType, FModulatorHandleSet>& OutHandleMap, const Atom::FModulatorHandleId InHandleId)
		{
			bool bHandleRemoved = false;

			if (!InModHandle.IsValid())
			{
				return bHandleRemoved;
			}

			if (FModulatorHandleSet* HandleSet = OutHandleMap.Find(InModHandle))
			{
				bHandleRemoved = HandleSet->Remove(InHandleId) > 0;
				if (HandleSet->IsEmpty())
				{
					OutHandleMap.Remove(InModHandle);
				}
			}

			return bHandleRemoved;
		}

		FReferencedProxies RefProxies;

		// Critical section & map of copied, computed modulation values for
		// use from the decoder threads & respective MetaSound getter nodes.
		mutable FCriticalSection ThreadSafeModValueCritSection;
		TMap<Atom::FModulatorId, float> ThreadSafeModValueMap;

		TSet<FBusMixId> ActiveBusMixIDs;

		TSet<FBusHandle> ManuallyActivatedBuses;
		TSet<FBusMixHandle> ManuallyActivatedBusMixes;
		TSet<FGeneratorHandle> ManuallyActivatedGenerators;

		// Global mixes each containing a single stage for any globally manipulated bus stage value.
		TMap<uint32, TObjectPtr<UAtomModulationControlBusMix>> ActiveGlobalBusValueMixes;

		// Command queue to be consumed on processing thread 
		TQueue<TUniqueFunction<void()>, EQueueMode::Mpsc> ProcessingThreadCommandQueue;

		// Thread modulators are processed on
		TAtomic<uint32> ProcessingThreadId { 0 };

		// Collection of maps with modulator handles to referencing object ids used by externally managing objects
		FReferencedModulators RefModulators;

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;

		// Registered Parameters
		using FParameterRegistry = TMap<FName, Atom::FModulationParameter>;
		FParameterRegistry ParameterRegistry;

#if !UE_BUILD_SHIPPING
	public:
		void SetDebugBusFilter(const FString* InFilter);
		void SetDebugMixFilter(const FString* InFilter);
		void SetDebugMatrixEnabled(bool bInIsEnabled);
		void SetDebugGeneratorsEnabled(bool bInIsEnabled);
		void SetDebugGeneratorFilter(const FString* InFilter);
		void SetDebugGeneratorTypeFilter(const FString* InFilter, bool bInIsEnabled);
		bool OnPostHelp(FCommonViewportClient* ViewportClient, const TCHAR* Stream);
		int OnRenderStat(UViewportStatsSubsystem* ViewportSubSystem, int32 OffsetX, int32 OffsetY);
		int OnRenderStat(FViewport* Viewport, FCanvas* Canvas, int32 X, int32 Y, const UFont& Font);
		bool OnToggleStat(FCommonViewportClient* ViewportClient, const TCHAR* Stream);

	private:
		TSharedPtr<FAtomModulationDebugger> Debugger;
#endif // !UE_BUILD_SHIPPING

		friend FControlBusProxy;
		friend FModulationInputProxy;
		friend FModulationPatchProxy;
		friend FModulationPatchRefProxy;
		friend FModulatorBusMixStageProxy;
	};
} // namespace
