
#pragma once

#include "Misc/Build.h"

#if !UE_BUILD_SHIPPING

#include "Templates/SharedPointer.h"

#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"
#include "Atom/Modulation/AtomModulationGenerator.h"
#include "AtomModulationProxy.h"

// Forward Declarations
class UViewportStatsSubsystem;

namespace AtomModulation
{
	// Forward Declarations
	struct FReferencedProxies;

	struct FControlBusMixStageDebugInfo
	{
		float TargetValue;
		float CurrentValue;
	};

	struct FControlBusMixDebugInfo
	{
		FString Name;
		uint32 Id;
		uint32 RefCount;

		TMap<uint32, FControlBusMixStageDebugInfo> Stages;
	};

	struct FControlBusDebugInfo
	{
		FString Name;
		float DefaultValue;
		float GeneratorValue;
		float MixValue;
		float Value;
		uint32 Id;
		uint32 RefCount;
	};

	struct FGeneratorSort
	{
		FORCEINLINE bool operator()(const FString& A, const FString& B) const
		{
			return A < B;
		}
	};

	struct FGeneratorDebugInfo
	{
		FGeneratorDebugInfo()
		{
		}

		FGeneratorDebugInfo(const TArray<FString>& InCategories)
			: Categories(InCategories)
		{
		}

		bool bEnabled = false;
		TArray<FString> Categories;

		FString NameFilter;

		using FInstanceValues = TArray<FString>;
		TArray<FInstanceValues> FilteredInstances;
	};

	class FAtomModulationDebugger : public TSharedFromThis<FAtomModulationDebugger, ESPMode::ThreadSafe>
	{
	public:
		FAtomModulationDebugger();

		void UpdateDebugData(double InElapsed, const FReferencedProxies& InRefProxies);
		void SetDebugBusFilter(const FString* InNameFilter);
		void SetDebugMatrixEnabled(bool bInIsEnabled);
		void SetDebugMixFilter(const FString* InNameFilter);
		void SetDebugGeneratorsEnabled(bool bInIsEnabled);
		void SetDebugGeneratorFilter(const FString* InFilter);
		void SetDebugGeneratorTypeFilter(const FString* InFilter, bool bInIsEnabled);
		bool OnPostHelp(FCommonViewportClient& ViewportClient, const TCHAR* Stream);
		int32 OnRenderStat(FCanvas& Canvas, int32 X, int32 Y, const UFont& Font);
		int32 OnRenderStat(UViewportStatsSubsystem* ViewportSubSystem, int32 OffsetX, int32 OffsetY);
		bool OnToggleStat(FCommonViewportClient& ViewportClient, const TCHAR* Stream);

		void ResetGeneratorStats();

	private:
		uint8 bActive : 1;
		uint8 bShowRenderStatMix : 1;
		uint8 bShowGenerators : 1;
		uint8 bEnableAllGenerators : 1;

		TArray<FControlBusDebugInfo> FilteredBuses;
		TArray<FControlBusMixDebugInfo> FilteredMixes;

		using FGeneratorSortMap = TSortedMap<FString, FGeneratorDebugInfo, FDefaultAllocator, FGeneratorSort>;
		FGeneratorSortMap FilteredGeneratorsMap;

		TMap<FString, bool> RequestedGeneratorUpdate;

		FString BusStringFilter;
		FString GeneratorStringFilter;
		FString MixStringFilter;

		float ElapsedSinceLastUpdate;
	};

} // namespace

#endif // !UE_BUILD_SHIPPING
