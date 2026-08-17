
#pragma once 

#include "Framework/Docking/TabManager.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Docking/SDockTab.h"

#include "IAtomInsightsTraceModule.h"
#include "Views/DashboardViewFactory.h"
#include "Views/MixerSourceMuteSoloFilter.h"
#include "Views/VirtualLoopsDebugDraw.h"

#include "AtomInsightsEditorDashboardFactory.h"
#include "ICriWareAtomInsightsEditorModule.h"

class FCriWareAtomInsightsEditorModule final : public ICriWareAtomInsightsEditorModule
{
public:
	FCriWareAtomInsightsEditorModule() = default;
	virtual ~FCriWareAtomInsightsEditorModule() = default;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual void RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory) override;
	virtual void UnregisterDashboardViewFactory(FName InName) override;
	virtual FAtomRuntimeId GetRuntimeID() const override;

	TSharedRef<Atom::Insights::FEditorDashboardFactory> GetDashboardFactory();
	const TSharedRef<Atom::Insights::FEditorDashboardFactory> GetDashboardFactory() const;

	static bool IsModuleLoaded();
	static FCriWareAtomInsightsEditorModule& GetChecked();
	virtual IAtomInsightsTraceModule& GetTraceModule() override;

	
private:
	void RegisterMenus();
	virtual TSharedRef<SDockTab> CreateDashboardTabWidget(const FSpawnTabArgs& Args);

	inline static const FName AtomInsightsEditorModuleName = "CriWareAtomInsightsEditor";

	TSharedPtr<Atom::Insights::FEditorDashboardFactory> DashboardFactory;
	Atom::Insights::FVirtualLoopsDebugDraw VirtualLoopsDebugDraw;
	Atom::Insights::FMuteSoloFilter MuteSoloFilter;
};
