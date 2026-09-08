#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

namespace AtomWidgets
{
	class SBandpassEQGraph
		: public SCompoundWidget
	{
	public:

		SLATE_BEGIN_ARGS(SBandpassEQGraph)
			{}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	protected:

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

		// マウスイベントのハンドラ
		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
		FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

		bool bIsDragging;
		FVector2D LastMousePosition;

		void UpdateGraph(const FVector2D& ClickedPosition, const FGeometry& AllottedGeometry);
		TArray<FVector2D> MyGraphPoints;
		bool bNeedsRepaint;
		TArray<FVector2D> Points;
		FVector2D Response;

		enum class BiquadType {
			None,
			LowPass,
			HighPass,
			Notch,
			LowShelf,
			HighShelf,
			BandPass,
			Peaking
		};

		struct BiquadParameters {
			BiquadType bqType;
			double featureFreq;
			double qFactor;
			double linearGain;
			bool enable;
			BiquadParameters(BiquadType bqType, double featureFreq, double qFactor, double linearGain, bool enable)
				: bqType(bqType), featureFreq(featureFreq), qFactor(qFactor), linearGain(linearGain), enable(enable) {}
		};

		static double CalculateBiquadGain(double delta_omega, double freq, const BiquadParameters& params);
		double getFunctionValue(double delta_omega, double freq, TArray<BiquadParameters> bandParams);
	};
} // namespace
