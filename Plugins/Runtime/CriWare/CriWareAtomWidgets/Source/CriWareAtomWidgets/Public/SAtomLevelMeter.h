// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/ISlateStyle.h"
#include "Styling/SlateWidgetStyleAsset.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

#include "AtomWidgetsStyle.h"
#include "AtomLevelMeterStyle.h"
#include "AtomLevelMeterTypes.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class FPaintArgs;
class FSlateWindowElementList;

class SAtomLevelMeterBase
	: public SLeafWidget
{
public:

	SLATE_BEGIN_ARGS(SAtomLevelMeterBase){}

	SLATE_END_ARGS()

	/** Is the active timer registered to refresh the meter channel info. */
	bool bIsActiveTimerRegistered = false;

public:

	virtual void SetMeterChannelInfo(const TAttribute<TArray<FAtomLevelMeterChannelInfo>>& InMeterChannelInfo) = 0;
	virtual TArray<FAtomLevelMeterChannelInfo> GetMeterChannelInfo() const = 0;
};

/**
 * A Slate slider control is a linear scale and draggable handle.
 */
class SAtomLevelMeter
	: public SAtomLevelMeterBase
{
public:

	SLATE_BEGIN_ARGS(SAtomLevelMeter)
		: _Orientation(EOrientation::Orient_Horizontal)
		, _BackgroundColor(FLinearColor::Black)
		, _MeterBackgroundColor(FLinearColor::Gray)
		, _MeterValueColor(FLinearColor::Green)
		, _MeterPeakColor(FLinearColor::Blue)
		, _MeterScaleColor(FLinearColor::White)
		, _MeterScaleLabelColor(FLinearColor::Gray)
		, _Style(&AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterStyle>("AtomLevelMeter.Style"))
	{
	}

	/** Whether the slidable area should be indented to fit the handle. */
	SLATE_ATTRIBUTE(bool, IndentHandle)

		/** The slider's orientation. */
		SLATE_ARGUMENT(EOrientation, Orientation)

		/** The color to draw the background in. */
		SLATE_ATTRIBUTE(FSlateColor, BackgroundColor)

		/** The color to draw the meter background in. */
		SLATE_ATTRIBUTE(FSlateColor, MeterBackgroundColor)

		/** The color to draw the meter value in. */
		SLATE_ATTRIBUTE(FSlateColor, MeterValueColor)

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
		SLATE_ATTRIBUTE(TArray<FAtomLevelMeterChannelInfo>, MeterChannelInfo)

	SLATE_END_ARGS()

	CRI_API SAtomLevelMeter();

	/**
	 * Construct the widget.
	 *
	 * @param InDeclaration A declaration from which to construct the widget.
	 */
	CRI_API void Construct(const SAtomLevelMeter::FArguments& InDeclaration);

	CRI_API void SetMeterChannelInfo(const TAttribute<TArray<FAtomLevelMeterChannelInfo>>& InMeterChannelInfo) override;
	CRI_API TArray<FAtomLevelMeterChannelInfo> GetMeterChannelInfo() const override;

	/** Set the Orientation attribute */
	CRI_API void SetOrientation(EOrientation InOrientation);

	CRI_API void SetBackgroundColor(FSlateColor InBackgroundColor);
	CRI_API void SetMeterBackgroundColor(FSlateColor InMeterBackgroundColor);
	CRI_API void SetMeterValueColor(FSlateColor InMeterValueColor);
	CRI_API void SetMeterPeakColor(FSlateColor InMeterPeakColor);
	CRI_API void SetMeterClippingColor(FSlateColor InMeterPeakColor);
	CRI_API void SetMeterScaleColor(FSlateColor InMeterScaleColor);
	CRI_API void SetMeterScaleLabelColor(FSlateColor InMeterScaleLabelColor);

public:

	// SWidget overrides

	CRI_API virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	CRI_API virtual FVector2D ComputeDesiredSize(float) const override;
	CRI_API virtual bool ComputeVolatility() const override;

protected:
	
	// Returns the scale height based off font size and hash height
	CRI_API float GetScaleHeight() const;
	
	// Holds the style passed to the widget upon construction.
	const FAtomLevelMeterStyle* Style;

	// Holds the slider's orientation.
	EOrientation Orientation;

	// Various colors
	TAttribute<FSlateColor> BackgroundColor;
	TAttribute<FSlateColor> MeterBackgroundColor;
	TAttribute<FSlateColor> MeterValueColor;
	TAttribute<FSlateColor> MeterPeakColor;
	TAttribute<FSlateColor> MeterClippingColor;
	TAttribute<FSlateColor> MeterScaleColor;
	TAttribute<FSlateColor> MeterScaleLabelColor;

	TAttribute<TArray<FAtomLevelMeterChannelInfo>> MeterChannelInfoAttribute;
};

#undef CRI_API
