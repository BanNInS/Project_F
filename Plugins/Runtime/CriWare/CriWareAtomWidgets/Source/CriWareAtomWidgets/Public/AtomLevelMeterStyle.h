
#pragma once

#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateWidgetStyle.h"
#include "Styling/SlateBrush.h"

#include "AtomLevelMeterStyle.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

struct FCompositeFont;

/**
 * Represents the appearance of an SAtomLevelMeter
 */
USTRUCT(BlueprintType)
struct FAtomLevelMeterStyle : public FSlateWidgetStyle
{
	GENERATED_BODY()

	CRI_API FAtomLevelMeterStyle();

	virtual ~FAtomLevelMeterStyle() {}

	CRI_API virtual void GetResources( TArray< const FSlateBrush* >& OutBrushes ) const override;

	static CRI_API const FName TypeName;
	virtual const FName GetTypeName() const override { return TypeName; };

	static CRI_API const FAtomLevelMeterStyle& GetDefault();

	/** Image to use to represent the meter value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateBrush MeterValueImage;
	FAtomLevelMeterStyle& SetMeterValueImage(const FSlateBrush& InMeterValueImage){ MeterValueImage = InMeterValueImage; return *this; }

	/** Image to use to represent the background. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FSlateBrush BackgroundImage;
	FAtomLevelMeterStyle& SetBackgroundImage(const FSlateBrush& InBackgroundImage) { BackgroundImage = InBackgroundImage; return *this; }

	/** Image to use to represent the meter background. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FSlateBrush MeterBackgroundImage;
	FAtomLevelMeterStyle& SetMeterBackgroundImage(const FSlateBrush& InMeterBackgroundImage) { MeterBackgroundImage = InMeterBackgroundImage; return *this; }

	/** Image to use to draw behind the meter value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FSlateBrush MeterValueBackgroundImage;
	FAtomLevelMeterStyle& SetMeterValueBackgroundImage(const FSlateBrush& InMeterValueBackgroundImage) { MeterValueBackgroundImage = InMeterValueBackgroundImage; return *this; }

	/** Image to use to represent the meter peak. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FSlateBrush MeterPeakImage;
	FAtomLevelMeterStyle& SetMeterPeakImage(const FSlateBrush& InMeterPeakImage) { MeterPeakImage = InMeterPeakImage; return *this; }

	// How thick to draw the meter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FVector2D MeterSize;
	FAtomLevelMeterStyle& SetMeterSize(const FVector2D& InMeterSize) { MeterSize = InMeterSize; return *this; }

	// How much padding to add around the meter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FVector2D MeterPadding;
	FAtomLevelMeterStyle& SetMeterPadding(const FVector2D& InMeterPadding) { MeterPadding = InMeterPadding; return *this; }

	// How much padding to add around the meter value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float MeterValuePadding;
	FAtomLevelMeterStyle& SetMeterValuePadding(float InMeterValuePadding) { MeterValuePadding = InMeterValuePadding; return *this; }

	// How wide to draw the peak value indicator
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float PeakValueWidth;
	FAtomLevelMeterStyle& SetPeakValueWidth(float InPeakValueWidth) { PeakValueWidth = InPeakValueWidth; return *this; }

	// The minimum and maximum value to display in dB (values are clamped in this range)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FVector2D ValueRangeDb;
	FAtomLevelMeterStyle& SetValueRangeDb(const FVector2D& InValueRangeDb) { ValueRangeDb = InValueRangeDb; return *this; }

	// Whether or not to show the decibel scale alongside the meter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	bool bShowScale;
	FAtomLevelMeterStyle& SetShowScale(bool bInShowScale) { bShowScale = bInShowScale; return *this; }

	// Which side to show the scale. If vertical, true means left side, false means right side. If horizontal, true means above, false means below.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	bool bScaleSide;
	FAtomLevelMeterStyle& SetScaleSide(bool bInScaleSide) { bScaleSide = bInScaleSide; return *this; }

	// Offset for the hashes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float ScaleHashOffset;
	FAtomLevelMeterStyle& SetScaleHashOffset(float InScaleHashOffset) { ScaleHashOffset = InScaleHashOffset; return *this; }

	// The width of each hash mark
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float ScaleHashWidth;
	FAtomLevelMeterStyle& SetScaleHashWidth(float InScaleHashWidth) { ScaleHashWidth = InScaleHashWidth; return *this; }

	// The height of each hash mark
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	float ScaleHashHeight;
	FAtomLevelMeterStyle& SetScaleHashHeight(float InScaleHashHeight) { ScaleHashHeight = InScaleHashHeight; return *this; }

	// How wide to draw the decibel scale, if it's enabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance, meta = (UIMin = "3", ClampMin="3", UIMax = "10"))
	int32 DecibelsPerHash;
	FAtomLevelMeterStyle& SetDecibelsPerHash(float InDecibelsPerHash) { DecibelsPerHash = InDecibelsPerHash; return *this; }

	/** Font family and size to be used when displaying the meter scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Appearance)
	FSlateFontInfo Font;
	FAtomLevelMeterStyle& SetFont(const FSlateFontInfo& InFont) { Font = InFont; return *this; }
	FAtomLevelMeterStyle& SetFont(TSharedPtr<const FCompositeFont> InCompositeFont, const int32 InSize, const FName& InTypefaceFontName = NAME_None) { Font = FSlateFontInfo(InCompositeFont, InSize, InTypefaceFontName); return *this; }
	FAtomLevelMeterStyle& SetFont(const UObject* InFontObject, const int32 InSize, const FName& InTypefaceFontName = NAME_None) { Font = FSlateFontInfo(InFontObject, InSize, InTypefaceFontName); return *this; }

	FAtomLevelMeterStyle& SetFontSize(uint16 InSize) { Font.Size = InSize; return *this; }
	FAtomLevelMeterStyle& SetTypefaceFontName(const FName& InTypefaceFontName) { Font.TypefaceFontName = InTypefaceFontName; return *this; }

	/**
	 * Unlinks all colors in this style.
	 * @see FSlateColor::Unlink
	 */
	void UnlinkColors()
	{
		MeterValueImage.UnlinkColors();
		MeterBackgroundImage.UnlinkColors();
//		MeterScaleImage.UnlinkColors();
		MeterPeakImage.UnlinkColors();
	}
};

#undef CRI_API
