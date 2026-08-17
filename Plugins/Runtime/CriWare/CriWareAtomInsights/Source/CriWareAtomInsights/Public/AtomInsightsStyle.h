
#pragma once

#include "Styling/SlateStyle.h"
#include "Internationalization/Text.h"

#define CRI_API CRIWAREATOMINSIGHTS_API

namespace Atom::Insights
{
	class  FSlateStyle final : public FSlateStyleSet
	{
	public:
		
		static CRI_API FSlateStyle& Get();

		static CRI_API FName GetStyleName();

		CRI_API const FNumberFormattingOptions* GetAmpFloatFormat()
		{
			static FNumberFormattingOptions FloatFormat;
			FloatFormat.MinimumIntegralDigits = 1;
			FloatFormat.MaximumIntegralDigits = 1;
			FloatFormat.MinimumFractionalDigits = 3;
			FloatFormat.MaximumFractionalDigits = 3;
			return &FloatFormat;
		};

		CRI_API const FNumberFormattingOptions* GetDefaultFloatFormat()
		{
			static FNumberFormattingOptions FloatFormat;
			FloatFormat.MinimumIntegralDigits = 1;
			FloatFormat.MinimumFractionalDigits = 4;
			FloatFormat.MaximumFractionalDigits = 4;
			return &FloatFormat;
		};

		CRI_API const FNumberFormattingOptions* GetFreqFloatFormat()
		{
			static FNumberFormattingOptions FloatFormat;
			FloatFormat.MinimumIntegralDigits = 1;
			FloatFormat.MaximumIntegralDigits = 5;
			FloatFormat.MinimumFractionalDigits = 0;
			FloatFormat.MaximumFractionalDigits = 2;
			return &FloatFormat;
		};

		CRI_API FNumberFormattingOptions* GetPitchFloatFormat()
		{
			static FNumberFormattingOptions FloatFormat;
			FloatFormat.MinimumIntegralDigits = 1;
			FloatFormat.MaximumIntegralDigits = 3;
			FloatFormat.MinimumFractionalDigits = 3;
			FloatFormat.MaximumFractionalDigits = 3;
			return &FloatFormat;
		};

		CRI_API const FNumberFormattingOptions* GetTimeFormat()
		{
			static FNumberFormattingOptions FloatFormat;
			FloatFormat.MinimumIntegralDigits = 1;
			FloatFormat.MinimumFractionalDigits = 3;
			FloatFormat.MaximumFractionalDigits = 3;
			return &FloatFormat;
		};

		CRI_API FText FormatSecondsAsTime(float InTimeSec);
		CRI_API FText FormatMillisecondsAsTime(float InTimeMS);

		CRI_API FSlateIcon CreateIcon(FName InName)
		{
			return { GetStyleName(), InName };
		}

		CRI_API const FSlateBrush& GetBrushEnsured(FName InName);

	private:

		FSlateStyle();
	};
} // namespace

#undef CRI_API
