
#pragma once

#include "AtomInsightsTraceProviderBase.h"
#include "Messages/ControlBusTraceMessages.h"

namespace Atom::Insights
{
	class FControlBusTraceProvider
		: public TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FControlBusDashboardEntry>>
		, public TSharedFromThis<FControlBusTraceProvider>
	{
	public:
		FControlBusTraceProvider()
			: TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FControlBusDashboardEntry>>(GetName_Static())
		{

		}

		virtual ~FControlBusTraceProvider() = default;
		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) override;

		virtual bool ProcessMessages() override;
		static FName GetName_Static();

	private:

		FControlBusMessages TraceMessages;
	};
} // namespace
