#include "AtomEnvelopeCustomization.h"

#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"

#include "Atom/AtomEnvelope.h"

#include "Widgets/SAtomEnvelopeShape.h"

#define LOCTEXT_NAMESPACE "AtomEnvelopeCustomization"

/* FAtomEnvelopeCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomEnvelopeCustomization::MakeInstance()
{
	return MakeShareable(new FAtomEnvelopeCustomization);
}

void FAtomEnvelopeCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
}

void FAtomEnvelopeCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}

	// Get envelope parameters property handles.
	TSharedPtr<IPropertyHandle> AttackPropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, AttackTime));
	TSharedPtr<IPropertyHandle> HoldPropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, HoldTime));
	TSharedPtr<IPropertyHandle> DecayPropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, DecayTime));
	TSharedPtr<IPropertyHandle> SustainPropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, SustainLevel));
	TSharedPtr<IPropertyHandle> ReleasePropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, ReleaseTime));

	// Add a group for the sahpe graph.
	auto& ShapeGroup = ChildBuilder.AddGroup(TEXT("AtomEnvelopeShapeGraph"), LOCTEXT("ShapeGraph", "Shape Graph"));
	
	// Show the group header with its legend option.
	ShapeGroup.HeaderRow()
	.NameContent()
	[
		SNew(STextBlock)
		.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(LOCTEXT("ShapeGraph", "Shape Graph"))
		.ToolTipText(LOCTEXT("EnvelopeShapeTooltip", "Show the envelope shape."))
	]
	.ValueContent()
	[
		SAssignNew(ShowSegmentsCheckBox, SCheckBox)
		.Visibility_Lambda([&ShapeGroup]() { return ShapeGroup.GetExpansionState() ? EVisibility::Visible : EVisibility::Hidden; })
		.IsChecked(false)
		.ToolTipText(LOCTEXT("ShowEnvelopeSegments", "Show Segments"))
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
			.Text(LOCTEXT("ShowEnvelopeSegments", "Show Segments"))
			.ToolTipText(LOCTEXT("ShowEnvelopeSegments", "Show Segments"))
		]
	];

	// Add a widget row for the shape graph.
	ShapeGroup.AddWidgetRow()
		[
			SNew(SAtomEnvelopeShape)
			.LineColor(FColor::Yellow)
		.IsShowingSegments(this, &FAtomEnvelopeCustomization::IsShowSegments)
		.Attack(this, &FAtomEnvelopeCustomization::GetParameterValue, AttackPropertyHandle.ToSharedRef())
		.Hold(this, &FAtomEnvelopeCustomization::GetParameterValue, HoldPropertyHandle.ToSharedRef())
		.Decay(this, &FAtomEnvelopeCustomization::GetParameterValue, DecayPropertyHandle.ToSharedRef())
		.Sustain(this, &FAtomEnvelopeCustomization::GetParameterValue, SustainPropertyHandle.ToSharedRef())
		.Release(this, &FAtomEnvelopeCustomization::GetParameterValue, ReleasePropertyHandle.ToSharedRef())
		.ReleaseExponent(ReleasePropertyHandle->GetFloatMetaData("SliderExponent"))
	];
	
	// show properties as they are
	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; Index++)
	{
		TSharedPtr<IPropertyHandle> ChildProperty = PropertyHandle->GetChildHandle(Index);
		if (ChildProperty->IsValidHandle())
		{
			ChildBuilder.AddProperty(ChildProperty.ToSharedRef());
		}
	}
}

bool FAtomEnvelopeCustomization::IsShowSegments() const
{
	if (ShowSegmentsCheckBox.IsValid())
	{
		return ShowSegmentsCheckBox->IsChecked();
	}

	return false;
}

float FAtomEnvelopeCustomization::GetParameterValue(TSharedRef<IPropertyHandle> ParameterProperty) const
{
	if (ParameterProperty->IsValidHandle())
	{
		float Value;
		ParameterProperty->GetValue(Value);
		return Value;
	}

	return 0.0f;
}

#undef LOCTEXT_NAMESPACE
