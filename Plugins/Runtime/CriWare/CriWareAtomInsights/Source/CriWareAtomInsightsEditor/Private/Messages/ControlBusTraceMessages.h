
#pragma once

#include "Math/NumericLimits.h"
#include "Trace/Analyzer.h"

#include "CriWareDefines.h"
#include "Messages/AnalyzerMessageQueue.h"
#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	using FBusId = uint32;

	struct FControlBusMessageBase
	{
		FControlBusMessageBase() = default;
		FControlBusMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
			RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
			Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
			ControlBusID = static_cast<FBusId>(EventData.GetValue<uint32>("ControlBusID"));
		}

		FAtomRuntimeId RuntimeID = INDEX_NONE;
		FBusId ControlBusID;
		double Timestamp = 0.0;
	};

	struct FControlBusActivateMessage : public FControlBusMessageBase
	{
		FControlBusActivateMessage() = default;
		FControlBusActivateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FControlBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
			EventData.GetString("Name", BusName);
			EventData.GetString("ParamName", ParamName);
		}

		FString BusName;
		FString ParamName;
	};

	using FControlBusDeactivateMessage = FControlBusMessageBase;

	struct FControlBusUpdateMessage : public FControlBusMessageBase
	{
		FControlBusUpdateMessage() = default;
		FControlBusUpdateMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FControlBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
			Value = EventData.GetValue<float>("Value");
			EventData.GetString("Name", BusName);
			EventData.GetString("ParamName", ParamName);
		}

		FString BusName;
		FString ParamName;
		float Value = 1.0f;
	};

	class FControlBusDashboardEntry : public FSoundAssetDashboardEntry
	{
	public:
		FControlBusDashboardEntry() = default;
		virtual ~FControlBusDashboardEntry() = default;

		FText GetParamNameAsFText() const { return FText::FromString(ParamName); }

		FBusId ControlBusID = INDEX_NONE;
		float Value = 1.0f;
		FString ParamName;
	};

	class FControlBusMessages
	{
		TAnalyzerMessageQueue<FControlBusUpdateMessage> UpdateMessages{ 2.0 };
		TAnalyzerMessageQueue<FControlBusActivateMessage> ActivateMessages{ 0.1 };
		TAnalyzerMessageQueue<FControlBusDeactivateMessage> DeactivateMessages{ 0.1 };

		friend class FControlBusTraceProvider;
	};

} // namespace
