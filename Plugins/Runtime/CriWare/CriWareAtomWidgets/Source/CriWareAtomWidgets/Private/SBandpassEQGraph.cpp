
#include "SBandpassEQGraph.h"

#include "Rendering/DrawElements.h"
#include "Widgets/SCompoundWidget.h"

#include "Atom/Atom.h"
#include "Atom/AtomBlueprintLibrary.h"

namespace
{
	const int CLICK_RADIUS = 12;
	const float MAX_DECIBEL = 24.0f;
	const float FREQ_RANGE_START = 24.0f;
	const float FREQ_RANGE_END = 24000.0f;

	/* ゲイン最低値を1/65536でクリップ（cridspと合わせた仕様） */
	const double min_linear_gain = 1.0 / 65536.0;
	/* 増幅の下限値を0とせず、-192dBとする。（対数スケール表示することを考慮） */
	const double min_amplitude = FMath::Pow(10.0, -9.6);

	const uint32 biquadMarginWidth = 59;/* Left + Right */
	/* 共鳴が起きる条件 (sqrt(0.5) < Q) の判定に使用する */
}

namespace AtomWidgets
{
	void SBandpassEQGraph::Construct(const FArguments& InArgs)
	{
		bNeedsRepaint = false;
		bIsDragging = false;
		MyGraphPoints.SetNumZeroed(500); // 500点の初期化
	}

	int32 SBandpassEQGraph::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{

		// get graph size
		FVector2D GraphSize = FVector2D(AllottedGeometry.GetLocalSize().X, AllottedGeometry.GetLocalSize().Y);

		// 背景をクリア
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor::Black
		);

		// グラフの描画
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			MyGraphPoints,
			ESlateDrawEffect::None,
			FLinearColor::White,
			true,
			2.0f
		);

		// draw grid lines
		TArray<FVector2D> HorizontalLines;
		TArray<FVector2D> VerticalLines;
		int32 NumHorizontalGridLines = 8;
		int32 NumVerticalGridLines = 10;

		for (uint16 i = 0; i <= NumHorizontalGridLines; ++i)
		{
			float Y = GraphSize.Y * i / NumHorizontalGridLines;
			HorizontalLines.Add(FVector2D(0, Y));
			HorizontalLines.Add(FVector2D(GraphSize.X, Y));
		}

		for (uint16 i = 0; i <= NumVerticalGridLines; ++i)
		{
			float X = GraphSize.X * i / NumVerticalGridLines;
			VerticalLines.Add(FVector2D(X, 0));
			VerticalLines.Add(FVector2D(X, GraphSize.Y));
		}

		// draw horizontal grid lines
		for (uint16 i = 0; i < HorizontalLines.Num(); i += 2)
		{
			TArray<FVector2D> LinePoints = { HorizontalLines[i], HorizontalLines[i + 1] };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				ESlateDrawEffect::None,
				FLinearColor::Gray,
				true,
				1.0f
			);
		}

		// draw vertical grid lines
		for (uint16 i = 0; i < VerticalLines.Num(); i += 2)
		{
			TArray<FVector2D> LinePoints = { VerticalLines[i], VerticalLines[i + 1] };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				ESlateDrawEffect::None,
				FLinearColor::Gray,
				true,
				1.0f
			);
		}

		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}


	FReply SBandpassEQGraph::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			FVector2D ClickedPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			bIsDragging = true;  // ドラッグ開始
			UpdateGraph(ClickedPosition, MyGeometry);  // クリックした座標とAllottedGeometryに基づく更新処理を呼び出す
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply SBandpassEQGraph::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
		{
			bIsDragging = false;  // ドラッグ終了
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply SBandpassEQGraph::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (bIsDragging)
		{
			FVector2D DraggedPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			UpdateGraph(DraggedPosition, MyGeometry);  // ドラッグ中の座標とAllottedGeometryに基づく更新処理を呼び出す
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	void SBandpassEQGraph::UpdateGraph(const FVector2D& ClickedPosition, const FGeometry& AllottedGeometry)
	{
		Points.Reset();

		FVector2D GraphSize = AllottedGeometry.GetLocalSize();
		BiquadParameters params(BiquadType::Peaking, ClickedPosition.X, 1.0f, 18.0f, true);
		double delta_omega = UE_TWO_PI / 48000;

		for (size_t freq_index = 0; freq_index < GraphSize.X; ++freq_index)
		{
			float X = Atom::GetLogFrequencyClamped(freq_index, { 0, GraphSize.X }, { 24, 24000 });

			TArray<BiquadParameters> paramsArray;
			paramsArray.Add(params);
			double Y = getFunctionValue(delta_omega, X, paramsArray);
			double Y2 = 1.0f - ((Y + 24) / 48);
			UE_LOG(LogTemp, Warning, TEXT("%f, %f"), Y, Y2);
			Points.Add(FVector2D(freq_index, Y2 * GraphSize.Y));
		}

		// グラフの再描画
		MyGraphPoints = Points;
		bNeedsRepaint = true;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);

	}

	double SBandpassEQGraph::CalculateBiquadGain(double delta_omega, double freq, const BiquadParameters& params)
	{
		if (!params.enable) {
			return 1.0;
		}
		/* 周波数の無次元化（周波数→ラジアン値） */
		double omega0 = params.featureFreq * delta_omega;
		double omega_dt = freq * delta_omega;
		/* 繰り返し使用する値を変数化 */
		double cos_omega0 = FMath::Cos(omega0);
		/* 係数の導出 */
		/* フィルタタイプに合わせたバイクアッド係数を作成する */
		double alpha = FMath::Sin(omega0) / 2.0 / params.qFactor;
		double a_factor = FMath::Sqrt(FMath::Max(::min_linear_gain, params.linearGain));
		double a0 = 1.0;
		double a1 = 0.0;
		double a2 = 0.0;
		double b0 = 1.0;
		double b1 = 0.0;
		double b2 = 0.0;

		switch (params.bqType)
		{
		case BiquadType::HighPass:
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_omega0;
			a2 = 1.0 - alpha;
			b0 = (1.0 + cos_omega0) / 2.0;
			b1 = -b0 * 2.0;
			b2 = b0;
			break;
		case BiquadType::LowPass:
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_omega0;
			a2 = 1.0 - alpha;
			b0 = (1.0 - cos_omega0) / 2.0;
			b1 = b0 * 2.0;
			b2 = b0;
			break;
		case BiquadType::BandPass:
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_omega0;
			a2 = 1.0 - alpha;
			b0 = alpha;
			b1 = 0.0;
			b2 = -b0;
			break;
		case BiquadType::Notch:
			a0 = 1.0 + alpha;
			a1 = -2.0 * cos_omega0;
			a2 = 1.0 - alpha;
			b0 = 1.0;
			b1 = a1;
			b2 = b0;
			break;
		case BiquadType::Peaking:
			a0 = a_factor + alpha;
			a1 = a_factor * (-2.0 * cos_omega0);
			a2 = a_factor - alpha;
			b0 = a_factor + alpha * a_factor * a_factor;
			b1 = a_factor * (-2.0 * cos_omega0);
			b2 = a_factor - alpha * a_factor * a_factor;
			break;
		case BiquadType::LowShelf:
			a0 = a_factor + 1.0 + (a_factor - 1.0) * cos_omega0 + 2.0 * FMath::Sqrt(a_factor) * alpha;
			a1 = -2.0 * (a_factor - 1.0 + (a_factor + 1.0) * cos_omega0);
			a2 = a_factor + 1.0 + (a_factor - 1.0) * cos_omega0 - 2.0 * FMath::Sqrt(a_factor) * alpha;
			b0 = a_factor * (a_factor + 1.0 - (a_factor - 1.0) * cos_omega0 + 2.0 * FMath::Sqrt(a_factor) * alpha);
			b1 = a_factor * 2.0 * (a_factor - 1.0 - (a_factor + 1.0) * cos_omega0);
			b2 = a_factor * (a_factor + 1.0 - (a_factor - 1.0) * cos_omega0 - 2.0 * FMath::Sqrt(a_factor) * alpha);
			break;
		case BiquadType::HighShelf:
			a0 = a_factor + 1.0 - (a_factor - 1.0) * cos_omega0 + 2.0 * FMath::Sqrt(a_factor) * alpha;
			a1 = 2.0 * (a_factor - 1.0 - (a_factor + 1.0) * cos_omega0);
			a2 = a_factor + 1.0 - (a_factor - 1.0) * cos_omega0 - 2.0 * FMath::Sqrt(a_factor) * alpha;
			b0 = a_factor * (a_factor + 1.0 + (a_factor - 1.0) * cos_omega0 + 2.0 * FMath::Sqrt(a_factor) * alpha);
			b1 = -a_factor * 2.0 * (a_factor - 1.0 + (a_factor + 1.0) * cos_omega0);
			b2 = a_factor * (a_factor + 1.0 + (a_factor - 1.0) * cos_omega0 - 2.0 * FMath::Sqrt(a_factor) * alpha);
			break;
		default:
			break;
		}

		a1 /= a0;
		a2 /= a0;
		b0 /= a0;
		b1 /= a0;
		b2 /= a0;
		/* 利得の導出 */
		/* 入力信号をXin =  1 * sin(2*pi* x * t)とし、*/
		/* 出力信号をXout = A * sin(2*pi* x * t) + B * cos(2*pi* x * t)としたとき、 */
		/* 以下の行列式を得る。 */
		/*
		* |  m1  m2 |   A        v1
		* |         | {   }  = {    }
		* | -m2  m1 |   B        v2
		*/
		/* 上記行列式からA, Bを求め、利得( =sqrt(A^2+B^2) )を計算すると  */
		/* sqrt( (v1^2+v2^2) / (m1^2 + m2^2) )となる。 */
		double m1 = 1.0 + a1 * FMath::Cos(omega_dt) + a2 * FMath::Cos(2.0 * omega_dt);
		double m2 = a1 * FMath::Sin(omega_dt) + a2 * FMath::Sin(2.0 * omega_dt);
		double v1 = b0 + b1 * FMath::Cos(omega_dt) + b2 * FMath::Cos(2.0 * omega_dt);
		double v2 = -b1 * FMath::Sin(omega_dt) - b2 * FMath::Sin(2.0 * omega_dt);
		double M = m1 * m1 + m2 * m2;
		if (M <= 0.0) {
			/* 雑な特異点対策
			* 特定条件を満たすと(カットオフ周波数がサンプル周波数のちょうど半分であり、
			* かつx軸の周波数もカットオフ周波数と全く同じになるとき。
			* 例えばサンプル周波数48k, カットオフ24kとしたときに、x軸右端(x=24k)の一点のみ)ここに来る。
			* （そもそも調べる周波数xはサンプル周波数の半分より小さい値にしなければならない） */
			return 1.0;
		}
		double ret = FMath::Sqrt((v1 * v1 + v2 * v2) / M);
		if (ret < ::min_amplitude) {
			ret = ::min_amplitude;
		}
		return ret;
	}

	double SBandpassEQGraph::getFunctionValue(double delta_omega, double freq, TArray<BiquadParameters> bandParams)
	{
		double y = 1.0;
		for (const SBandpassEQGraph::BiquadParameters& bandParam : bandParams)
		{
			y *= SBandpassEQGraph::CalculateBiquadGain(delta_omega, freq, bandParam);
		}
		return y;
	}
} // namespace
