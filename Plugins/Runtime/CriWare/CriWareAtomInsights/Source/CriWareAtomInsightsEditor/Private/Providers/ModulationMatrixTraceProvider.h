
#pragma once

#include "Containers/Map.h"
#include "Delegates/DelegateCombinations.h"

#include "AtomInsightsTraceProviderBase.h"
#include "Messages/ModulationMatrixTraceMessages.h"

namespace Atom::Insights
{
	class FModulationMatrixTraceProvider
		: public TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FModulationMatrixDashboardEntry>>
		, public TSharedFromThis<FModulationMatrixTraceProvider>
	{
	public:
		FModulationMatrixTraceProvider();

		virtual ~FModulationMatrixTraceProvider();

		static FName GetName_Static();

		virtual bool ProcessMessages() override;
		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) override;

		struct BusInfo
		{
			FString BusName;
			int32 RefCount = 0;
		};

		using BusIdToBusInfoMap = TMap<FBusId, BusInfo>;

		DECLARE_DELEGATE_OneParam(FOnControlBusesAdded, const BusIdToBusInfoMap& /*AddedControlBuses*/);
		FOnControlBusesAdded OnControlBusesAdded;

		DECLARE_DELEGATE_OneParam(FOnControlBusesRemoved, const TArray<FName>& /*RemovedControlBusesNames*/);
		FOnControlBusesRemoved OnControlBusesRemoved;

	private:
		void UpdateActiveControlBusesToAdd(const TMap<FBusId, FString>& InBusIDToBusNameMap);
		void UpdateActiveControlBusesToRemove(const TMap<FBusId, float>& InBusIDToValueMap);
		void OnAtomRuntimeDestroyed(FAtomRuntimeId InRuntimeID);

		FModulationMatrixMessages TraceMessages;

		TMap<FAtomRuntimeId, TSet<uint32>> RuntimeIDToActiveModulatorSourceIDsMap;
		BusIdToBusInfoMap ActiveControlBuses;
		TArray<FName> RemovedControlBusesNames;
	};
} // namespace
