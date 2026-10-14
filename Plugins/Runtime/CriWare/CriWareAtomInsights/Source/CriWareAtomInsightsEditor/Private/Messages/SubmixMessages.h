
#pragma once

#include "Math/NumericLimits.h"
#include "Trace/Analyzer.h"

#include "CriWareDefines.h"

#include "Messages/AnalyzerMessageQueue.h"
#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	struct FSubmixMessageBase
	{
		FSubmixMessageBase() = default;

		FSubmixMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
			SubmixID = EventData.GetValue<uint32>("SubmixID");
			Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
		}

		FAtomRuntimeId RuntimeID = INDEX_NONE;
		uint32 SubmixID = INDEX_NONE;
		double Timestamp = 0.0;
	};

	struct FSubmixHasActivityMessage : public FSubmixMessageBase
	{
		FSubmixHasActivityMessage() = default;

		FSubmixHasActivityMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FSubmixMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			bHasActivity = EventData.GetValue<bool>("HasActivity");
		}

		bool bHasActivity = false;
	};

	class FSubmixMessages
	{
		TAnalyzerMessageQueue<FSubmixHasActivityMessage> HasActivityMessages{ 0.1 };

		friend class FSubmixProvider;
	};

	struct FSubmixAssetDashboardEntry : public FSoundAssetDashboardEntry
	{
		FSubmixAssetDashboardEntry() = default;
		virtual ~FSubmixAssetDashboardEntry() = default;

		uint32 SubmixID = INDEX_NONE;
		bool bHasActivity = false;
		bool bIsBus = false;
	};
} // namespace
