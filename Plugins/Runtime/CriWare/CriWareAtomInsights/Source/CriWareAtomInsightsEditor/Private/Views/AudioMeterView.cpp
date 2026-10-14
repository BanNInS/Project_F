
#include "AudioMeterView.h"

#include "Widgets/SNullWidget.h"

#include "AtomLevelMeter.h"
#include "SAtomLevelMeter.h"

#include "Atom/AtomAudioBus.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomRack.h"

#include "AtomInsightsEditorDashboardFactory.h"
#include "Analyzers/AudioMeterRackAnalyzer.h"
#include "Analyzers/AudioMeterBusAnalyzer.h"
#include "Views/SubmixesDashboardViewFactory.h"

namespace Atom::Insights
{
	FAudioMeterView::FAudioMeterView(FAudioAssetVariant InAudioAssetVariant)
		: AudioAssetVariant(InAudioAssetVariant)
		, AudioMeterAnalyzerVariant(MakeAudioMeterAnalyzerVariant(InAudioAssetVariant))
		, AudioAssetNameTextBlock(MakeAudioAssetNameTextBlock(InAudioAssetVariant))
		, AudioMeterViewWidget(MakeWidget())
		, OnActiveAtomRuntimeChangedHandle(FEditorDashboardFactory::OnActiveAtomRuntimeChanged.AddRaw(this, &FAudioMeterView::HandleOnActiveAtomRuntimeChanged))
	{
		
	}

	FAudioMeterView::~FAudioMeterView()
	{
		FEditorDashboardFactory::OnActiveAtomRuntimeChanged.Remove(OnActiveAtomRuntimeChangedHandle);
	}

	FAudioMeterView::FAudioMeterVariant FAudioMeterView::MakeAudioMeterAnalyzerVariant(const FAudioAssetVariant InAudioAssetVariant)
	{
		switch (AudioAssetVariant.GetIndex())
		{
			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomAudioBus>>():
			{
				return FAudioMeterVariant(TInPlaceType<TSharedPtr<FAudioMeterAnalyzer>>(), MakeShared<FAudioMeterAnalyzer>(InAudioAssetVariant.Get<TWeakObjectPtr<UAtomAudioBus>>()));
			}

			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomRackWithParentBase>>():
			{
				return FAudioMeterVariant(TInPlaceType<TSharedPtr<FAudioMeterRackAnalyzer>>(), MakeShared<FAudioMeterRackAnalyzer>(InAudioAssetVariant.Get<TWeakObjectPtr<UAtomRackWithParentBase>>()));
			}

			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomBus>>():
			{
				return FAudioMeterVariant(TInPlaceType<TSharedPtr<FAudioMeterBusAnalyzer>>(), MakeShared<FAudioMeterBusAnalyzer>(InAudioAssetVariant.Get<TWeakObjectPtr<UAtomBus>>()));
			}

			default:
				return FAudioMeterView::FAudioMeterVariant();
		}
	}

	TSharedRef<STextBlock> FAudioMeterView::MakeAudioAssetNameTextBlock(const FAudioAssetVariant InAudioAssetVariant)
	{
		switch (InAudioAssetVariant.GetIndex())
		{
			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomAudioBus>>():
			{
				if (TWeakObjectPtr<UAtomAudioBus> AudioBusAsset = InAudioAssetVariant.Get<TWeakObjectPtr<UAtomAudioBus>>();
					AudioBusAsset.IsValid())
				{
					return SNew(STextBlock)
						.Text_Lambda([AudioBusAsset] { return AudioBusAsset.IsValid() ? FText::FromString(AudioBusAsset->GetName()) : FText::GetEmpty(); })
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FSlateColor(FColor(80, 200, 255)));
				}
			}
			break;

			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomRackWithParentBase>>():
			{
				if (TWeakObjectPtr<UAtomRackWithParentBase> AtomRackAsset = AudioAssetVariant.Get<TWeakObjectPtr<UAtomRackWithParentBase>>();
					AtomRackAsset.IsValid())
				{
					return SNew(STextBlock)
						.Text_Lambda([AtomRackAsset] { return AtomRackAsset.IsValid() ? FText::FromString(AtomRackAsset->GetName()) : FText::GetEmpty(); })
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FSlateColor(FColor(30, 240, 90)));
				}
			}
			break;


			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomBus>>():
			{
				if (TWeakObjectPtr<UAtomBus> AtomBusAsset = AudioAssetVariant.Get<TWeakObjectPtr<UAtomBus>>();
					AtomBusAsset.IsValid())
				{
					return SNew(STextBlock)
						.Text_Lambda([AtomBusAsset] { return AtomBusAsset.IsValid() ? FText::FromString(AtomBusAsset->GetName()) : FText::GetEmpty(); })
						.Justification(ETextJustify::Center)
						.ColorAndOpacity(FSlateColor(FColor(90, 90, 90)));
				}
			}
			break;
		}

		return SNew(STextBlock);
	}

	TSharedRef<SWidget> FAudioMeterView::GetAudioMeterAnalyzerWidget() const
	{
		switch (AudioMeterAnalyzerVariant.GetIndex())
		{
			case FAudioMeterVariant::IndexOfType<TSharedPtr<FAudioMeterAnalyzer>>():
			{
				TSharedPtr<FAudioMeterAnalyzer> AudioMeterAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterAnalyzer>>();
				return AudioMeterAnalyzer.IsValid() ? AudioMeterAnalyzer->GetLevelMeter()->GetWidget() : SNullWidget::NullWidget;
			}

			case FAudioMeterVariant::IndexOfType<TSharedPtr<FAudioMeterRackAnalyzer>>():
			{
				TSharedPtr<FAudioMeterRackAnalyzer> AudioMeterRackAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterRackAnalyzer>>();
				return AudioMeterRackAnalyzer.IsValid() ? AudioMeterRackAnalyzer->GetWidget() : SNullWidget::NullWidget;
			}

			case FAudioMeterVariant::IndexOfType<TSharedPtr<FAudioMeterBusAnalyzer>>():
			{
				TSharedPtr<FAudioMeterBusAnalyzer> AudioMeterBusAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterBusAnalyzer>>();
				return AudioMeterBusAnalyzer.IsValid() ? AudioMeterBusAnalyzer->GetWidget() : SNullWidget::NullWidget;
			}

			default:
				return SNullWidget::NullWidget;
		}
	}

	TSharedRef<SWidget> FAudioMeterView::MakeWidget()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SVerticalBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				// Height padding
				+ SVerticalBox::Slot()
				.FillHeight(0.05)
				[
					SNew(SBox)
				]
				// Audio Meter container
				+ SVerticalBox::Slot()
				.FillHeight(0.8)
				[
					// Audio Meter
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					[
						GetAudioMeterAnalyzerWidget()
					]
				]
				// Height padding
				+ SVerticalBox::Slot()
				.FillHeight(0.025)
				[
					SNew(SBox)
				]
				// Audio aaset name label
				+ SVerticalBox::Slot()
				.FillHeight(0.1)
				[
					AudioAssetNameTextBlock
				]
				// Height padding
				+ SVerticalBox::Slot()
				.FillHeight(0.025)
				[
					SNew(SBox)
				]
			];
	}

	void FAudioMeterView::HandleOnActiveAtomRuntimeChanged()
	{
		switch (AudioAssetVariant.GetIndex())
		{
			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomAudioBus>>():
			{
				if (TSharedPtr<FAudioMeterAnalyzer> AudioMeterAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterAnalyzer>>();
					AudioMeterAnalyzer.IsValid())
				{
					AudioMeterAnalyzer->RebuildAudioMeter(AudioAssetVariant.Get<TWeakObjectPtr<UAtomAudioBus>>());
				}
			}
			break;

			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomRackWithParentBase>>():
			{
				if (TSharedPtr<FAudioMeterRackAnalyzer> AudioMeterRackAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterRackAnalyzer>>();
					AudioMeterRackAnalyzer.IsValid())
				{
					AudioMeterRackAnalyzer->SetRack(AudioAssetVariant.Get<TWeakObjectPtr<UAtomRackWithParentBase>>());
				}
			}
			break;

			case FAudioAssetVariant::IndexOfType<TWeakObjectPtr<UAtomBus>>():
			{
				if (TSharedPtr<FAudioMeterBusAnalyzer> AudioMeterBusAnalyzer = AudioMeterAnalyzerVariant.Get<TSharedPtr<FAudioMeterBusAnalyzer>>();
					AudioMeterBusAnalyzer.IsValid())
				{
					AudioMeterBusAnalyzer->SetBus(AudioAssetVariant.Get<TWeakObjectPtr<UAtomBus>>());
				}
			}
			break;

			default:
				break;
		}
	}
} // namespace
