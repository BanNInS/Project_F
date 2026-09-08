
#include "AtomInsightsEditorDashboardFactory.h"

#include "Async/Async.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Framework/Docking/LayoutService.h"
#include "Framework/Docking/TabManager.h"
#include "Internationalization/Text.h"
#include "IPropertyTypeCustomization.h"
#include "Kismet2/DebuggerCommands.h"
#include "Templates/SharedPointer.h"
#include "ToolMenu.h"
#include "ToolMenus.h"
#include "Widgets/Input/SCheckBox.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "CriWareAtomInsightsEditorModule.h"
#include "CriWareAtomInsightsEditorSettings.h"
#include "AtomInsightsStyle.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	namespace EditorDashboardFactoryPrivate
	{
		static const FText ToolName = LOCTEXT("AtomDashboard_ToolName", "ADX Atom Insights");

		static const FName MainToolbarName = "MainToolbar";
		static const FText MainToolbarDisplayName = LOCTEXT("AtomDashboard_MainToolbarDisplayName", "Dashboard Transport");

		static const FText PreviewRuntimeDisplayName = LOCTEXT("AtomDashboard_PreviewRuntime", "[Preview Atom Audio]");
		static const FText DashboardWorldSelectDescription = LOCTEXT("AtomDashboard_SelectWorldDescription", "Select world(s) to monitor (worlds may share audio output).");

		static const FText OnlyTraceAtomChannelsName = LOCTEXT("AtomDashboard_OnlyTraceAtomChannelsDisplayName", "Only trace Atom channels during PIE:");
		static const FText OnlyTraceAtomChannelsDescription = LOCTEXT("AtomDashboard_OnlyTraceAtomChannelsDescription", "Disable all non-command line trace channels apart from Atom, Atom Mixer and CPU during PIE. This will reduce the file sizes of trace sessions while using ADX Atom Insights.");

		FText GetDebugNameFromRuntimeID(FAtomRuntimeId InRuntimeID)
		{
			FString WorldName;
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				TArray<UWorld*> RuntimeWorlds = RuntimeManager->GetWorldsUsingAtomRuntime(InRuntimeID);
				for (const UWorld* World : RuntimeWorlds)
				{
					if (!WorldName.IsEmpty())
					{
						WorldName += TEXT(", ");
					}
					WorldName += World->GetDebugDisplayName();
				}
			}

			if (WorldName.IsEmpty())
			{
				return PreviewRuntimeDisplayName;
			}

			return FText::FromString(WorldName);
		}
	} // namespace

	void FEditorDashboardFactory::OnWorldRegisteredToAtomRuntime(const UWorld* InWorld, FAtomRuntimeId InRuntimeID)
	{
		if (InRuntimeID != INDEX_NONE)
		{
			StartTraceAnalysis(InWorld, InRuntimeID);
		}

		RefreshRuntimeSelector();
	}

	void FEditorDashboardFactory::OnWorldUnregisteredFromAtomRuntime(const UWorld* InWorld, ::Audio::FDeviceId InDeviceId)
	{
		RefreshRuntimeSelector();
	}

	void FEditorDashboardFactory::OnPIEStarted(bool bSimulating)
	{
		IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();
		TraceModule.StartTraceAnalysis(bOnlyTraceAtomChannels);
	}

	void FEditorDashboardFactory::OnPIEStopped(bool bSimulating)
	{
		IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();
		TraceModule.StopTraceAnalysis();

		RefreshRuntimeSelector();
	}


	void FEditorDashboardFactory::OnRuntimeCreated(FAtomRuntimeId InRuntimeID)
	{
		OnActiveAtomRuntimeChanged.Broadcast();
	}

	void FEditorDashboardFactory::OnRuntimeDestroyed(FAtomRuntimeId InRuntimeID)
	{
		if (ActiveRuntimeID == InRuntimeID)
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				ActiveRuntimeID = RuntimeManager->GetMainAtomRuntimeID();
			}
		}

		AtomRuntimeIDs.RemoveAll([InRuntimeID](const TSharedPtr<FAtomRuntimeId>& RuntimeIDPtr)
		{
			return *RuntimeIDPtr.Get() == InRuntimeID;
		});

		if (AtomRuntimeComboBox.IsValid())
		{
			AtomRuntimeComboBox->RefreshOptions();
		}

		OnActiveAtomRuntimeChanged.Broadcast();
	}

	void FEditorDashboardFactory::RefreshRuntimeSelector()
	{
		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			if (!RuntimeManager->IsValidAtomRuntime(ActiveRuntimeID))
			{
				ActiveRuntimeID = RuntimeManager->GetMainAtomRuntimeID();
			}
		}

		AtomRuntimeIDs.Empty();
		if (const FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			RuntimeManager->IterateOverAllRuntimes([this, &RuntimeManager](FAtomRuntimeId RuntimeID, const FAtomRuntime* AtomRuntime)
			{
				AtomRuntimeIDs.Add(MakeShared<FAtomRuntimeId>(RuntimeID));
			});
		}

		if (AtomRuntimeComboBox.IsValid())
		{
			AtomRuntimeComboBox->RefreshOptions();
		}
	}

	void FEditorDashboardFactory::ResetDelegates()
	{
		if (OnWorldRegisteredToAtomRuntimeHandle.IsValid())
		{
			FAtomRuntimeWorldDelegates::OnWorldRegisteredToAtomRuntime.Remove(OnWorldRegisteredToAtomRuntimeHandle);
			OnWorldRegisteredToAtomRuntimeHandle.Reset();
		}

		if (OnWorldUnregisteredFromAtomRuntimeHandle.IsValid())
		{
			FAtomRuntimeWorldDelegates::OnWorldUnregisteredWithAtomRuntime.Remove(OnWorldUnregisteredFromAtomRuntimeHandle);
			OnWorldUnregisteredFromAtomRuntimeHandle.Reset();
		}

		if (OnRuntimeCreatedHandle.IsValid())
		{
			FAtomRuntimeManagerDelegates::OnAtomRuntimeCreated.Remove(OnRuntimeCreatedHandle);
			OnRuntimeCreatedHandle.Reset();
		}

		if (OnRuntimeDestroyedHandle.IsValid())
		{
			FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Remove(OnRuntimeDestroyedHandle);
			OnRuntimeDestroyedHandle.Reset();
		}

		if (OnPIEStartedHandle.IsValid())
		{
			FEditorDelegates::PreBeginPIE.Remove(OnPIEStartedHandle);
			OnPIEStartedHandle.Reset();
		}

		if (OnPIEStoppedHandle.IsValid())
		{
			FEditorDelegates::EndPIE.Remove(OnPIEStoppedHandle);
			OnPIEStoppedHandle.Reset();
		}
	}

	FAtomRuntimeId FEditorDashboardFactory::GetRuntimeID() const
	{
		return ActiveRuntimeID;
	}

	TSharedRef<SDockTab> FEditorDashboardFactory::MakeDockTabWidget(const FSpawnTabArgs& Args)
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Label(EditorDashboardFactoryPrivate::ToolName)
			.Clipping(EWidgetClipping::ClipToBounds)
			.TabRole(ETabRole::NomadTab);

		DashboardTabManager = FGlobalTabmanager::Get()->NewTabManager(DockTab);

		DashboardTabManager->SetOnPersistLayout(FTabManager::FOnPersistLayout::CreateStatic([](const TSharedRef<FTabManager::FLayout>& InLayout)
		{
			if (InLayout->GetPrimaryArea().Pin().IsValid())
			{
				FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, InLayout);
			}
		}));

		InitDelegates();

		RegisterTabSpawners();
		RefreshRuntimeSelector();

		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
			RuntimeManager&& GEditor && GEditor->PlayWorld)
		{
			StartTraceAnalysis(GEditor->PlayWorld, RuntimeManager->GetActiveAtomRuntime()->GetAtomRuntimeID());
		}

		const TSharedRef<FTabManager::FLayout> TabLayout = LoadLayoutFromConfig();

		const TSharedRef<SWidget> TabContent = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeMenuBarWidget()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakeMainToolbarWidget()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(4.0f)
			]
			+ SVerticalBox::Slot()
			[
				DashboardTabManager->RestoreFrom(TabLayout, Args.GetOwnerWindow()).ToSharedRef()
			];

		DockTab->SetContent(TabContent);

		DockTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([this](TSharedRef<SDockTab> TabClosed)
		{
			// If we are still in PIE, make sure we stop tracing for Audio Insights
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
				RuntimeManager && GEditor && GEditor->PlayWorld)
			{
				IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();
				TraceModule.StopTraceAnalysis();
			}

			ResetDelegates();
			UnregisterTabSpawners();
			SaveLayoutToConfig();

			for (const auto& KVP : DashboardViewFactories)
			{
				if (TSharedPtr<SDockTab> DashboardTab = DashboardTabManager->FindExistingLiveTab(KVP.Key))
				{
					// Explicitly close each dashboard tab. This will give a chance for it to close any undocked sub-managed tabs of its own:
					DashboardTab->RequestCloseTab();
				}
			}

			DashboardTabManager->CloseAllAreas();

			DashboardTabManager.Reset();
			DashboardWorkspace.Reset();
		}));

		return DockTab;
	}

	TSharedRef<SWidget> FEditorDashboardFactory::MakeMenuBarWidget()
	{
		FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());

		MenuBarBuilder.AddPullDownMenu(
			LOCTEXT("File_MenuLabel", "File"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(LOCTEXT("Close_MenuLabel", "Close"),
					LOCTEXT("Close_MenuLabel_Tooltip", "Closes the Audio Insights dashboard."),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this]()
					{
						if (DashboardTabManager.IsValid())
						{
							if (TSharedPtr<SDockTab> OwnerTab = DashboardTabManager->GetOwnerTab())
							{
								OwnerTab->RequestCloseTab();
							}
						}
					}))
				);
			}),
			"File"
		);

		MenuBarBuilder.AddPullDownMenu(
			LOCTEXT("ViewMenuLabel", "View"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
			{
				for (const auto& KVP : DashboardViewFactories)
				{
					const FName& FactoryName = KVP.Key;
					const TSharedPtr<IDashboardViewFactory>& Factory = KVP.Value;

					MenuBuilder.AddMenuEntry(Factory->GetDisplayName(),
						FText::GetEmpty(),
						FSlateStyle::Get().CreateIcon(Factory->GetIcon().GetStyleName()),
						FUIAction(FExecuteAction::CreateLambda([this, FactoryName]()
						{
							if (DashboardTabManager.IsValid())
							{
								if (TSharedPtr<SDockTab> ViewportTab = DashboardTabManager->FindExistingLiveTab(FactoryName);
									!ViewportTab.IsValid())
								{
									DashboardTabManager->TryInvokeTab(FactoryName);

									if (TSharedPtr<SDockTab> InvokedOutputMeterTab = DashboardTabManager->TryInvokeTab(FactoryName);
										InvokedOutputMeterTab.IsValid() && DashboardViewFactories[FactoryName].IsValid())
									{
										if (const EDefaultDashboardTabStack DefaultTabStack = DashboardViewFactories[FactoryName]->GetDefaultTabStack();
											DefaultTabStack == EDefaultDashboardTabStack::AudioAnalyzerRack)
										{
											InvokedOutputMeterTab->SetParentDockTabStackTabWellHidden(true);
										}
									}
								}
								else
								{
									ViewportTab->RequestCloseTab();
								}
							}
						}),
						FCanExecuteAction(),
						FIsActionChecked::CreateLambda([&DashboardTabManager = DashboardTabManager, FactoryName]()
						{
							return DashboardTabManager.IsValid() ? DashboardTabManager->FindExistingLiveTab(FactoryName).IsValid() : false;
						})),
						NAME_None,
						EUserInterfaceActionType::Check
					);

					if (const EDefaultDashboardTabStack DefaultTabStack = DashboardViewFactories[FactoryName]->GetDefaultTabStack();
						DefaultTabStack == EDefaultDashboardTabStack::Log || DefaultTabStack == EDefaultDashboardTabStack::AudioMeters)
					{
						MenuBuilder.AddMenuSeparator();
					}
				}

				MenuBuilder.AddMenuSeparator();

				MenuBuilder.AddMenuEntry(LOCTEXT("ViewMenu_ResetLayoutText", "Reset Layout"), 
					FText::GetEmpty(), 
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this]()
					{
						if (DashboardTabManager)
						{
							for (const auto& KVP : DashboardViewFactories)
							{
								// Try and get the dashboard tab:
								TSharedPtr<SDockTab> DashboardTab = DashboardTabManager->FindExistingLiveTab(KVP.Key);
								if (!DashboardTab.IsValid())
								{
									DashboardTab = DashboardTabManager->TryInvokeTab(KVP.Key);
								}

								if (DashboardTab.IsValid())
								{
									if (TSharedPtr<FTabManager> SubTabManager = FGlobalTabmanager::Get()->GetTabManagerForMajorTab(DashboardTab))
									{
										// There is a sub tab manager for this dashbaord tab; clear its persisted areas:
										SubTabManager->CloseAllAreas();
										SubTabManager->SavePersistentLayout();
									}
								}
							}

							if (TSharedPtr<SDockTab> OwnerTab = DashboardTabManager->GetOwnerTab())
							{
								// Wipe all the persisted areas and close tab
								DashboardTabManager->CloseAllAreas();
								OwnerTab->RequestCloseTab();

								// Can't invoke the tab immediately (it won't show up), needs to be done a bit later
								AsyncTask(ENamedThreads::GameThread, [AtomInsightsTabID = OwnerTab->GetLayoutIdentifier()]()
								{
									FGlobalTabmanager::Get()->TryInvokeTab(AtomInsightsTabID);
								});
							}
						}
					}),
					FCanExecuteAction()));
			}),
			"View"
		);

		return MenuBarBuilder.MakeWidget();
	}

	TSharedRef<SWidget> FEditorDashboardFactory::MakeMainToolbarWidget()
	{
		using namespace EditorDashboardFactoryPrivate;

		static const FName PlayWorldToolBarName = "Kismet.DebuggingViewToolBar";
		if (!UToolMenus::Get()->IsMenuRegistered(PlayWorldToolBarName))
		{
			UToolMenu* ToolBar = UToolMenus::Get()->RegisterMenu(PlayWorldToolBarName, NAME_None, EMultiBoxType::SlimHorizontalToolBar);
			FToolMenuSection& Section = ToolBar->AddSection("Debug");
			FPlayWorldCommands::BuildToolbar(Section);
		}

		static FSlateBrush TransportBackgroundColorBrush;
		TransportBackgroundColorBrush.TintColor = FSlateColor(FLinearColor(0.018f, 0.018f, 0.018f, 1.0f));
		TransportBackgroundColorBrush.DrawAs    = ESlateBrushDrawType::Box;

		return SNew(SBorder)
			.BorderImage(&TransportBackgroundColorBrush)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("NoBorder"))
					[
						UToolMenus::Get()->GenerateWidget(PlayWorldToolBarName, { FPlayWorldCommands::GlobalPlayWorldActions })
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(OnlyTraceAtomChannelsName)
					.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				[
					SNew(SCheckBox)
					.ToolTipText(OnlyTraceAtomChannelsDescription)
					.IsChecked_Lambda([this]() { return bOnlyTraceAtomChannels ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						bOnlyTraceAtomChannels = NewState == ECheckBoxState::Checked;

						IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();
						TraceModule.OnOnlyTraceAtomChannelsStateChanged(bOnlyTraceAtomChannels);
					})
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SelectDashboardWorld_DisplayName", "World Filter:"))
					.ToolTipText(DashboardWorldSelectDescription)
					.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				[
					SAssignNew(AtomRuntimeComboBox, SComboBox<TSharedPtr<FAtomRuntimeId>>)
					.ToolTipText(DashboardWorldSelectDescription)
					.OptionsSource(&AtomRuntimeIDs)
					.OnGenerateWidget_Lambda([](const TSharedPtr<FAtomRuntimeId>& WidgetRuntimeID)
					{
						FText NameText = GetDebugNameFromRuntimeID(*WidgetRuntimeID);
						return SNew(STextBlock)
							.Text(NameText)
							.Font(IPropertyTypeCustomizationUtils::GetRegularFont());
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FAtomRuntimeId> NewRuntimeID, ESelectInfo::Type)
					{
						if (NewRuntimeID.IsValid())
						{
							ActiveRuntimeID = *NewRuntimeID;
							RefreshRuntimeSelector();

							OnActiveAtomRuntimeChanged.Broadcast();
						}
					})
					[
						SNew(STextBlock)
						.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
						.Text_Lambda([this]()
						{
							return GetDebugNameFromRuntimeID(ActiveRuntimeID);
						})
					]
				]
			];
	}

	void FEditorDashboardFactory::StartTraceAnalysis(const TObjectPtr<const UWorld> InWorld, const FAtomRuntimeId InRuntimeID)
	{
		if (InWorld && InWorld->IsGameWorld())
		{
			IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();
			TraceModule.StartTraceAnalysis(bOnlyTraceAtomChannels);

			const TObjectPtr<const UCriWareAtomInsightsEditorSettings> AtomInsightsEditorSettings = GetDefault<UCriWareAtomInsightsEditorSettings>();

			// We don't want to set ActiveDeviceId if bWorldFilterDefaultsToFirstClient is true and more than 2 PIE clients are running
			if (AtomInsightsEditorSettings == nullptr ||
				(AtomInsightsEditorSettings && !AtomInsightsEditorSettings->bWorldFilterDefaultsToFirstClient) ||
				AtomRuntimeIDs.Num() < 2)
			{
				ActiveRuntimeID = InRuntimeID;
			}
		}
	}

	void FEditorDashboardFactory::InitDelegates()
	{
		if (!OnWorldRegisteredToAtomRuntimeHandle.IsValid())
		{
			OnWorldRegisteredToAtomRuntimeHandle = FAtomRuntimeWorldDelegates::OnWorldRegisteredToAtomRuntime.AddSP(this, &FEditorDashboardFactory::OnWorldRegisteredToAtomRuntime);
		}

		if (!OnWorldUnregisteredFromAtomRuntimeHandle.IsValid())
		{
			OnWorldUnregisteredFromAtomRuntimeHandle = FAtomRuntimeWorldDelegates::OnWorldUnregisteredWithAtomRuntime.AddSP(this, &FEditorDashboardFactory::OnWorldUnregisteredFromAtomRuntime);
		}

		if (!OnRuntimeCreatedHandle.IsValid())
		{
			OnRuntimeCreatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeCreated.AddSP(this, &FEditorDashboardFactory::OnRuntimeCreated);
		}

		if (!OnRuntimeDestroyedHandle.IsValid())
		{
			OnRuntimeDestroyedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddSP(this, &FEditorDashboardFactory::OnRuntimeDestroyed);
		}

		if (!OnPIEStartedHandle.IsValid())
		{
			OnPIEStartedHandle = FEditorDelegates::PreBeginPIE.AddSP(this, &FEditorDashboardFactory::OnPIEStarted);
		}

		if (!OnPIEStoppedHandle.IsValid())
		{
			OnPIEStoppedHandle = FEditorDelegates::EndPIE.AddSP(this, &FEditorDashboardFactory::OnPIEStopped);
		}
	}

	TSharedRef<FTabManager::FLayout> FEditorDashboardFactory::GetDefaultTabLayout()
	{
		using namespace EditorDashboardFactoryPrivate;

		TSharedRef<FTabManager::FStack> ViewportTabStack = FTabManager::NewStack();
		TSharedRef<FTabManager::FStack> LogTabStack = FTabManager::NewStack();
		TSharedRef<FTabManager::FStack> AnalysisTabStack = FTabManager::NewStack();
		TSharedRef<FTabManager::FStack> AudioMetersTabStack = FTabManager::NewStack();
		TSharedRef<FTabManager::FStack> AudioAnalyzerRackTabStack = FTabManager::NewStack()->SetHideTabWell(true)->SetSizeCoefficient(0.15f);

		for (const auto& KVP : DashboardViewFactories)
		{
			const FName& FactoryName = KVP.Key;
			const TSharedPtr<IDashboardViewFactory>& Factory = KVP.Value;

			const EDefaultDashboardTabStack DefaultTabStack = Factory->GetDefaultTabStack();
			switch (DefaultTabStack)
			{
				case EDefaultDashboardTabStack::Viewport:
				{
					ViewportTabStack->AddTab(FactoryName, ETabState::OpenedTab);
				}
				break;

				case EDefaultDashboardTabStack::Log:
				{
					LogTabStack->AddTab(FactoryName, ETabState::OpenedTab);
				}
				break;

				case EDefaultDashboardTabStack::Analysis:
				{
					AnalysisTabStack->AddTab(FactoryName, ETabState::OpenedTab);
				}
				break;

				case EDefaultDashboardTabStack::AudioMeters:
				{
					AudioMetersTabStack->AddTab(FactoryName, ETabState::OpenedTab);
				}
				break;

				case EDefaultDashboardTabStack::AudioAnalyzerRack:
				{
					AudioAnalyzerRackTabStack->AddTab(FactoryName, ETabState::OpenedTab);
				}
				break;

				default:
					break;
			}
		}

		AnalysisTabStack->SetForegroundTab(FName("MixerSources"));

		return FTabManager::NewLayout("AtomDashboard_Layout_v2")
		->AddArea
		(
FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				// Left column
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					// Top
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.25f) // Column width
					->Split
					(
						ViewportTabStack
						->SetSizeCoefficient(0.5f)
					)
					// Bottom
					->Split
					(
						LogTabStack
						->SetSizeCoefficient(0.5f)
					)
				)
				
				// Middle column
				->Split
				(
					// Top
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.6f) // Column width
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->Split
						(
							AnalysisTabStack
							->SetSizeCoefficient(0.58f)
						)
					)
					// Bottom
					->Split
					(
						AudioMetersTabStack
						->SetSizeCoefficient(0.42f)
					)
				)
				// Right column
				->Split
				(
					AudioAnalyzerRackTabStack
				)
			)
		);
	}

	void FEditorDashboardFactory::RegisterTabSpawners()
	{
		using namespace EditorDashboardFactoryPrivate;

		DashboardWorkspace = DashboardTabManager->AddLocalWorkspaceMenuCategory(ToolName);

		for (const auto& KVP : DashboardViewFactories)
		{
			const FName& FactoryName = KVP.Key;
			const TSharedPtr<IDashboardViewFactory>& Factory = KVP.Value;

			DashboardTabManager->RegisterTabSpawner(FactoryName, FOnSpawnTab::CreateLambda([this, Factory](const FSpawnTabArgs& Args)
			{
				TSharedRef<SDockTab> DockTab = SNew(SDockTab)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Label(Factory->GetDisplayName());

				TSharedRef<SWidget> DashboardView = Factory->MakeWidget(DockTab, Args);
				DockTab->SetContent(DashboardView);

				return DockTab;
			}))
			.SetDisplayName(Factory->GetDisplayName())
			.SetGroup(DashboardWorkspace->AsShared())
			.SetIcon(Factory->GetIcon());
		}
	}

	void FEditorDashboardFactory::UnregisterTabSpawners()
	{
		if (DashboardTabManager.IsValid())
		{
			for (const auto& KVP : DashboardViewFactories)
			{
				const FName& FactoryName = KVP.Key;
				DashboardTabManager->UnregisterTabSpawner(FactoryName);
			}
		}
	}

	void FEditorDashboardFactory::RegisterViewFactory(TSharedRef<IDashboardViewFactory> InFactory)
	{
		if (const FName Name = InFactory->GetName();
			ensureAlwaysMsgf(!DashboardViewFactories.Contains(Name), TEXT("Failed to register Atom Insights Dashboard '%s': Dashboard with name already registered"), *Name.ToString()))
		{
			DashboardViewFactories.Add(Name, InFactory);
		}
	}

	void FEditorDashboardFactory::UnregisterViewFactory(FName InName)
	{
		DashboardViewFactories.Remove(InName);
	}

	TSharedRef<FTabManager::FLayout> FEditorDashboardFactory::LoadLayoutFromConfig()
	{
		return FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, GetDefaultTabLayout());
	}

	void FEditorDashboardFactory::SaveLayoutToConfig()
	{
		if (DashboardTabManager.IsValid())
		{
			FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, DashboardTabManager->PersistLayout());
		}
	}
} // namespace

#undef LOCTEXT_NAMESPACE
