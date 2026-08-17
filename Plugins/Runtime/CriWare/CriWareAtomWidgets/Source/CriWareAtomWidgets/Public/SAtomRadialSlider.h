#pragma once

#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

// Forward Declarations
class SRadialSlider;

namespace AtomWidgets
{
	class CRIWAREATOMWIDGETS_API SAtomRadialSlider
		: public SCompoundWidget
	{
	public:

		SLATE_BEGIN_ARGS(SAtomRadialSlider)
			: _Name(FText::FromString("Title"))
			, _Value(0.0f)
			, _Range({ 0.0f, 1.0f })
			, _Scale(1.0f)
			, _OnValueChanged(nullptr)

			{}

			SLATE_ATTRIBUTE(FText, Name)
			SLATE_ATTRIBUTE(float, Value)
			SLATE_ARGUMENT(FVector2D, Range)
			SLATE_ARGUMENT(float, Scale)
			SLATE_EVENT(FOnFloatValueChanged, OnValueChanged)

		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		void SetValue(const TAttribute<float>& InValue);

		float GetValue() const { return Value.Get(); }

		void SetTextValue(FText InText);

	private:

		TAttribute<float> Value;

		TSharedPtr<SRadialSlider> Radial;

		TSharedPtr<STextBlock> Text;

		FOnFloatValueChanged OnValueChanged;
	};
} // namespace
