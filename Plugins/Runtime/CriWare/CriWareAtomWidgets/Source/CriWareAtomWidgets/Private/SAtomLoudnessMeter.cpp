
#include "SAtomLoudnessMeter.h"

#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"

#include "SAtomLevelMeter.h"

#define LOCTEXT_NAMESPACE "SAtomLoudnessMeter"

FName SAtomLoudnessMeter::ContextMenuExtensionHook("Loudness Meter");

SAtomLoudnessMeter::SAtomLoudnessMeter()
{
}

void SAtomLoudnessMeter::Construct(const SAtomLoudnessMeter::FArguments& InArgs)
{
	check(InArgs._Style);

	BackgroundColor = InArgs._BackgroundColor;
	MetersBackgroundColor = InArgs._MetersBackgroundColor;
	MomentaryValueColor = InArgs._MomentaryValueColor;
	ShortTermValueColor = InArgs._ShortTermValueColor;
	IntegratedValueColor = InArgs._IntegratedValueColor;
	MeterPeakColor = InArgs._MeterPeakColor;
	MeterScaleColor = InArgs._MeterScaleColor;
	MeterScaleLabelColor = InArgs._MeterScaleLabelColor;
	MeterClippingColor = InArgs._MeterClippingColor;
	OnAtomLoudnessMeterResetMenuEntryClicked = InArgs._OnAtomLoudnessMeterResetMenuEntryClicked;

	Style = InArgs._Style;

	MeterInfoAttribute = InArgs._MeterInfo;

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 1;
	Options.MaximumFractionalDigits = 1;

	LoudnessMeterStyle = *Style;
	LoudnessMeterStyle
		.SetMeterSize(FVector2D(250.0f, 60.0f))
		.SetFontSize(10.0f);

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.MinWidth(120.0f)
		.Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				SNew(SAtomLevelMeter)
				.ToolTipText(LOCTEXT("LoudnessMeterMomentaryToolTip", "Momentary LUFS"))
				.Orientation(EOrientation::Orient_Vertical)
				.BackgroundColor(BackgroundColor)
				.MeterBackgroundColor(MetersBackgroundColor)
				.MeterValueColor(MomentaryValueColor)
				.MeterPeakColor(MeterPeakColor)
				.MeterClippingColor(MeterClippingColor)
				.MeterScaleColor(MeterScaleColor)
				.MeterScaleLabelColor(MeterScaleLabelColor)
				.Style(&LoudnessMeterStyle)
				.MeterChannelInfo_Lambda([this]()
				{
					const float Value = MeterInfoAttribute.Get().MomentaryValue;
					TArray<FAtomLevelMeterChannelInfo> Array;
					Array.Add(FAtomLevelMeterChannelInfo{ Value, Value, Value });
					return Array;
				})
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 4, 0, 2)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Text(LOCTEXT("LoudnessMeterMomentaryText", "Momentary"))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 2, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Text_Lambda([this, Options]()
						{
							return FText::Format(LOCTEXT("LoudnessMeterMomentaryValue", "{0}"), { FText::AsNumber(MeterInfoAttribute.Get().MomentaryValue, &Options) });
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Right)
					.Padding(6, 0, 0, 0)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.Text(LOCTEXT("LoudnessMeterUnit", "LUFS"))
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.MinWidth(120.0f)
		.Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				SNew(SAtomLevelMeter)
				.ToolTipText(LOCTEXT("LoudnessMeterShortTermToolTip", "Short-term LUFS"))
				.Orientation(EOrientation::Orient_Vertical)
				.BackgroundColor(BackgroundColor)
				.MeterBackgroundColor(MetersBackgroundColor)
				.MeterValueColor(ShortTermValueColor)
				.MeterPeakColor(MeterPeakColor)
				.MeterClippingColor(MeterClippingColor)
				.MeterScaleColor(MeterScaleColor)
				.MeterScaleLabelColor(MeterScaleLabelColor)
				.Style(&LoudnessMeterStyle)
				.MeterChannelInfo_Lambda([this]()
				{
					const float Value = MeterInfoAttribute.Get().ShortTermValue;
					TArray<FAtomLevelMeterChannelInfo> Array;
					Array.Add(FAtomLevelMeterChannelInfo{ Value, Value, Value });
					return Array;
				})
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 4, 0, 2)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Text(LOCTEXT("LoudnessMeterShortTermText", "Short-term"))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 2, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Text_Lambda([this, Options]()
						{
							return FText::Format(LOCTEXT("LoudnessMeterShortTermValue", "{0}"), { FText::AsNumber(MeterInfoAttribute.Get().ShortTermValue, &Options) });
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Bottom)
					.Padding(6, 0, 0, 0)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.Text(LOCTEXT("LoudnessMeterUnit", "LUFS"))
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.MinWidth(120.0f)
		.Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				SNew(SAtomLevelMeter)
				.ToolTipText(LOCTEXT("LoudnessMeterIntegratedToolTip", "Integrated LUFS"))
				.Orientation(EOrientation::Orient_Vertical)
				.BackgroundColor(BackgroundColor)
				.MeterBackgroundColor(MetersBackgroundColor)
				.MeterValueColor(IntegratedValueColor)
				.MeterPeakColor(MeterPeakColor)
				.MeterClippingColor(MeterClippingColor)
				.MeterScaleColor(MeterScaleColor)
				.MeterScaleLabelColor(MeterScaleLabelColor)
				.Style(&LoudnessMeterStyle)
				.MeterChannelInfo_Lambda([this]()
				{
					const float Value = MeterInfoAttribute.Get().IntegratedValue;
					TArray<FAtomLevelMeterChannelInfo> Array;
					Array.Add(FAtomLevelMeterChannelInfo{ Value, Value, Value });
					return Array;
				})
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 4, 0, 2)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Text(LOCTEXT("LoudnessMeterIntegratedText", "Integrated"))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 2, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.Text_Lambda([this, Options]()
						{
							return FText::Format(LOCTEXT("LoudnessMeterIntegratedValue", "{0}"), { FText::AsNumber(MeterInfoAttribute.Get().IntegratedValue, &Options) });
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Bottom)
					.Padding(6, 0, 0, 0)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.Text(LOCTEXT("LoudnessMeterUnit", "LUFS"))
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.MinWidth(120.0f)
		.Padding(4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				SNew(SAtomLevelMeter)
				.Orientation(EOrientation::Orient_Vertical)
				.BackgroundColor(BackgroundColor)
				.MeterBackgroundColor(MetersBackgroundColor)
				.MeterValueColor(TruePeakValueColor)
				.MeterPeakColor(TruePeakValueColor)
				.MeterClippingColor(MeterClippingColor)
				.MeterScaleColor(MeterScaleColor)
				.MeterScaleLabelColor(MeterScaleLabelColor)
				.Style(Style)
				.MeterChannelInfo_Lambda([this]()
				{
					TArray<FAtomLevelMeterChannelInfo> Array;
					Array.Add(MeterInfoAttribute.Get().TruePeakMeterChannelInfo);
					return Array;
				})
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.AutoHeight()
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 4, 0, 2)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.Text(LOCTEXT("LoudnessMeterMaxTruePeakText", "Max True Peak"))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				.Padding(0, 2, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.Text_Lambda([this, Options]()
						{
							MaxTruePeakValue = FMath::Max(MaxTruePeakValue, MeterInfoAttribute.Get().TruePeakMeterChannelInfo.MeterValue);
							return FText::Format(LOCTEXT("LoudnessMeterTruePeakValue", "{0}"), { FText::AsNumber(MaxTruePeakValue, &Options) });
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Bottom)
					.Padding(6, 0, 0, 0)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.Text(LOCTEXT("TruePeakMeterUnit", "dB"))
					]
				]
			]
		]
	];
}

TSharedRef<const FExtensionBase> SAtomLoudnessMeter::AddContextMenuExtension(EExtensionHook::Position HookPosition, const TSharedPtr<FUICommandList>& CommandList, const FMenuExtensionDelegate& MenuExtensionDelegate)
{
	if (!ContextMenuExtender.IsValid())
	{
		ContextMenuExtender = MakeShared<FExtender>();
	}

	return ContextMenuExtender->AddMenuExtension(ContextMenuExtensionHook, HookPosition, CommandList, MenuExtensionDelegate);
}

void SAtomLoudnessMeter::RemoveContextMenuExtension(const TSharedRef<const FExtensionBase>& Extension)
{
	if (ensure(ContextMenuExtender.IsValid()))
	{
		ContextMenuExtender->RemoveExtension(Extension);
	}
}

FReply SAtomLoudnessMeter::OnMouseButtonDown(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (!HasMouseCapture())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			// Right clicking to summon context menu, but we'll do that on mouse-up.
			return FReply::Handled().CaptureMouse(AsShared()).SetUserFocus(AsShared(), EFocusCause::Mouse);
		}
	}

	return SCompoundWidget::OnMouseButtonDown(InMyGeometry, InMouseEvent);
}

FReply SAtomLoudnessMeter::OnMouseButtonUp(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	// The mouse must have been captured by mouse down before we'll process mouse ups
	if (HasMouseCapture())
	{
		if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (InMyGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
			{
				TSharedPtr<SWidget> ContextMenu = /*OnContextMenuOpening.IsBound() ? OnContextMenuOpening.Execute() : */ BuildDefaultContextMenu();

				if (ContextMenu.IsValid())
				{
					const FWidgetPath WidgetPath = (InMouseEvent.GetEventPath() != nullptr) ? *InMouseEvent.GetEventPath() : FWidgetPath();

					FSlateApplication::Get().PushMenu(
						AsShared(),
						WidgetPath,
						ContextMenu.ToSharedRef(),
						InMouseEvent.GetScreenSpacePosition(),
						FPopupTransitionEffect::ESlideDirection::ContextMenu);
				}
			}

			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return SCompoundWidget::OnMouseButtonUp(InMyGeometry, InMouseEvent);
}

float SAtomLoudnessMeter::GetScaleHeight() const
{
	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D LabelSize = FontMeasureService->Measure(FString::FromInt(-60), Style->Font);

	float ScaleHeight = Style->ScaleHashHeight + Style->ScaleHashOffset;
	return ScaleHeight + LabelSize.X;
}

void SAtomLoudnessMeter::SetMeterInfo(const TAttribute<FAtomLoudnessMeterInfo>& InMeterInfo)
{
	SetAttribute(MeterInfoAttribute, InMeterInfo, EInvalidateWidgetReason::Paint);
}

FAtomLoudnessMeterInfo SAtomLoudnessMeter::GetMeterInfo() const
{
	return MeterInfoAttribute.Get();
}

void SAtomLoudnessMeter::SetBackgroundColor(FSlateColor InBackgroundColor)
{
	SetAttribute(BackgroundColor, TAttribute<FSlateColor>(InBackgroundColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMetersBackgroundColor(FSlateColor InMeterBackgroundColor)
{
	SetAttribute(MetersBackgroundColor, TAttribute<FSlateColor>(InMeterBackgroundColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMomentaryValueColor(FSlateColor InMeterValueColor)
{
	SetAttribute(MomentaryValueColor, TAttribute<FSlateColor>(InMeterValueColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetShortTermValueColor(FSlateColor InMeterValueColor)
{
	SetAttribute(ShortTermValueColor, TAttribute<FSlateColor>(InMeterValueColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetIntegratedValueColor(FSlateColor InMeterValueColor)
{
	SetAttribute(IntegratedValueColor, TAttribute<FSlateColor>(InMeterValueColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetTruePeakValueColor(FSlateColor InMeterValueColor)
{
	SetAttribute(TruePeakValueColor, TAttribute<FSlateColor>(InMeterValueColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMeterPeakColor(FSlateColor InMeterPeakColor)
{
	SetAttribute(MeterPeakColor, TAttribute<FSlateColor>(InMeterPeakColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMeterClippingColor(FSlateColor InMeterClippingColor)
{
	SetAttribute(MeterClippingColor, TAttribute<FSlateColor>(InMeterClippingColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMeterScaleColor(FSlateColor InMeterScaleColor)
{
	SetAttribute(MeterScaleColor, TAttribute<FSlateColor>(InMeterScaleColor), EInvalidateWidgetReason::Paint);
}

void SAtomLoudnessMeter::SetMeterScaleLabelColor(FSlateColor InMeterScaleLabelColor)
{
	SetAttribute(MeterScaleLabelColor, TAttribute<FSlateColor>(InMeterScaleLabelColor), EInvalidateWidgetReason::Paint);
}

TSharedRef<SWidget> SAtomLoudnessMeter::BuildDefaultContextMenu()
{
	constexpr bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, nullptr, ContextMenuExtender);
	MenuBuilder.BeginSection(ContextMenuExtensionHook, LOCTEXT("LoudnessMeter", "Loudness Meter"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Reset", "Reset"),
		LOCTEXT("Reset", "Reset"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSPLambda(this, [this]()
			{
				MaxTruePeakValue = -96.0f;

				if (OnAtomLoudnessMeterResetMenuEntryClicked.IsBound())
				{
					OnAtomLoudnessMeterResetMenuEntryClicked.Execute();
				}
			}),
			FCanExecuteAction()
		),
		NAME_None,
		EUserInterfaceActionType::Button);

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
