#include "SAtomEnvelopeShape.h"

#include "Fonts/FontMeasure.h"

#include "Atom/AtomEnvelope.h"

#define LOCTEXT_NAMESPACE "AtomEnvelopeShape"

#define ATOM_ENVELOPE_ATTACKTIME_MAX	2000.0f
#define ATOM_ENVELOPE_HOLDTIME_MAX		2000.0f
#define ATOM_ENVELOPE_DECAYTIME_MAX		2000.0f
#define ATOM_ENVELOPE_RELEASETIME_MAX	10000.0f

bool AreTextsOverlapping(const FVector2D& APos, const FVector2D& ASize, const FVector2D& BPos, const FVector2D& BSize)
{
	return FSlateRect::DoRectanglesIntersect(
		FSlateRect(APos.X - ASize.X / 2.0, 0.0, APos.X + ASize.X / 2.0, 0.0),
		FSlateRect(BPos.X - BSize.X / 2.0, 0.0, BPos.X + BSize.X / 2.0, 0.0)
	);
}

SAtomEnvelopeShape::SAtomEnvelopeShape()
{
	// Initalize legend labels.
	const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const FText AttackText = LOCTEXT("EnvelopeAttackLabel", "Attack");
	const FText HoldText = LOCTEXT("EnvelopeHoldLabel", "Hold");
	const FText DecayText = LOCTEXT("EnvelopeAttackLabel", "Decay");
	const FText SustainText = LOCTEXT("EnvelopeAttackLabel", "Sustain");
	const FText ReleaseText = LOCTEXT("EnvelopeAttackLabel", "Release");

	// We have to measure the string so we can draw it centered. 
	LabelInfos.Add(FTextLabelInfo{ AttackText, FontMeasure->Measure(AttackText, FontInfo) });
	LabelInfos.Add(FTextLabelInfo{ HoldText, FontMeasure->Measure(HoldText, FontInfo) });
	LabelInfos.Add(FTextLabelInfo{ DecayText, FontMeasure->Measure(DecayText, FontInfo) });
	LabelInfos.Add(FTextLabelInfo{ SustainText, FontMeasure->Measure(SustainText, FontInfo) });
	LabelInfos.Add(FTextLabelInfo{ ReleaseText, FontMeasure->Measure(ReleaseText, FontInfo) });
}

SAtomEnvelopeShape::~SAtomEnvelopeShape()
{}

void SAtomEnvelopeShape::Construct(const FArguments& InArgs)
{
	LineColor = InArgs._LineColor;
	LabelColor = InArgs._LabelColor;
	IsShowingSegments = InArgs._IsShowingSegments;
	Attack = InArgs._Attack;
	Hold = InArgs._Hold;
	Decay = InArgs._Decay;
	Sustain = InArgs._Sustain;
	Release = InArgs._Release;

	if (InArgs._ReleaseExponent > SMALL_NUMBER)
	{
		ReleaseExponent = InArgs._ReleaseExponent;
	}

	AttackCurve = InArgs._AttackCurve;
	DecayCurve = InArgs._DecayCurve;
	ReleaseCurve = InArgs._ReleaseCurve;
	AttackCurveStrength = InArgs._AttackCurveStrength;
	DecayCurveStrength = InArgs._DecayCurveStrength;
	ReleaseCurveStrength = InArgs._ReleaseCurveStrength;
}

int32 SAtomEnvelopeShape::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Darken the background so our text stands out more
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FAppStyle::GetBrush(TEXT("WhiteBrush")),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.75f)
	);

	//FLinearColor LinearColor = GetColorAndOpacity() * InWidgetStyle.GetColorAndOpacityTint();
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const bool bShowSegments = IsShowingSegments.Get();

	const double BorderSize = 10.0;
	const double Width = Size.X - (BorderSize * 2.0);
	const double Height = Size.Y - (BorderSize * 2.0) - (bShowSegments ? LabelInfos[0].LabelSize.Y + 4.0 : 0.0);
	const double SectionSize = Width / 5.0;
	
	const float AttackScale = FMath::Clamp(Attack.Get(), 0.0f, ATOM_ENVELOPE_ATTACKTIME_MAX) / ATOM_ENVELOPE_ATTACKTIME_MAX;
	const float HoldScale = FMath::Clamp(Hold.Get(), 0.0f, ATOM_ENVELOPE_HOLDTIME_MAX) / ATOM_ENVELOPE_HOLDTIME_MAX;
	const float DecayScale = FMath::Clamp(Decay.Get(), 0.0f, ATOM_ENVELOPE_DECAYTIME_MAX) / ATOM_ENVELOPE_DECAYTIME_MAX;
	const float SustainScale = FMath::Clamp(Sustain.Get(), 0.0f, 1.0f);
	const float ReleaseScale = 1.0f - FMath::Pow(1.0f - (FMath::Clamp(Release.Get(), 0.0f, ATOM_ENVELOPE_RELEASETIME_MAX) / ATOM_ENVELOPE_RELEASETIME_MAX), ReleaseExponent);

	/*TArray< FVector2D > LinePoints;
	LinePoints.AddUninitialized(7);

	LinePoints[0] = FVector2D(BorderSize, BorderSize + Height); // origin
	LinePoints[1] = FVector2D(BorderSize + (AttackScale * SectionSize),	BorderSize); // Attack 
	LinePoints[2] = FVector2D(LinePoints[1].X + (HoldScale * SectionSize), BorderSize); // Hold
	LinePoints[3] = FVector2D(LinePoints[2].X + (DecayScale * SectionSize), BorderSize + Height - (SustainScale * Height)); // Decay
	LinePoints[4] = FVector2D(BorderSize + (4 * SectionSize), BorderSize + Height - (SustainScale * Height)); // Sustain
	LinePoints[5] = FVector2D(LinePoints[4].X + (ReleaseScale * SectionSize), BorderSize + Height); // Release
	LinePoints[6] = FVector2D(BorderSize + Width, BorderSize + Height); // end

	// Draw lines
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId++,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		ESlateDrawEffect::None,
		LineColor,
		false, // anti-alias
		2.0f // tickness
	);*/

	TArray< FVector2f > Points;
	Points.AddUninitialized(7);

	Points[0] = FVector2f(BorderSize, BorderSize + Height); // origin
	Points[1] = FVector2f(BorderSize + (AttackScale * SectionSize), BorderSize); // Attack 
	Points[2] = FVector2f(Points[1].X + (HoldScale * SectionSize), BorderSize); // Hold
	Points[3] = FVector2f(Points[2].X + (DecayScale * SectionSize), BorderSize + Height - (SustainScale * Height)); // Decay
	Points[4] = FVector2f(BorderSize + (4 * SectionSize), BorderSize + Height - (SustainScale * Height)); // Sustain
	Points[5] = FVector2f(Points[4].X + (ReleaseScale * SectionSize), BorderSize + Height); // Release
	Points[6] = FVector2f(BorderSize + Width, BorderSize + Height); // end

	// Draw Curves 
	// 
	auto MakeCurve = [&](EAtomEnvelopeCurve Curve, float Strength, FVector2f Start, FVector2f End)
		{
			float DiffX = End.X - Start.X;
			float DiffY = End.Y - Start.Y;

			FVector2f StartDir = FVector2f(DiffX, DiffY);
			FVector2f EndDir = FVector2f(DiffX, DiffY);

			switch (Curve)
			{
			case EAtomEnvelopeCurve::Linear:
				break;
			case EAtomEnvelopeCurve::Exponential: // Square
			{
				StartDir = FVector2f(0.0f, 0.0f);
				EndDir = FVector2f(Strength * DiffX, 0.0f);
				break;
			}
			case EAtomEnvelopeCurve::Logarithmic: // Inv Square
			{
				StartDir = FVector2f(Strength * DiffX, 0.0f);
				EndDir = FVector2f(0.0f, 0.0f);
				break;
			}
			case EAtomEnvelopeCurve::SCurve: // Sin
			{
				StartDir = EndDir = FVector2f(Strength * DiffX, 0.0f);
				break;
			}
			case EAtomEnvelopeCurve::InvertedSCurve: // Sin
			{
				StartDir = EndDir = FVector2f(0.0f, Strength * DiffY);
				break;
			}
			}

			FSlateDrawElement::MakeSpline(
				OutDrawElements,
				LayerId++,
				AllottedGeometry.ToPaintGeometry(),
				Start, // start
				StartDir, // start dir
				End, // end
				EndDir, // end dir
				2.0f, // tickness
				ESlateDrawEffect::None,
				LineColor // tint
			);
		};

	MakeCurve(AttackCurve.Get(), AttackCurveStrength.Get(), Points[0], Points[1]);	// attack
	MakeCurve(EAtomEnvelopeCurve::Linear, 0.0f, Points[1], Points[2]);				// hold
	MakeCurve(DecayCurve.Get(), DecayCurveStrength.Get(), Points[2], Points[3]);	// decay
	MakeCurve(EAtomEnvelopeCurve::Linear, 0.0f, Points[3], Points[4]);				// sustain
	MakeCurve(ReleaseCurve.Get(), ReleaseCurveStrength.Get(), Points[4], Points[5]);// release
	MakeCurve(EAtomEnvelopeCurve::Linear, 0.0f, Points[5], Points[6]);				// end spacing

	// Draw dots
	const FVector2D DotSize(4.0);

	for (int Index = 0; Index < Points.Num(); Index++)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(DotSize, FSlateLayoutTransform(FVector2D(Points[Index]) - DotSize / 2.0)),
			FAppStyle::GetBrush(TEXT("WhiteBrush")),
			ESlateDrawEffect::None,
			LineColor
		);
	}

	LayerId++;

	// Draw some text helping the user how to use envelope parameters.
	if (bShowSegments)
	{
		const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");
		const double TextY = Size.Y - BorderSize - (LabelInfos[0].LabelSize.Y / 2.0);

		FVector2D AttackTextPos(Points[0].X + ((Points[1].X - Points[0].X) / 2.0) - (LabelInfos[0].LabelSize.X / 2.0), TextY);
		FVector2D HoldTextPos(Points[1].X + ((Points[2].X - Points[1].X) / 2.0) - (LabelInfos[1].LabelSize.X / 2.0), TextY);
		FVector2D DecayTextPos(Points[2].X + ((Points[3].X - Points[2].X) / 2.0) - (LabelInfos[2].LabelSize.X / 2.0), TextY);
		FVector2D SustainTextPos(Points[3].X + ((Points[4].X - Points[3].X) / 2.0) - (LabelInfos[3].LabelSize.X / 2.0), TextY);
		FVector2D ReleaseTextPos(Points[4].X + ((Points[6].X - Points[4].X) / 2.0) - (LabelInfos[4].LabelSize.X / 2.0), TextY);

		// Try to avoid texts overlapping.
		if (AreTextsOverlapping(SustainTextPos, LabelInfos[3].LabelSize, ReleaseTextPos, LabelInfos[4].LabelSize))
		{
			SustainTextPos.Y -= LabelInfos[0].LabelSize.Y + 2.0;
		}
		if (AreTextsOverlapping(DecayTextPos, LabelInfos[2].LabelSize, SustainTextPos, LabelInfos[3].LabelSize))
		{
			DecayTextPos.Y = SustainTextPos.Y - (LabelInfos[0].LabelSize.Y + 2.0);
		}
		if (AreTextsOverlapping(HoldTextPos, LabelInfos[1].LabelSize, DecayTextPos, LabelInfos[2].LabelSize))
		{
			HoldTextPos.Y = DecayTextPos.Y - (LabelInfos[0].LabelSize.Y + 2.0);
		}
		if (AreTextsOverlapping(AttackTextPos, LabelInfos[0].LabelSize, HoldTextPos, LabelInfos[1].LabelSize))
		{
			AttackTextPos.Y = HoldTextPos.Y - (LabelInfos[0].LabelSize.Y + 2.0);
		}

		// Draw segmentlines
		
		// Draw segment texts.
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(AttackTextPos)),
			LabelInfos[0].Text,
			FontInfo,
			ESlateDrawEffect::None,
			LabelColor
		);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(HoldTextPos)),
			LabelInfos[1].Text,
			FontInfo,
			ESlateDrawEffect::None,
			LabelColor
		);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(DecayTextPos)),
			LabelInfos[2].Text,
			FontInfo,
			ESlateDrawEffect::None,
			LabelColor
		);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(SustainTextPos)),
			LabelInfos[3].Text,
			FontInfo,
			ESlateDrawEffect::None,
			LabelColor
		);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(ReleaseTextPos)),
			LabelInfos[4].Text,
			FontInfo,
			ESlateDrawEffect::None,
			LabelColor
		);

		LayerId++;
	}

	return LayerId;
}

#undef LOCTEXT_NAMESPACE
