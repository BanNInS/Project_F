#include "SBandpassEQWidget.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateTypes.h"

#include "SBandpassEQGraph.h"
#include "SAtomRadialSlider.h"

#define LOCTEXT_NAMESPACE "BandpassEQWidget"

namespace AtomWidgets
{
	void SBandpassEQWidget::Construct(const FArguments& InArgs)
	{
		Preset = InArgs._Preset;
		if (Preset == nullptr)
		{
			return;
		}
	
		FName PresetName = Preset->GetEffectName();

		ChildSlot
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)

			[	
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				[
					SNew(SBox)
					.MinDesiredWidth(200.0f)
					.MinDesiredHeight(100.0f)
					[
						SNew(SBandpassEQGraph)
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SBox)
					.MinDesiredWidth(200.0f)
					.MinDesiredHeight(100.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SAssignNew(FreqencySlider, SAtomRadialSlider)
							.Name(FText::FromString("Freqency"))
							.OnValueChanged_Lambda([this](float InSliderValue)
							{
								FreqencySlider->SetValue(Preset->GetParameterValue(0));
							})
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SAssignNew(GainSlider, SAtomRadialSlider)
							.Name(FText::FromString("Gain"))
							.OnValueChanged_Lambda([this](float InSliderValue)
							{
								GainSlider->SetValue(Preset->GetParameterValue(0));
							})
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SAssignNew(QSlider, SAtomRadialSlider)
							.Name(FText::FromString("Q"))
							.OnValueChanged_Lambda([this](float InSliderValue)
							{
								QSlider->SetValue(Preset->GetParameterValue(0));
							})
						]
					]
				]
			];
	}
} // namespace

#undef LOCTEXT_NAMESPACE