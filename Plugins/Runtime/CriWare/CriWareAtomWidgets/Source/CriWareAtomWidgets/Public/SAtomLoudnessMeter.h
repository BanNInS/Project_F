// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/ISlateStyle.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"

#include "AtomWidgetsStyle.h"
#include "AtomLevelMeterStyle.h"
#include "AtomLevelMeterTypes.h"
#include "AtomLoudnessMeterTypes.h"

#define CRI_API CRIWAREATOMWIDGETS_API

/**
 * A Loudness Meter widget includes all loudness and true peak result in one widget.
 */
class SAtomLoudnessMeter
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAtomLoudnessMeter)
		: _BackgroundColor(FLinearColor::Black)
		, _MetersBackgroundColor(FLinearColor::Gray)
		, _MomentaryValueColor(FLinearColor::Blue)
		, _ShortTermValueColor(FLinearColor::Yellow)
		, _IntegratedValueColor(FLinearColor::Red)
		, _TruePeakValueColor(FLinearColor::Green)
		, _MeterPeakColor(FLinearColor::Blue)
		, _MeterScaleColor(FLinearColor::White)
		, _MeterScaleLabelColor(FLinearColor::Gray)
		, _Style(&AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterStyle>("AtomLevelMeter.Style"))
		{
		}

		/** Whether the slidable area should be indented to fit the handle. */
		SLATE_ATTRIBUTE(bool, IndentHandle)

		/** The color to draw the background in. */
		SLATE_ATTRIBUTE(FSlateColor, BackgroundColor)

		/** The color to draw to all meters background in. */
		SLATE_ATTRIBUTE(FSlateColor, MetersBackgroundColor)

		/** The color to draw the meter value in. */
		SLATE_ATTRIBUTE(FSlateColor, MomentaryValueColor)

		/** The color to draw the meter value in. */
		SLATE_ATTRIBUTE(FSlateColor, ShortTermValueColor)

		/** The color to draw the meter value in. */
		SLATE_ATTRIBUTE(FSlateColor, IntegratedValueColor)

		/** The color to draw the meter value in. */
		SLATE_ATTRIBUTE(FSlateColor, TruePeakValueColor)

		/** The color to draw the meter peak. */
		SLATE_ATTRIBUTE(FSlateColor, MeterPeakColor)

		/** The color to draw the clipping value in. */
		SLATE_ATTRIBUTE(FSlateColor, MeterClippingColor)

		/** The color to draw the scale in. */
		SLATE_ATTRIBUTE(FSlateColor, MeterScaleColor)

		/** The color to draw the scale in. */
		SLATE_ATTRIBUTE(FSlateColor, MeterScaleLabelColor)

		/** The style used to draw the slider. */
		SLATE_STYLE_ARGUMENT(FAtomLevelMeterStyle, Style)

		/** A value representing the audio meter value. */
		SLATE_ATTRIBUTE(FAtomLoudnessMeterInfo, MeterInfo)

		SLATE_EVENT(FOnClicked, OnAtomLoudnessMeterResetMenuEntryClicked)

	SLATE_END_ARGS()

	CRI_API SAtomLoudnessMeter();

	/**
	 * Construct the widget.
	 *
	 * @param InDeclaration A declaration from which to construct the widget.
	 */
	CRI_API void Construct(const SAtomLoudnessMeter::FArguments& InDeclaration);

	CRI_API void SetMeterInfo(const TAttribute<FAtomLoudnessMeterInfo>& InMeterChannelInfo);
	CRI_API FAtomLoudnessMeterInfo GetMeterInfo() const;

	CRI_API void SetBackgroundColor(FSlateColor InBackgroundColor);
	CRI_API void SetMetersBackgroundColor(FSlateColor InMeterBackgroundColor);
	CRI_API void SetMomentaryValueColor(FSlateColor InMeterValueColor);
	CRI_API void SetShortTermValueColor(FSlateColor InMeterValueColor);
	CRI_API void SetIntegratedValueColor(FSlateColor InMeterValueColor);
	CRI_API void SetTruePeakValueColor(FSlateColor InMeterValueColor);
	CRI_API void SetMeterPeakColor(FSlateColor InMeterPeakColor);
	CRI_API void SetMeterClippingColor(FSlateColor InMeterPeakColor);
	CRI_API void SetMeterScaleColor(FSlateColor InMeterScaleColor);
	CRI_API void SetMeterScaleLabelColor(FSlateColor InMeterScaleLabelColor);

	CRI_API TSharedRef<const FExtensionBase> AddContextMenuExtension(EExtensionHook::Position HookPosition, const TSharedPtr<FUICommandList>& CommandList, const FMenuExtensionDelegate& MenuExtensionDelegate);
	CRI_API void RemoveContextMenuExtension(const TSharedRef<const FExtensionBase>& Extension);

	// Begin SWidget overrides.
	CRI_API virtual FReply OnMouseButtonDown(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	CRI_API virtual FReply OnMouseButtonUp(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	// End SWidget overrides.

protected:

	static CRI_API FName ContextMenuExtensionHook;
	TSharedPtr<FExtender> ContextMenuExtender;

	// Returns the scale height based off font size and hash height
	CRI_API float GetScaleHeight() const;

	CRI_API TSharedRef<SWidget> BuildDefaultContextMenu();

	// Holds the style passed to the widget upon construction.
	const FAtomLevelMeterStyle* Style;

	FAtomLevelMeterStyle LoudnessMeterStyle;

	// Various colors
	TAttribute<FSlateColor> BackgroundColor;
	TAttribute<FSlateColor> MetersBackgroundColor;
	TAttribute<FSlateColor> MomentaryValueColor;
	TAttribute<FSlateColor> ShortTermValueColor;
	TAttribute<FSlateColor> IntegratedValueColor;
	TAttribute<FSlateColor> TruePeakValueColor;
	TAttribute<FSlateColor> MeterPeakColor;
	TAttribute<FSlateColor> MeterClippingColor;
	TAttribute<FSlateColor> MeterScaleColor;
	TAttribute<FSlateColor> MeterScaleLabelColor;

	TAttribute<FAtomLoudnessMeterInfo> MeterInfoAttribute;

	FOnClicked OnAtomLoudnessMeterResetMenuEntryClicked;

	float MaxTruePeakValue;
};

#undef CRI_API
