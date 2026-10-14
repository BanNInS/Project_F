
#include "CriWareAtomInsightsModule.h"

#include "Features/IModularFeatures.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "TraceServices/ModuleService.h"
#include "UObject/NameTypes.h"

#include "CriWareAtomInsightsLog.h"
#include "AtomInsightsDashboardAssetCommands.h"
#include "AtomInsightsDashboardFactory.h"
#include "AtomInsightsTraceModule.h"

#if !WITH_EDITOR
#include "AtomInsightsComponent.h"
#include "Views/SoundDashboardViewFactory.h"
#include "Views/MixerSourceDashboardViewFactory.h"
#include "Views/VirtualLoopDashboardViewFactory.h"
#endif // !WITH_EDITOR

#define LOCTEXT_NAMESPACE "CriWareAtomInsights"
DEFINE_LOG_CATEGORY(LogCriWareAtomInsights);

void FCriWareAtomInsightsModule::StartupModule()
{
	// Don't run providers in cook commandlet to avoid additional, unnecessary overhead as audio insights is dormant.
	if (!IsRunningCommandlet())
	{
		TraceModule = MakeUnique<Atom::Insights::FTraceModule>();
		//RewindDebuggerExtension = MakeUnique<FRewindDebugger>();

		IModularFeatures::Get().RegisterModularFeature(TraceServices::ModuleFeatureName, TraceModule.Get());
		//IModularFeatures::Get().RegisterModularFeature(IRewindDebuggerRuntimeExtension::ModularFeatureName, RewindDebuggerExtension.Get());

		DashboardFactory = MakeShared<Atom::Insights::FDashboardFactory>();

		Atom::Insights::FDashboardAssetCommands::Register();

#if !WITH_EDITOR
		IModularFeatures::Get().RegisterModularFeature(UE::Insights::Timing::TimingViewExtenderFeatureName, &AtomInsightsTimingViewExtender);

		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FSoundDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FMixerSourceDashboardViewFactory>());
		DashboardFactory->RegisterViewFactory(MakeShared<Atom::Insights::FVirtualLoopDashboardViewFactory>());

		AtomInsightsComponent = Atom::Insights::FAtomInsightsComponent::CreateInstance();

		IUnrealInsightsModule& UnrealInsightsModule = FModuleManager::LoadModuleChecked<IUnrealInsightsModule>("TraceInsights");
		UnrealInsightsModule.RegisterComponent(AtomInsightsComponent);
#endif // !WITH_EDITOR

		FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([this]
		{
			LLM_SCOPE_BYNAME(TEXT("Insights/AtomInsights"));
			IUnrealInsightsModule& UnrealInsightsModule = FModuleManager::LoadModuleChecked<IUnrealInsightsModule>("TraceInsights");
			if (!UnrealInsightsModule.GetStoreClient())
			{
				UE_LOG(LogCore, Display, TEXT("AtomInsights module auto-connecting to local trace server..."));
				UnrealInsightsModule.ConnectToStore(TEXT("127.0.0.1"));
				UnrealInsightsModule.CreateSessionViewer(false);
			}
		});
	}
}

void FCriWareAtomInsightsModule::ShutdownModule()
{
	if (!IsRunningCommandlet())
	{
#if !WITH_EDITOR
		IUnrealInsightsModule& UnrealInsightsModule = FModuleManager::LoadModuleChecked<IUnrealInsightsModule>("TraceInsights");
		UnrealInsightsModule.UnregisterComponent(AtomInsightsComponent);

		AtomInsightsComponent.Reset();

		IModularFeatures::Get().UnregisterModularFeature(TraceServices::ModuleFeatureName, &AtomInsightsTimingViewExtender);
#endif // !WITH_EDITOR

		Atom::Insights::FDashboardAssetCommands::Unregister();

		DashboardFactory.Reset();

		IModularFeatures::Get().UnregisterModularFeature(TraceServices::ModuleFeatureName, TraceModule.Get());
		//IModularFeatures::Get().UnregisterModularFeature(IRewindDebuggerRuntimeExtension::ModularFeatureName, RewindDebuggerExtension.Get());
	}
}

void FCriWareAtomInsightsModule::RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory)
{
	DashboardFactory->RegisterViewFactory(InDashboardFactory);
}

void FCriWareAtomInsightsModule::UnregisterDashboardViewFactory(FName InName)
{
	DashboardFactory->UnregisterViewFactory(InName);
}

FAtomRuntimeId FCriWareAtomInsightsModule::GetRuntimeID() const
{
	return DashboardFactory->GetRuntimeID();
}

FCriWareAtomInsightsModule& FCriWareAtomInsightsModule::GetChecked()
{
	return static_cast<FCriWareAtomInsightsModule&>(FModuleManager::GetModuleChecked<FCriWareAtomInsightsModule>(GetName()));
}

FCriWareAtomInsightsModule* FCriWareAtomInsightsModule::GetModulePtr()
{
	if (ICriWareAtomInsightsModule* AtomInsightsModulePtr = FModuleManager::LoadModulePtr<ICriWareAtomInsightsModule>("CriWareAtomInsights"))
	{
		return static_cast<FCriWareAtomInsightsModule*>(AtomInsightsModulePtr);
	}

	return nullptr;
}

TSharedRef<Atom::Insights::FDashboardFactory> FCriWareAtomInsightsModule::GetDashboardFactory()
{
	return DashboardFactory->AsShared();
}

const TSharedRef<Atom::Insights::FDashboardFactory> FCriWareAtomInsightsModule::GetDashboardFactory() const
{
	return DashboardFactory->AsShared();
}

IAtomInsightsTraceModule& FCriWareAtomInsightsModule::GetTraceModule()
{
	return *TraceModule;
}

TSharedRef<SDockTab> FCriWareAtomInsightsModule::CreateDashboardTabWidget(const FSpawnTabArgs& Args)
{
	return DashboardFactory->MakeDockTabWidget(Args);
}

#undef LOCTEXT_NAMESPACE // CriWareAtomInsights

IMPLEMENT_MODULE(FCriWareAtomInsightsModule, CriWareAtomInsights)
