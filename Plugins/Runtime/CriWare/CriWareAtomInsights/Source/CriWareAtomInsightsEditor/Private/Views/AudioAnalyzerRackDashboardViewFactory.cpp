
#include "Views/AudioAnalyzerRackDashboardViewFactory.h"

#include "UObject/WeakObjectPtr.h"
#include "Widgets/Colors/SColorBlock.h"

#include "AtomInsightsEditorDashboardFactory.h"
#include "AtomInsightsStyle.h"
#include "Analyzers/SubmixAudioAnalyzerRack.h"
#include "SubmixesDashboardViewFactory.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	FName FAudioAnalyzerRackDashboardViewFactory::GetName() const
	{
		return "AudioAnalyzerRack";
	}

	FText FAudioAnalyzerRackDashboardViewFactory::GetDisplayName() const
	{
		return LOCTEXT("AudioDashboard_DashboardsAnalyzerRackTab_DisplayName", "Analyzers");
	}

	EDefaultDashboardTabStack FAudioAnalyzerRackDashboardViewFactory::GetDefaultTabStack() const
	{
		return EDefaultDashboardTabStack::AudioAnalyzerRack;
	}

	FSlateIcon FAudioAnalyzerRackDashboardViewFactory::GetIcon() const
	{
		return FSlateStyle::Get().CreateIcon("AtomInsights.Icon");
	}

	TSharedRef<SWidget> FAudioAnalyzerRackDashboardViewFactory::MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs)
	{
		// Create SubmixAudioAnalyzerRack (use MainSubmix as default)

		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			if (auto Runtime = RuntimeManager->GetActiveAtomRuntime(); Runtime.IsValid())
			{
				MainSubmix = Cast<UAtomRackBase>(Runtime->GetMasterRack());
			}
		}

		if (!MainSubmix)
		{
			if (TWeakObjectPtr<UCriWareCoreSettings> AudioSettings = GetMutableDefault<UCriWareCoreSettings>();
				AudioSettings.IsValid())
			{
				MainSubmix = Cast<UAtomRackBase>(AudioSettings->MasterRack.ResolveObject());
			}
		}

		if (MainSubmix)
		{
			UAtomRackWithParentBase* MainRack = Cast<UAtomRackWithParentBase>(MainSubmix);

			// prefer use the main bus since spectrogram and osciloscope for Racks cannot works.
			SubmixAudioAnalyzerRack = MakeShared<FSubmixAudioAnalyzerRack>(MainRack->MasterBus);
			//SubmixAudioAnalyzerRack = MakeShared<FSubmixAudioAnalyzerRack>(MainSubmix);

			FEditorDashboardFactory::OnActiveAtomRuntimeChanged.AddSP(this, &FAudioAnalyzerRackDashboardViewFactory::HandleOnActiveAtomRuntimeChanged);
			FSubmixesDashboardViewFactory::OnSubmixSelectionChanged.AddSP(this, &FAudioAnalyzerRackDashboardViewFactory::HandleOnSubmixSelectionChanged);
		}
		
		if (!SubmixAudioAnalyzerRack.IsValid())
		{
			return SNew(SColorBlock)
				.Color(FSlateStyle::Get().GetColor("AtomInsights.Analyzers.BackgroundColor"));
		}

		return SubmixAudioAnalyzerRack->MakeWidget(OwnerTab, SpawnTabArgs);
	}

	void FAudioAnalyzerRackDashboardViewFactory::HandleOnActiveAtomRuntimeChanged()
	{
		if (SubmixAudioAnalyzerRack.IsValid())
		{
			UAtomRackWithParentBase* MainRack = Cast<UAtomRackWithParentBase>(MainSubmix);

			SubmixAudioAnalyzerRack->RebuildAudioAnalyzerRack(MainRack->MasterBus);
		}
	}

	void FAudioAnalyzerRackDashboardViewFactory::HandleOnSubmixSelectionChanged(const TWeakObjectPtr<UObject> InSubmix)
	{
		if (SubmixAudioAnalyzerRack.IsValid())
		{
			if (UAtomRackWithParentBase* SubmixRack = Cast<UAtomRackWithParentBase>(InSubmix))
			{
				SubmixAudioAnalyzerRack->RebuildAudioAnalyzerRack(SubmixRack->MasterBus);
			}
			else if (UAtomBus* SubmixBus = Cast<UAtomBus>(InSubmix))
			{
				SubmixAudioAnalyzerRack->RebuildAudioAnalyzerRack(SubmixBus);
			}
		}
	}
} // namespace

#undef LOCTEXT_NAMESPACE
