
#include "AtomInsightsDashboardAssetCommands.h"

#include "Framework/Commands/Commands.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Misc/Attribute.h"

#include "CriWareAtomInsightsModule.h"
#include "AtomInsightsStyle.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	FDashboardAssetCommands::FDashboardAssetCommands()
		: TCommands<FDashboardAssetCommands>("AtomInsightsDashboardAssetCommands", LOCTEXT("AtomInsightsDashboardAssetCommands", "Dashboard Asset Commands"), NAME_None, FSlateStyle::GetStyleName())
	{
	}

	void FDashboardAssetCommands::RegisterCommands()
	{
		UI_COMMAND(BrowserSync, "Browse", "Browses to the selected asset in the content browser.", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(Open, "Open", "Opens the selected asset(s) in respective editor(s).", EUserInterfaceActionType::Button, FInputChord());

		UI_COMMAND(Start, "Start Trace", "Starts the active trace session used by Audio Insights.", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(Stop, "Stop Trace", "Stops the active trace session used by Audio Insights.", EUserInterfaceActionType::Button, FInputChord());
	}

	void FDashboardAssetCommands::AddAssetCommands(FToolBarBuilder& OutToolbarBuilder) const
	{
		OutToolbarBuilder.AddToolBarButton(
			BrowserSync,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			TAttribute<FSlateIcon>::Create([]() { return FSlateStyle::Get().CreateIcon("AtomInsights.Icon.ContentBrowser"); }),
			"BrowserSync"
		);

		OutToolbarBuilder.AddToolBarButton(
			Open,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			TAttribute<FSlateIcon>::Create([]() { return FSlateStyle::Get().CreateIcon("AtomInsights.Icon.Open"); }),
			"Open"
		);
	}

	TSharedPtr<const FUICommandInfo> FDashboardAssetCommands::GetBrowserSyncCommand() const
	{
		return BrowserSync;
	};

	TSharedPtr<const FUICommandInfo> FDashboardAssetCommands::GetOpenCommand() const
	{
		return Open;
	}

	TSharedPtr<const FUICommandInfo> FDashboardAssetCommands::GetStartCommand() const
	{
		return Start;
	}

	TSharedPtr<const FUICommandInfo> FDashboardAssetCommands::GetStopCommand() const
	{
		return Stop;
	}

	FSlateIcon FDashboardAssetCommands::GetStartIcon() const
	{
		const FTraceModule& TraceModule = static_cast<FTraceModule&>(FCriWareAtomInsightsModule::GetChecked().GetTraceModule());
		return FSlateStyle::Get().CreateIcon(TraceModule.IsTraceAnalysisActive()
			? "AtomInsights.Icon.Start.Inactive"
			: "AtomInsights.Icon.Start.Active"
		);
	}

	FSlateIcon FDashboardAssetCommands::GetStopIcon() const
	{
		const FTraceModule& TraceModule = static_cast<FTraceModule&>(FCriWareAtomInsightsModule::GetChecked().GetTraceModule());
		return FSlateStyle::Get().CreateIcon(TraceModule.IsTraceAnalysisActive()
			? "AtomInsights.Icon.Stop.Active"
			: "AtomInsights.Icon.Stop.Inactive"
		);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
