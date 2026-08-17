#include "SAtomRadialSlider.h"

#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SRadialSlider.h"

#define LOCTEXT_NAMESPACE "SAtomRadialSlider"

namespace AtomWidgets
{
	void SAtomRadialSlider::Construct(const FArguments& InArgs)
	{
		// keep args

		Value = InArgs._Value;
		OnValueChanged = InArgs._OnValueChanged;

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Center)
			.Padding(4.0f)
			[
				SNew(STextBlock)
				.Text(InArgs._Name)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.MaxHeight(100 * InArgs._Scale)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SAssignNew(Radial, SRadialSlider)
				.OnValueChanged_Lambda([this](float InSliderValue)
				{
					OnValueChanged.ExecuteIfBound(InSliderValue);

				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Center)
			.Padding(-10.0f, 4.0f, 4.0f, 4.0f)
			[
				SAssignNew(Text, STextBlock)
				.Text(LOCTEXT("RadialSliderNoText", "NA"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			]
		];

		FRuntimeFloatCurve SliderRange;

		//Set range of X to Y in a linear curve.
		SliderRange.GetRichCurve()->AddKey(0.0f, InArgs._Range.X);
		SliderRange.GetRichCurve()->AddKey(1.0f, InArgs._Range.Y);

		Radial->SetSliderRange(SliderRange);

		FLinearColor SliderBarColor = FLinearColor(
			0x05 / 255.0f,
			0x08 / 255.0f,
			0x0A / 255.0f,
			0xFF / 255.0f
		);

		FSlateColor SlateSliderBarColor = FSlateColor(SliderBarColor);

		FLinearColor SliderProgressColor = FLinearColor(
			0x02 / 255.0f,
			0x03 / 255.0f,
			0x04 / 255.0f,
			0xFF / 255.0f
		);

		FSlateColor SlateSliderProgressColor = FSlateColor(SliderProgressColor);

		FLinearColor SliderHandleColor = FLinearColor(
			0x0E / 255.0f,
			0x5C / 255.0f,
			0x93 / 255.0f,
			0xFF / 255.0f
		);

		FSlateColor SlateSliderHandleColor = FSlateColor(SliderHandleColor);

		Radial->SetSliderBarColor(SlateSliderBarColor);
		Radial->SetSliderProgressColor(SlateSliderProgressColor);
		Radial->SetSliderHandleColor(SlateSliderHandleColor);

	}

	void SAtomRadialSlider::SetValue(const TAttribute<float>& InValue)
	{
		Radial->SetValue(InValue);
	}

	void SAtomRadialSlider::SetTextValue(FText InText)
	{
		Text->SetText(InText);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
