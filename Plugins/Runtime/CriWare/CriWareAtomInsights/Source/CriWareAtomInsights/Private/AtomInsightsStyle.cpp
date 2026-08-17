
#include "AtomInsightsStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	FSlateStyle& FSlateStyle::Get()
	{
		static FSlateStyle InsightsStyle;
		return InsightsStyle;
	}

	FName FSlateStyle::GetStyleName()
	{
		const FLazyName StyleName = "AtomInsights";
		return StyleName.Resolve();
	}

	FText FSlateStyle::FormatSecondsAsTime(float InTimeSec)
	{
		return FText::Format(LOCTEXT("TimeInSecondsFormat", "{0}s"), FText::AsNumber(InTimeSec, GetTimeFormat()));
	}

	FText FSlateStyle::FormatMillisecondsAsTime(float InTimeMS)
	{
		return FText::Format(LOCTEXT("TimeInMillisecondsFormat", "{0}ms"), FText::AsNumber(InTimeMS, GetTimeFormat()));
	}

	const FSlateBrush& FSlateStyle::GetBrushEnsured(FName InName)
	{
		const ISlateStyle* AtomInsightsStyle = FSlateStyleRegistry::FindSlateStyle(GetStyleName());
		if (ensureMsgf(AtomInsightsStyle, TEXT("Missing slate style '%s'"), *GetStyleName().ToString()))
		{
			const FSlateBrush* Brush = AtomInsightsStyle->GetBrush(InName);
			if (ensureMsgf(Brush, TEXT("Missing brush '%s'"), *InName.ToString()))
			{
				return *Brush;
			}
		}

		if (const FSlateBrush* NoBrush = FAppStyle::GetBrush("NoBrush"))
		{
			return *NoBrush;
		}

		return *DefaultBrush;
	}

	FSlateStyle::FSlateStyle()
		: FSlateStyleSet(GetStyleName())
	{
		SetParentStyleName(FAppStyle::GetAppStyleSetName());

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CriWareAtomInsights"));
		check(Plugin.IsValid());

		SetContentRoot(Plugin->GetContentDir());
		SetCoreContentRoot(FPaths::EnginePluginsDir() / TEXT("Slate"));

		/* Atom Insights */
		// Colors
		Set("AtomInsights.Analyzers.BackgroundColor", FLinearColor(0.0075f, 0.0075f, 0.0075f, 1.0f));

		// Icons
		const FVector2D Icon16(16.0f, 16.0f);
		const FVector2D Icon20(20.0f, 20.0f);
		const FVector2D Icon24(24.0f, 24.0f);
		const FVector2D Icon64(64.0f, 64.0f);

		Set("AtomInsights.Icon", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_insights_icon"), Icon16));
		Set("AtomInsights.Icon.Dashboard", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_dashboard"), Icon16));
		Set("AtomInsights.Icon.Event", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_event"), Icon16));
		Set("AtomInsights.Icon.Log", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_log"), Icon16));
		Set("AtomInsights.Icon.Sources", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_sources"), Icon16));
		Set("AtomInsights.Icon.Sources.Plots", new IMAGE_BRUSH(TEXT("Icons/audio_sources_plots"), Icon24));
		Set("AtomInsights.Icon.Submix", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_submix"), Icon16));
		Set("AtomInsights.Icon.VirtualLoop", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_virtualloop"), Icon16));
		Set("AtomInsights.Icon.Viewport", new IMAGE_BRUSH_SVG(TEXT("Icons/viewport"), Icon16));
		Set("AtomInsights.Icon.Open", new IMAGE_BRUSH_SVG(TEXT("Icons/open"), Icon20));
		Set("AtomInsights.Icon.ContentBrowser", new IMAGE_BRUSH_SVG(TEXT("Icons/content_browser"), Icon20));
		Set("AtomInsights.Icon.Start.Active", new IMAGE_BRUSH_SVG(TEXT("Icons/start_active"), Icon20));
		Set("AtomInsights.Icon.Start.Inactive", new IMAGE_BRUSH_SVG(TEXT("Icons/start_inactive"), Icon20));
		Set("AtomInsights.Icon.Record.Active", new IMAGE_BRUSH_SVG(TEXT("Icons/record_active"), Icon20));
		Set("AtomInsights.Icon.Record.Inactive", new IMAGE_BRUSH_SVG(TEXT("Icons/record_inactive"), Icon20));
		Set("AtomInsights.Icon.Stop.Active", new IMAGE_BRUSH_SVG(TEXT("Icons/stop_active"), Icon20));
		Set("AtomInsights.Icon.Stop.Inactive", new IMAGE_BRUSH_SVG(TEXT("Icons/stop_inactive"), Icon20));

		Set("AtomInsights.Thumbnail", new IMAGE_BRUSH_SVG(TEXT("Icons/audio_insights"), Icon16));

		/* Tree Dashboard */
		// Table style
		Set("TreeDashboard.TableViewRow", FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
			.SetOddRowBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed))
			.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Background))
			.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::SelectHover))
			.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::SelectHover))
		);

		/* Sound Dashboard */
		// Icons
		Set("AtomInsights.Icon.SoundDashboard.Browse",           	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/browse"),			Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Edit",             	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/edit"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Filter",           	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/filter"),			Icon16));
		// @TODO UE-250399: Hide category pending to implement
		//Set("AtomInsights.Icon.SoundDashboard.Hide",             	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/hide"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Info",             	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/info"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.MetaSound",        	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/metasound"),			Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Mute",             	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/mute"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Pin",              	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/pin"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.ProceduralSource",	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/proceduralsource"),	Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Reset",            	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/reset"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Solo",             	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/solo"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.SoundCue",         	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/soundcue"),			Icon16));
		Set("AtomInsights.Icon.SoundDashboard.SoundWave",        	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/soundwave"),			Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Tab",              	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/tab"),				Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Transparent",      	new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/transparent"),		Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Visible",		  		new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/visible"),			Icon16));
		Set("AtomInsights.Icon.SoundDashboard.Invisible",			new IMAGE_BRUSH_SVG(TEXT("Icons/SoundDashboard/invisible"),			Icon16));
			
		// Category colors
		Set("SoundDashboard.MetaSoundColor", FLinearColor(0.008f, 0.76f, 0.078f));
		Set("SoundDashboard.SoundCueColor", FLinearColor(0.98f, 0.49f, 0.14f));
		Set("SoundDashboard.ProceduralSourceColor", FLinearColor(0.98f, 0.32f, 0.006f));
		Set("SoundDashboard.SoundWaveColor", FLinearColor(0.12f, 0.093f, 0.64f));
		//Set("SoundDashboard.SoundCueTemplateColor", FLinearColor(0.98f, 0.01f, 0.01f));
		Set("SoundDashboard.PinnedColor", FLinearColor(0.9f, 0.9f, 0.9f));
		Set("SoundDashboard.HiddenColor", FLinearColor(0.4f, 0.4f, 0.4f));

		// Text colors
		Set("SoundDashboard.TimingOutTextColor", FLinearColor(1.0f, 1.0f, 1.0f, 0.25f));

		// Mute/Solo buttons style
		const FSlateRoundedBoxBrush RoundedWhiteAlphaBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.1f), 5.0f);

		Set("SoundDashboard.MuteSoloButton", FCheckBoxStyle(FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("TransparentCheckBox"))
			.SetPadding(FMargin(5.0f, 2.0f, 5.0f, 2.0f))
			.SetCheckedHoveredImage(RoundedWhiteAlphaBrush)
			.SetUncheckedHoveredImage(RoundedWhiteAlphaBrush)
		);

		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
