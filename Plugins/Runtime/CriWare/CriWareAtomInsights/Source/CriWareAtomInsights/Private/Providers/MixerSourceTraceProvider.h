
#pragma once

#include "UObject/NameTypes.h"

#include "AtomInsightsTraceChannelHandle.h"
#include "AtomInsightsTraceProviderBase.h"
#include "Messages/MixerSourceTraceMessages.h"

namespace Atom::Insights
{
	class FMixerSourceTraceProvider
		: public TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FMixerSourceDashboardEntry>>
		, public TSharedFromThis<FMixerSourceTraceProvider>
	{
	public:
		FMixerSourceTraceProvider()
			: TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FMixerSourceDashboardEntry>>(GetName_Static())
		{

		}

		virtual ~FMixerSourceTraceProvider() = default;
		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) override;

		virtual bool ProcessMessages() override;

		static FName GetName_Static();

#if !WITH_EDITOR
		virtual void InitSessionCachedMessages(TraceServices::IAnalysisSession& InSession) override;
#endif // !WITH_EDITOR

	private:
#if !WITH_EDITOR
		virtual void OnTimingViewTimeMarkerChanged(double TimeMarker) override;

		TUniquePtr<FMixerSourceSessionCachedMessages> SessionCachedMessages;
#endif // !WITH_EDITOR

		FMixerSourceMessages TraceMessages;
	};
} // namespace
