
#include "AtomInsightsTimingViewExtender.h"

#include "Insights/ITimingViewSession.h"
#include "TraceServices/Model/AnalysisSession.h"

namespace Atom::Insights
{
	void FAtomInsightsTimingViewExtender::OnBeginSession(UE::Insights::Timing::ITimingViewSession& InSession)
	{
		InSession.OnTimeMarkerChanged().AddRaw(this, &FAtomInsightsTimingViewExtender::OnTimeMarkerChanged);
	}

	void FAtomInsightsTimingViewExtender::OnEndSession(UE::Insights::Timing::ITimingViewSession& InSession)
	{
		InSession.OnTimeMarkerChanged().RemoveAll(this);
	}

	void FAtomInsightsTimingViewExtender::Tick(UE::Insights::Timing::ITimingViewSession& InSession, const TraceServices::IAnalysisSession& InAnalysisSession)
	{
		AnalysisSession = &InAnalysisSession;
	}

	void FAtomInsightsTimingViewExtender::OnTimeMarkerChanged(UE::Insights::Timing::ETimeChangedFlags InFlags, double InTimeMarker)
	{
		if (AnalysisSession == nullptr)
		{
			return;
		}
		
		TraceServices::FAnalysisSessionReadScope SessionReadScope(*AnalysisSession);

		const bool bIsValidTime = InTimeMarker >= 0.0 && InTimeMarker <= AnalysisSession->GetDurationSeconds();

		// Scrub Audio Insights when all data is loaded in non-live sessions
		if (AnalysisSession->IsAnalysisComplete() && bIsValidTime)
		{
			OnTimingViewTimeMarkerChanged.Broadcast(InTimeMarker);
		}
	}
} // namespace
