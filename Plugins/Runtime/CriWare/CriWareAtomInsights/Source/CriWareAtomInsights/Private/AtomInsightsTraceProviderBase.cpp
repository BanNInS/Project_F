
#include "AtomInsightsTraceProviderBase.h"

#if !WITH_EDITOR
#include "Modules/ModuleManager.h"
#include "CriWareAtomInsightsModule.h"
#include "AtomInsightsTimingViewExtender.h"
#endif // !WITH_EDITOR

namespace Atom::Insights
{
	FTraceProviderBase::FTraceProviderBase(FName InName)
		: Name(InName)
	{
#if !WITH_EDITOR
		FCriWareAtomInsightsModule& AtomInsightsModule = FCriWareAtomInsightsModule::GetChecked();
		FAtomInsightsTimingViewExtender& AtomInsightsTimingViewExtender = AtomInsightsModule.GetTimingViewExtender();
		AtomInsightsTimingViewExtender.OnTimingViewTimeMarkerChanged.AddRaw(this, &FTraceProviderBase::OnTimingViewTimeMarkerChanged);
#endif // !WITH_EDITOR
	}

	FTraceProviderBase::~FTraceProviderBase()
	{
#if !WITH_EDITOR
		if (FModuleManager::Get().IsModuleLoaded("CriWareAtomInsights"))
		{
			FCriWareAtomInsightsModule& AtomInsightsModule = FCriWareAtomInsightsModule::GetChecked();
			FAtomInsightsTimingViewExtender& AtomInsightsTimingViewExtender = AtomInsightsModule.GetTimingViewExtender();
			AtomInsightsTimingViewExtender.OnTimingViewTimeMarkerChanged.RemoveAll(this);
		}
#endif // !WITH_EDITOR
	}

	FName FTraceProviderBase::GetName() const
	{
		return Name;
	}

	FTraceProviderBase::FTraceAnalyzerBase::FTraceAnalyzerBase(TSharedRef<FTraceProviderBase> InProvider)
		: Provider(InProvider)
	{
	}

	void FTraceProviderBase::FTraceAnalyzerBase::OnAnalysisBegin(const FOnAnalysisContext& Context)
	{
		Provider->Reset();
	}

	bool FTraceProviderBase::FTraceAnalyzerBase::OnEventSuccess(uint16 RouteId, EStyle Style, const FOnEventContext& Context)
	{
		++(Provider->LastMessageID);
		return true;
	}

	bool FTraceProviderBase::FTraceAnalyzerBase::OnEventFailure(uint16 RouteId, EStyle Style, const FOnEventContext& Context)
	{
		++(Provider->LastMessageID);

		const FString ProviderName = Provider->GetName().ToString();
		constexpr bool bEventSucceeded = false;
		ensureAlwaysMsgf(bEventSucceeded, TEXT("'%s' TraceProvider's Analyzer message with RouteId '%u' event not handled"), *ProviderName, RouteId);
		return bEventSucceeded;
	}
} // namespace
