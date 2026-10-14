
#include "AtomInsightsDashboardFactory.h"

#include "Editor.h"
#include "Engine/World.h"
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

#include "CriWareAtomInsightsModule.h"
#include "AtomInsightsStyle.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	namespace DashboardFactoryPrivate
	{
		static const FText ToolName = LOCTEXT("AtomDashboard_ToolName", "ADX Atom Insights");

		static const FText EnableTracesButtonText = LOCTEXT("AtomDashboard_AutomaticallyEnableTracesTitle", "Enable Atom traces");
		static const FText EnableTracesDescription = LOCTEXT("AtomDashboard_AutomaticallyEnableTracesDescription", "Enables the Atom and Atom mixer trace channels. Atom Insights will not function without these channels enabled.");
		static const FText NoTracesEnabledWarning = LOCTEXT("AtomDashboard_NoTracesEnabledWarning", "Atom Insights requires the Atom and Atom mixer trace channels to be enabled to function.");
		static const FText EnableThemNowText = LOCTEXT("AtomDashboard_EnableNowText", "Enable them now?");
		static const FText TraceControllerUnavailabledWarning = LOCTEXT("AtomDashboard_TraceControllerUnavailableWarning", "The Trace Controller API is currently unavailable.");
		static const FText TryEnablingMessagingText = LOCTEXT("AtomDashboard_TryEnablingMessagingText", "Make sure you have launched this package with the -Messaging command line argument.");
	} // namespace

	FAtomRuntimeId FDashboardFactory::GetRuntimeID() const
	{
		return ActiveRuntimeID;
	}

	TSharedRef<SDockTab> FDashboardFactory::MakeDockTabWidget(const FSpawnTabArgs& Args)
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Label(DashboardFactoryPrivate::ToolName)
			.Clipping(EWidgetClipping::ClipToBounds)
			.TabRole(ETabRole::NomadTab);

		DashboardTabManager = FGlobalTabmanager::Get()->NewTabManager(DockTab);

		TabLayout = GetDefaultTabLayout();

		RegisterTabSpawners();

		const TSharedRef<SWidget> TabContent = 
			
#if !WITH_EDITOR
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
#endif	// !WITH_EDITOR
			
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeMenuBarWidget()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(4.0f)
				]
				+ SVerticalBox::Slot()
				[
					DashboardTabManager->RestoreFrom(TabLayout->AsShared(), Args.GetOwnerWindow()).ToSharedRef()
#if WITH_EDITOR
				];
#else
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				MakeEnableTracesOverlay()
			];
#endif	// WITH_EDITOR

		DockTab->SetContent(TabContent);

		DockTab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([this](TSharedRef<SDockTab> TabClosed)
		{
			UnregisterTabSpawners();
		}));

		return DockTab;
	}

	TSharedRef<SWidget> FDashboardFactory::MakeMenuBarWidget()
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
			}),
			"View"
		);

		return MenuBarBuilder.MakeWidget();
	}

	TSharedPtr<FTabManager::FLayout> FDashboardFactory::GetDefaultTabLayout()
	{
		using namespace DashboardFactoryPrivate;

		TSharedRef<FTabManager::FStack> LogTabStack = FTabManager::NewStack();
		TSharedRef<FTabManager::FStack> AnalysisTabStack = FTabManager::NewStack();

		for (const auto& [FactoryName, Factory] : DashboardViewFactories)
		{
			const EDefaultDashboardTabStack DefaultTabStack = Factory->GetDefaultTabStack();

			switch (DefaultTabStack)
			{
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

				default:
					break;
			}
		}

		AnalysisTabStack->SetForegroundTab(FName("MixerSources"));

		return FTabManager::NewLayout("AtomDashboard_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					LogTabStack
					->SetSizeCoefficient(0.25f)
				)
				->Split
				(
					AnalysisTabStack
					->SetSizeCoefficient(0.75f)
				)
			)
		);
	}

	void FDashboardFactory::RegisterTabSpawners()
	{
		using namespace DashboardFactoryPrivate;

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

	void FDashboardFactory::RegisterViewFactory(TSharedRef<IDashboardViewFactory> InFactory)
	{
		if (const FName Name = InFactory->GetName(); 
			ensureAlwaysMsgf(!DashboardViewFactories.Contains(Name), TEXT("Failed to register Atom Insights Dashboard '%s': Dashboard with name already registered"), *Name.ToString()))
		{
			DashboardViewFactories.Add(Name, InFactory);
		}
	}

	void FDashboardFactory::UnregisterTabSpawners()
	{
		if (DashboardTabManager.IsValid())
		{
			for (const auto& KVP : DashboardViewFactories)
			{
				const FName& FactoryName = KVP.Key;
				DashboardTabManager->UnregisterTabSpawner(FactoryName);
			}

			DashboardTabManager.Reset();
		}

		DashboardWorkspace.Reset();
	}

	void FDashboardFactory::UnregisterViewFactory(FName InName)
	{
		DashboardViewFactories.Remove(InName);
	}

	#if !WITH_EDITOR
	TSharedRef<SWidget> FDashboardFactory::MakeEnableTracesOverlay()
	{
		using namespace DashboardFactoryPrivate;

		return SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.8f))
			.Visibility_Lambda([]() -> EVisibility
			{
				FCriWareAtomInsightsModule* AtomInsightsModulePtr = FCriWareAtomInsightsModule::GetModulePtr();
				if (AtomInsightsModulePtr == nullptr)
				{
					return EVisibility::Hidden;
				}

				IAtomInsightsTraceModule& TraceModule = AtomInsightsModulePtr->GetTraceModule();
				return TraceModule.AtomChannelsCanBeManuallyEnabled() ? EVisibility::Visible : EVisibility::Hidden;
			})
			[
				SNew(SVerticalBox)
					
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNullWidget::NullWidget
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([]()
					{
						FCriWareAtomInsightsModule* AtomInsightsModulePtr = FCriWareAtomInsightsModule::GetModulePtr();
						if (AtomInsightsModulePtr == nullptr)
						{
							return FText::GetEmpty();
						}

						IAtomInsightsTraceModule& TraceModule = AtomInsightsModulePtr->GetTraceModule();
						return TraceModule.TraceControllerIsAvailable() ? NoTracesEnabledWarning : TraceControllerUnavailabledWarning;
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([]()
					{
						FCriWareAtomInsightsModule* AtomInsightsModulePtr = FCriWareAtomInsightsModule::GetModulePtr();
						if (AtomInsightsModulePtr == nullptr)
						{
							return FText::GetEmpty();
						}

						IAtomInsightsTraceModule& TraceModule = AtomInsightsModulePtr->GetTraceModule();
						return TraceModule.TraceControllerIsAvailable() ? EnableThemNowText : TryEnablingMessagingText;
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 10.0f)
				[
					MakeEnableTracesButton()
				]

				+ SVerticalBox::Slot()
				.VAlign(VAlign_Bottom)
				[
					SNullWidget::NullWidget
				]
			];
	}

	TSharedRef<SWidget> FDashboardFactory::MakeEnableTracesButton()
	{
		using namespace DashboardFactoryPrivate;

		return SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
			.OnClicked(this, &FDashboardFactory::ToggleAutoEnableAudioTraces)
			.ToolTipText(EnableTracesDescription)
			.ContentPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
			.Visibility_Lambda([]() -> EVisibility
			{
				FCriWareAtomInsightsModule* AtomInsightsModulePtr = FCriWareAtomInsightsModule::GetModulePtr();
				if (AtomInsightsModulePtr == nullptr)
				{
					return EVisibility::Hidden;
				}

				IAtomInsightsTraceModule& TraceModule = AtomInsightsModulePtr->GetTraceModule();

				return TraceModule.TraceControllerIsAvailable() ? EVisibility::Visible : EVisibility::Hidden;
			})
			.Content()
			[
				SNew(SBox)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
					.Justification(ETextJustify::Center)
					.Text(EnableTracesButtonText)
				]
			];
	}

	FReply FDashboardFactory::ToggleAutoEnableAtomTraces()
	{
		IAtomInsightsTraceModule& TraceModule = FCriWareAtomInsightsModule::GetChecked().GetTraceModule();
		TraceModule.StartTraceAnalysis(false);

		return FReply::Handled();
	}
#endif // !WITH_EDITOR
} // namespace

#undef LOCTEXT_NAMESPACE
