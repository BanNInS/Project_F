
#include "AtomInsightsComponent.h"

#include "Modules/ModuleManager.h"
#include "Trace/StoreClient.h"
#include "TraceServices/Model/Diagnostics.h"
#include "Widgets/Docking/SDockTab.h"

#include "CriWareAtomInsightsModule.h"
#include "AtomInsightsStyle.h"

#define LOCTEXT_NAMESPACE "AtomInsightsComponent"

namespace Atom::Insights
{
	namespace ComponentPrivate
	{
		static const FName TabName = "ADX Atom Insights";
	}

	FAtomInsightsComponent::~FAtomInsightsComponent()
	{
		ensure(!bIsInitialized);
	}

	TSharedPtr<FAtomInsightsComponent> FAtomInsightsComponent::CreateInstance()
	{
		ensure(!Instance.IsValid());

		if (Instance.IsValid())
		{
			Instance.Reset();
		}

		Instance = MakeShared<FAtomInsightsComponent>();

		return Instance;
	}

	void FAtomInsightsComponent::Initialize(IUnrealInsightsModule& InsightsModule)
	{
		ensure(!bIsInitialized);
	
		if (!bIsInitialized)
		{
			bIsInitialized = true;

			OnTick = FTickerDelegate::CreateSP(this, &FAtomInsightsComponent::Tick);

			constexpr float TickDelay = 0.5f; // 500 ms. delay between ticks
			OnTickHandle = FTSTicker::GetCoreTicker().AddTicker(OnTick, TickDelay);
		}
	}

	void FAtomInsightsComponent::Shutdown()
	{
		if (!bIsInitialized)
		{
			return;
		}
	
		bIsInitialized = false;

		FTSTicker::GetCoreTicker().RemoveTicker(OnTickHandle);

		Instance.Reset();
	}

	void FAtomInsightsComponent::RegisterMajorTabs(IUnrealInsightsModule& InsightsModule)
	{
		using namespace ComponentPrivate;

		const FInsightsMajorTabConfig& Config = InsightsModule.FindMajorTabConfig(TabName);

		if (Config.bIsAvailable)
		{
			// Register tab spawner for the Audio Insights.
			FTabSpawnerEntry& TabSpawnerEntry = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,
				FOnSpawnTab::CreateRaw(this,  &FAtomInsightsComponent::SpawnTab), 
				FCanSpawnTab::CreateRaw(this, &FAtomInsightsComponent::CanSpawnTab))
				.SetDisplayName(Config.TabLabel.IsSet()   ? Config.TabLabel.GetValue()   : LOCTEXT("AtomInsights_TabTitle", "ADX Atom Insights"))
				.SetTooltipText(Config.TabTooltip.IsSet() ? Config.TabTooltip.GetValue() : LOCTEXT("AtomInsights_TooltipText", "Open the ADX Atom Insights tab (Only available for standalone game traces)."))
				.SetIcon(Config.TabIcon.IsSet() ? Config.TabIcon.GetValue() : FSlateStyle::Get().CreateIcon("AtomInsights.Icon.Submix"));

			const TSharedRef<FWorkspaceItem>* FoundWorkspace = FGlobalTabmanager::Get()->GetLocalWorkspaceMenuRoot()->GetChildItems().FindByPredicate(
				[](const TSharedRef<FWorkspaceItem>& WorkspaceItem)
				{
					return WorkspaceItem->GetDisplayName().ToString() == "Insights Tools";
				});

			if (FoundWorkspace)
			{
				TabSpawnerEntry.SetGroup(*FoundWorkspace);
			}
		}
	}

	void FAtomInsightsComponent::UnregisterMajorTabs()
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ComponentPrivate::TabName);
	}

	bool FAtomInsightsComponent::CanSpawnTab(const FSpawnTabArgs& Args) const
	{
		return bCanSpawnTab;
	}

	TSharedRef<SDockTab> FAtomInsightsComponent::SpawnTab(const FSpawnTabArgs& Args)
	{
		const TSharedRef<SDockTab> DockTab = FCriWareAtomInsightsModule::GetChecked().CreateDashboardTabWidget(Args);

		OnTabSpawn.Broadcast();

		return DockTab;
	}

	bool FAtomInsightsComponent::GetIsLiveSession() const
	{
		IUnrealInsightsModule& UnrealInsightsModule = FModuleManager::LoadModuleChecked<IUnrealInsightsModule>("TraceInsights");

		TSharedPtr<const TraceServices::IAnalysisSession> Session = UnrealInsightsModule.GetAnalysisSession();
		if (Session.IsValid())
		{
			TraceServices::FAnalysisSessionReadScope SessionReadScope(*Session.Get());

			const UE::Trace::FStoreClient* StoreClient = UnrealInsightsModule.GetStoreClient();
			const UE::Trace::FStoreClient::FSessionInfo* SessionInfo = StoreClient ? StoreClient->GetSessionInfoByTraceId(Session->GetTraceId()) : nullptr;

			return !Session->IsAnalysisComplete() && StoreClient != nullptr && SessionInfo != nullptr;
		}

		return false;
	}

	bool FAtomInsightsComponent::Tick(float DeltaTime)
	{
		if (!bCanSpawnTab)
		{
			const IUnrealInsightsModule& UnrealInsightsModule = FModuleManager::LoadModuleChecked<IUnrealInsightsModule>("TraceInsights");

			TSharedPtr<const TraceServices::IAnalysisSession> Session = UnrealInsightsModule.GetAnalysisSession();
			if (Session.IsValid())
			{
				TraceServices::FAnalysisSessionReadScope SessionReadScope(*Session.Get());

				const TraceServices::IDiagnosticsProvider* DiagnosticsProvider = TraceServices::ReadDiagnosticsProvider(*Session.Get());
				if (DiagnosticsProvider && DiagnosticsProvider->IsSessionInfoAvailable())
				{
					const TraceServices::FSessionInfo& TraceServicesSessionInfo = DiagnosticsProvider->GetSessionInfo();

					bIsEditorTrace = TraceServicesSessionInfo.TargetType == EBuildTargetType::Editor;
				}

				// Allow to spawn tab if non-editor trace
				if (!bIsEditorTrace)
				{
					bCanSpawnTab = true;
				}
			}
		}

		return true;
	}
} // namespace

#undef LOCTEXT_NAMESPACE
