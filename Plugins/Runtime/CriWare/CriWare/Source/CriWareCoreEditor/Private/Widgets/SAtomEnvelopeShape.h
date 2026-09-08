#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"
#include "PropertyHandle.h"

// Forward Declarations
enum class EAtomEnvelopeCurve : uint8;

class SAtomEnvelopeShape : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAtomEnvelopeShape)
		: _LineColor(FLinearColor::White)
		, _LabelColor(FLinearColor::White)
		, _IsShowingSegments(false)
		, _ReleaseExponent(5.0)
	{}
		/** */
		SLATE_ARGUMENT(FLinearColor, LineColor)
		
		/** */
		SLATE_ARGUMENT(FLinearColor, LabelColor)
		
		/** */
		SLATE_ATTRIBUTE(bool, IsShowingSegments)

		/** */
		SLATE_ATTRIBUTE(float, Attack)

		/** */
		SLATE_ATTRIBUTE(float, Hold)

		/** */
		SLATE_ATTRIBUTE(float, Decay)
		
		/** */
		SLATE_ATTRIBUTE(float, Sustain)
		
		/** */
		SLATE_ATTRIBUTE(float, Release)

		/** */
		SLATE_ARGUMENT(float, ReleaseExponent)

		/** */
		SLATE_ATTRIBUTE(float, AttackCurveStrength)

		/** */
		SLATE_ATTRIBUTE(float, DecayCurveStrength)

		/** */
		SLATE_ATTRIBUTE(float, ReleaseCurveStrength)

		/** */
		SLATE_ATTRIBUTE(EAtomEnvelopeCurve, AttackCurve)
		
		/** */
		SLATE_ATTRIBUTE(EAtomEnvelopeCurve, DecayCurve)

		/** */
		SLATE_ATTRIBUTE(EAtomEnvelopeCurve, ReleaseCurve)

	SLATE_END_ARGS()

	SAtomEnvelopeShape();
	~SAtomEnvelopeShape();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScale) const override { return FVector2D(200.0f, 100.0f); }
	
private:

	FLinearColor LineColor;
	FLinearColor LabelColor;
	TAttribute<bool> IsShowingSegments;
	TAttribute<float> Attack;
	TAttribute<float> Hold;
	TAttribute<float> Decay;
	TAttribute<float> Sustain;
	TAttribute<float> Release;
	float ReleaseExponent;
	TAttribute<float> AttackCurveStrength;
	TAttribute<float> DecayCurveStrength;
	TAttribute<float> ReleaseCurveStrength;
	TAttribute<EAtomEnvelopeCurve> AttackCurve;
	TAttribute<EAtomEnvelopeCurve> DecayCurve;
	TAttribute<EAtomEnvelopeCurve> ReleaseCurve;

	struct FTextLabelInfo
	{
		const FText Text;
		const FVector2D LabelSize;
	};

	TArray<FTextLabelInfo> LabelInfos;
};
