
#include "AtomLevelMeterStyle.h"

#include "Styling/StyleDefaults.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomLevelMeterStyle)

FAtomLevelMeterStyle::FAtomLevelMeterStyle()
	: MeterSize(FVector2D(250.0f, 25.0f))
	, MeterPadding(FVector2D(10.0f, 5.0f))
	, MeterValuePadding(3.0f)
	, PeakValueWidth(2.0f)
	, ValueRangeDb(FVector2D(-96, 10))
	, bShowScale(true)
	, bScaleSide(true)
	, ScaleHashOffset(5.0f)
	, ScaleHashWidth(1.0f)
	, ScaleHashHeight(10.0f)
	, DecibelsPerHash(10)
	, Font(FStyleDefaults::GetFontInfo(5))
{
}

void FAtomLevelMeterStyle::GetResources(TArray< const FSlateBrush* >& OutBrushes) const
{
	OutBrushes.Add(&MeterValueImage);
	OutBrushes.Add(&MeterBackgroundImage);
	OutBrushes.Add(&MeterPeakImage);
}

const FName FAtomLevelMeterStyle::TypeName(TEXT("FAtomLevelMeterStyle"));

const FAtomLevelMeterStyle& FAtomLevelMeterStyle::GetDefault()
{
	static FAtomLevelMeterStyle Default;
	return Default;
}
