
#include "CriWareAtomInsightsEditorModule.h"

#include "Features/IModularFeatures.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "TraceServices/ModuleService.h"
#include "UObject/NameTypes.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#include "CriWareAtomInsightsEditorLog.h"
#include "AtomInsightsStyle.h"
#include "AtomInsightsDashboardAssetCommands.h"
#include "AtomInsightsDashboardFactory.h"
#include "AtomInsightsTraceModule.h"

#include "Views/AudioAnalyzerRackDashboardViewFactory.h"
#include "Views/AudioBusesDashboardViewFactory.h"
#include "Views/AudioMetersDashboardViewFactory.h"
#include "Views/LogDashboardViewFactory.h"
//#include "Views/MonitorViewFactory.h"
#include "Views/SoundDashboardViewFactory.h"
#include "Views/MixerSourceDashboardViewFactory.h"
#include "Views/SubmixesDashboardViewFactory.h"
#include "Views/VirtualLoopDashboardViewFactory.h"
#include "Views/ControlBusDashboardViewFactory.h"
#include "Views/ModulationMatrixDashboardViewFactory.h"

#define LOCTEXT_NAMESPACE "CriWareAtomInsights"

DEFINE_LOG_CATEGORY(LogCriWareAtomInsightsEditor);

void FCriWareAtomInsightsEditorModule::StartupModule()
{
	// Don't run providers in cook commandlet to avoid additional, unnecessary overhead as audio insights is dormant.
	if (!IsRunningCommandlet())
	{
		RegisterMenus();

		DashboardFactory = MakeShared<Atom::Insights::FEditorDashboardFactory>();

		//DashboardFactory->RegisterViewFactory(MakeShared<FViewportDashboardViewFactory>());
		//DashboardFactory->RegisterViewFactory(MakeShared<FMonitorDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FLogDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FSoundDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FMixerSourceDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FVirtualLoopDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FSubmixesDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FAudioBusesDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FAudioMetersDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FAudioAnalyzerRackDashboardViewFactory>());

		// Modulation
		RegisterDashboardViewFactory(MakeShared<Atom::Insights::FControlBusDashboardViewFactory>());
		RegisterDashboardViewFactory(MakeShared<Atom::Insights::FModulationMatrixDashboardViewFactory>());
	}
}

void FCriWareAtomInsightsEditorModule::ShutdownModule()
{
	if (!IsRunningCommandlet())
	{
		DashboardFactory.Reset();
	}
}

void FCriWareAtomInsightsEditorModule::RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory)
{
	DashboardFactory->RegisterViewFactory(InDashboardFactory);
}

void FCriWareAtomInsightsEditorModule::UnregisterDashboardViewFactory(FName InName)
{
	DashboardFactory->UnregisterViewFactory(InName);
}

FAtomRuntimeId FCriWareAtomInsightsEditorModule::GetRuntimeID() const
{
	return DashboardFactory->GetRuntimeID();
}

bool FCriWareAtomInsightsEditorModule::IsModuleLoaded()
{
	return FModuleManager::Get().IsModuleLoaded(AtomInsightsEditorModuleName);
}

FCriWareAtomInsightsEditorModule& FCriWareAtomInsightsEditorModule::GetChecked()
{
	return static_cast<FCriWareAtomInsightsEditorModule&>(FModuleManager::GetModuleChecked<ICriWareAtomInsightsEditorModule>(AtomInsightsEditorModuleName));
}

IAtomInsightsTraceModule& FCriWareAtomInsightsEditorModule::GetTraceModule()
{
	ICriWareAtomInsightsModule& InsightsModule = ICriWareAtomInsightsModule::GetChecked();
	return InsightsModule.GetTraceModule();
}

TSharedRef<Atom::Insights::FEditorDashboardFactory> FCriWareAtomInsightsEditorModule::GetDashboardFactory()
{
	return DashboardFactory->AsShared();
}

const TSharedRef<Atom::Insights::FEditorDashboardFactory> FCriWareAtomInsightsEditorModule::GetDashboardFactory() const
{
	return DashboardFactory->AsShared();
}

TSharedRef<SDockTab> FCriWareAtomInsightsEditorModule::CreateDashboardTabWidget(const FSpawnTabArgs& Args)
{
	return DashboardFactory->MakeDockTabWidget(Args);
}

void FCriWareAtomInsightsEditorModule::RegisterMenus()
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("ADX AtomInsights", FOnSpawnTab::CreateRaw(this, &FCriWareAtomInsightsEditorModule::CreateDashboardTabWidget))
		.SetDisplayName(LOCTEXT("OpenDashboard_TabDisplayName", "ADX Atom Insights"))
		.SetTooltipText(LOCTEXT("OpenDashboard_TabTooltip", "Opens ADX Atom Insights, an extensible suite of tools and visualizers which enable monitoring and debugging ADX Atom audio engine in the Unreal Engine."))
		.SetGroup(MenuStructure.GetToolsCategory())
		.SetIcon(Atom::Insights::FSlateStyle::Get().CreateIcon("AtomInsights.Icon.Dashboard"));
};

#undef LOCTEXT_NAMESPACE // CriWareAtomInsights

IMPLEMENT_MODULE(FCriWareAtomInsightsEditorModule, CriWareAtomInsightsEditor)
