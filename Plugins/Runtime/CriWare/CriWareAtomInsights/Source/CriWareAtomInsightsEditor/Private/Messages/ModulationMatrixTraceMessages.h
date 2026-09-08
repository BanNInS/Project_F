
#pragma once

#include "Trace/Analyzer.h"

#include "CriWareDefines.h"
#include "Messages/AnalyzerMessageQueue.h"
#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	using FBusId    = uint32;
	using FSourceId = uint32;

	enum class EModulationMatrixEntryType : uint8
	{
		// Arranged alphabetically to simplify sorting
		BusMix,
		BusFinalValues,
		Generator,
		None
	};


	// Modulation matrix dashboard entry
	class FModulationMatrixDashboardEntry : public FSoundAssetDashboardEntry
	{
	public:
		FModulationMatrixDashboardEntry() = default;
		virtual ~FModulationMatrixDashboardEntry() = default;

		FSourceId SourceID = INDEX_NONE;
		EModulationMatrixEntryType EntryType = EModulationMatrixEntryType::None;

		TMap<FBusId, float> BusIDToValueMap;
	};
	

	// Modulation matrix message base
	struct FModulationMatrixMessageBase
	{
		FModulationMatrixMessageBase() = default;

		FModulationMatrixMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
			Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
			SourceID  = static_cast<FSourceId>(EventData.GetValue<uint32>("SourceID"));
		}

		FAtomRuntimeId RuntimeID = INDEX_NONE;
		FSourceId SourceID = INDEX_NONE;
		double Timestamp = 0.0;
	};


	// Register bus message
	struct FModulationMatrixRegisterBusMessage : public FModulationMatrixMessageBase
	{
		FModulationMatrixRegisterBusMessage() = default;

		FModulationMatrixRegisterBusMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			ModulatingSourceID = static_cast<FSourceId>(EventData.GetValue<uint32>("ModulatingSourceID"));
			EventData.GetString("BusName", BusName);
		}

		FSourceId ModulatingSourceID = INDEX_NONE;
		FString BusName;
	};


	// Activate messages
	struct FModulationMatrixActivateMessage : public FModulationMatrixMessageBase
	{
		FModulationMatrixActivateMessage() = default;

		FModulationMatrixActivateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			EventData.GetString("Name", Name);
		}

		EModulationMatrixEntryType EntryType = EModulationMatrixEntryType::None;
		FString Name;
	};

	struct FBusMixActivateMessage : public FModulationMatrixActivateMessage
	{
		FBusMixActivateMessage() = default;

		FBusMixActivateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixActivateMessage(InContext)
		{
			EntryType = EModulationMatrixEntryType::BusMix;
		}
	};

	struct FGeneratorActivateMessage : public FModulationMatrixActivateMessage
	{
		FGeneratorActivateMessage() = default;

		FGeneratorActivateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixActivateMessage(InContext)
		{
			EntryType = EModulationMatrixEntryType::Generator;
		}
	};


	// Update messages
	struct FModulationMatrixUpdateMessage : public FModulationMatrixMessageBase
	{
		FModulationMatrixUpdateMessage() = default;

		FModulationMatrixUpdateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			const TArrayView<const FBusId> BusIDs   = EventData.GetArrayView<FBusId>("BusIDs");
			const TArrayView<const float> BusValues = EventData.GetArrayView<float>("BusValues");

			check(BusIDs.Num() == BusValues.Num());

			for (int32 Index = 0; Index < BusIDs.Num(); ++Index)
			{
				BusIDToValueMap.Emplace(BusIDs[Index], BusValues[Index]);
			}
		}

		TMap<FBusId, float> BusIDToValueMap;
	};

	using FBusMixUpdateMessage = FModulationMatrixUpdateMessage;

	struct FGeneratorUpdateMessage : public FModulationMatrixMessageBase
	{
		FGeneratorUpdateMessage() = default;

		FGeneratorUpdateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FModulationMatrixMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			const TArrayView<const FBusId> BusIDs = EventData.GetArrayView<FBusId>("BusIDs");
			const float GeneratorValue = EventData.GetValue<float>("GeneratorValue");

			// All control buses use the same generator value
			for (int32 Index = 0; Index < BusIDs.Num(); ++Index)
			{
				BusIDToValueMap.Emplace(BusIDs[Index], GeneratorValue);
			}
		}

		TMap<FBusId, float> BusIDToValueMap;
	};

	using FBusFinalValuesUpdateMessage = FModulationMatrixUpdateMessage;


	// Deactivate message
	using FModulationMatrixDeactivateMessage = FModulationMatrixMessageBase;


	// Modulation matrix messages
	class FModulationMatrixMessages
	{
		TAnalyzerMessageQueue<FModulationMatrixRegisterBusMessage> RegisterBusMessages{ 0.1 };

		TAnalyzerMessageQueue<FBusMixActivateMessage> BusMixActivateMessages{ 0.1 };
		TAnalyzerMessageQueue<FBusMixUpdateMessage> BusMixUpdateMessages{ 0.1 };

		TAnalyzerMessageQueue<FGeneratorActivateMessage> GeneratorActivateMessages{ 0.1 };
		TAnalyzerMessageQueue<FGeneratorUpdateMessage> GeneratorUpdateMessages{ 0.1 };

		TAnalyzerMessageQueue<FBusFinalValuesUpdateMessage> BusFinalValuesUpdateMessages{ 0.1 };

		TAnalyzerMessageQueue<FModulationMatrixDeactivateMessage> DeactivateMessages{ 0.1 };

		friend class FModulationMatrixTraceProvider;
	};
} // namespace
