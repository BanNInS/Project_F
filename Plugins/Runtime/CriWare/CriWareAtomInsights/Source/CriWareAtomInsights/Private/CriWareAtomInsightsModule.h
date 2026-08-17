
#pragma once 

#include "Framework/Docking/TabManager.h"
#include "Insights/IUnrealInsightsModule.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "Widgets/Docking/SDockTab.h"

#include "Views/DashboardViewFactory.h"
#include "AtomInsightsDashboardFactory.h"
#include "AtomInsightsTraceModule.h"
#include "ICriWareAtomInsightsModule.h"

#if !WITH_EDITOR
#include "AtomInsightsTimingViewExtender.h"
#endif // !WITH_EDITOR

#if !WITH_EDITOR
namespace Atom::Insights
{
	class FAtomInsightsComponent;
}
#endif // !WITH_EDITOR

class FCriWareAtomInsightsModule final : public ICriWareAtomInsightsModule
{
public:
	FCriWareAtomInsightsModule() = default;
	virtual ~FCriWareAtomInsightsModule() = default;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory) override;
	virtual void UnregisterDashboardViewFactory(FName InName) override;
	virtual FAtomRuntimeId GetRuntimeID() const override;

	static FCriWareAtomInsightsModule& GetChecked();
	static FCriWareAtomInsightsModule* GetModulePtr();
	virtual IAtomInsightsTraceModule& GetTraceModule();

#if !WITH_EDITOR
	TSharedPtr<FAtomInsightsComponent> GetAtomInsightsComponent() { return AtomInsightsComponent; };
	FAtomInsightsTimingViewExtender& GetTimingViewExtender() { return AtomInsightsTimingViewExtender; };
#endif // !WITH_EDITOR

	TSharedRef<Atom::Insights::FDashboardFactory> GetDashboardFactory();
	const TSharedRef<Atom::Insights::FDashboardFactory> GetDashboardFactory() const;

	virtual TSharedRef<SDockTab> CreateDashboardTabWidget(const FSpawnTabArgs& Args);

private:
	TSharedPtr<Atom::Insights::FDashboardFactory> DashboardFactory;
	TUniquePtr<Atom::Insights::FTraceModule> TraceModule;

#if !WITH_EDITOR
	TSharedPtr<FAtomInsightsComponent> AtomInsightsComponent;
	FAtomInsightsTimingViewExtender AtomInsightsTimingViewExtender;
#endif // !WITH_EDITOR
};
