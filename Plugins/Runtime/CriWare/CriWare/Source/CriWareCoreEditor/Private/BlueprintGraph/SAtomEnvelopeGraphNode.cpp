
#include "SAtomEnvelopeGraphNode.h"

#include "NodeFactory.h"
#include "EditorStyleSet.h"
#include "SGraphPin.h"

#include "Atom/AtomEnvelope.h"

#include "Widgets/SAtomEnvelopeShape.h"

#define LOCTEXT_NAMESPACE "SAtomEnvelopeGraphNode"

namespace
{
	static const FName AttackPinName(TEXT("Attack Time"));
	static const FName HoldPinName(TEXT("Hold Time"));
	static const FName DecayPinName(TEXT("Decay Time"));
	static const FName SustainPinName(TEXT("Sustain Level"));
	static const FName ReleasePinName(TEXT("Release Time"));

	static const FName AttackCurvePinName(TEXT("Attack Curve"));
	static const FName AttackCurveStrengthPinName(TEXT("Attack Curve Strength"));
	static const FName DecayCurvePinName(TEXT("Decay Curve"));
	static const FName DecayCurveStrengthPinName(TEXT("Decay Curve Strength"));
	static const FName ReleaseCurvePinName(TEXT("Release Curve"));
	static const FName ReleaseCurveStrengthPinName(TEXT("Release Curve Strength"));
}

void SGraphNodeMakeAtomEnvelope::Construct(const FArguments& InArgs, UK2Node_MakeAtomEnvelope* InNode)
{
    GraphNode = InNode;
    UpdateGraphNode();
}

TSharedRef<SWidget> SGraphNodeMakeAtomEnvelope::CreateNodeContentArea()
{
	UK2Node_MakeAtomEnvelope* MakeAtomEnvelopeNode = CastChecked<UK2Node_MakeAtomEnvelope>(GraphNode);

    const UEdGraphPin* AttackPin = GraphNode->FindPin(AttackPinName);
    const UEdGraphPin* HoldPin = GraphNode->FindPin(HoldPinName);
    const UEdGraphPin* DecayPin = GraphNode->FindPin(DecayPinName);
    const UEdGraphPin* SustainPin = GraphNode->FindPin(SustainPinName);
    const UEdGraphPin* ReleasePin = GraphNode->FindPin(ReleasePinName);

	const UEdGraphPin* AttackCurvePin = GraphNode->FindPin(AttackCurvePinName);
	const UEdGraphPin* AttackCurveStrengthPin = GraphNode->FindPin(AttackCurveStrengthPinName);
	const UEdGraphPin* DecayCurvePin = GraphNode->FindPin(DecayCurvePinName);
	const UEdGraphPin* DecayCurveStrengthPin = GraphNode->FindPin(DecayCurveStrengthPinName);
	const UEdGraphPin* ReleaseCurvePin = GraphNode->FindPin(ReleaseCurvePinName);
	const UEdGraphPin* ReleaseCurveStrengthPin = GraphNode->FindPin(ReleaseCurveStrengthPinName);

	TSharedRef<SWidget> FullArea = SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 6.0f, 6.0f, 0.0f))
		[
			SNew(SAtomEnvelopeShape)
			.Visibility_UObject(MakeAtomEnvelopeNode, &UK2Node_MakeAtomEnvelope::GetShapeVisibility)
			.LineColor(MakeAtomEnvelopeNode->GetShapeColor())
			.IsShowingSegments(false)
			.Attack(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, AttackPin)
			.Hold(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, HoldPin)
			.Decay(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, DecayPin)
			.Sustain(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, SustainPin)
			.Release(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, ReleasePin)
			.ReleaseExponent(5.0f)
			.AttackCurve(this, &SGraphNodeMakeAtomEnvelope::GetParameterCurve, AttackCurvePin)
			.AttackCurveStrength(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, AttackCurveStrengthPin)
			.DecayCurve(this, &SGraphNodeMakeAtomEnvelope::GetParameterCurve, DecayCurvePin)
			.DecayCurveStrength(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, DecayCurveStrengthPin)
			.ReleaseCurve(this, &SGraphNodeMakeAtomEnvelope::GetParameterCurve, ReleaseCurvePin)
			.ReleaseCurveStrength(this, &SGraphNodeMakeAtomEnvelope::GetParameterValue, ReleaseCurveStrengthPin)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SGraphNodeK2Base::CreateNodeContentArea()
		];

	return FullArea;
}

float SGraphNodeMakeAtomEnvelope::GetParameterValue(const UEdGraphPin* Pin) const
{
    return Pin ? FCString::Atof(*Pin->DefaultValue) : 0.0f;
}

EAtomEnvelopeCurve SGraphNodeMakeAtomEnvelope::GetParameterCurve(const UEdGraphPin* Pin) const
{
	UEnum* Enum = StaticEnum<EAtomEnvelopeCurve>();
	FName TypeName = Pin ? FName(*Pin->DefaultValue) : NAME_None;
	return static_cast<EAtomEnvelopeCurve>(Enum->GetValueByName(TypeName));
}

#undef LOCTEXT_NAMESPACE
