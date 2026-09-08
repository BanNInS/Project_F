
#pragma once

#include "TraceServices/ModuleService.h"

#include "AtomInsightsTraceProviderBase.h"

namespace Atom::Insights
{
	class FTraceProviderBase;
} // namespace

namespace TraceServices
{
	struct FSessionInfo;
} // namespace

class CRIWAREATOMINSIGHTS_API IAtomInsightsTraceModule : public TraceServices::IModule
{
public:
	virtual void AddTraceProvider(TSharedPtr<Atom::Insights::FTraceProviderBase> TraceProvider) = 0;

	virtual void StartTraceAnalysis(const bool bOnlyTraceAtomChannels) = 0;
	virtual bool IsTraceAnalysisActive() const = 0;
	virtual void StopTraceAnalysis() = 0;

	virtual void OnOnlyTraceAtomChannelsStateChanged(const bool bOnlyTraceAtomChannels) = 0;

	virtual bool AtomChannelsCanBeManuallyEnabled() const = 0;

#if !WITH_EDITOR
	virtual void InitializeSessionInfo(const TraceServices::FSessionInfo& SessionInfo) = 0;
	virtual void RequestChannelUpdate() = 0;
	virtual void ResetTicker() = 0;
	virtual bool TraceControllerIsAvailable() const = 0;
#endif // !WITH_EDITOR
};
