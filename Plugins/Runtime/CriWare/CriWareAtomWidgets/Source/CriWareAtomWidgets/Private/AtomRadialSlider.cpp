
#include "AtomRadialSlider.h"

#include "SAtomRadialSlider.h"

#define LOCTEXT_NAMESPACE "AtomRadialSlider"

UAtomRadialSlider::UAtomRadialSlider(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UAtomRadialSlider::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<float> ValueBinding = PROPERTY_BINDING(float, Value);

	if (AtomRadialSlider.IsValid())
	{
		AtomRadialSlider->SetValue(ValueBinding);
	}
}

TSharedRef<SWidget> UAtomRadialSlider::RebuildWidget()
{
	return SAssignNew(AtomRadialSlider, AtomWidgets::SAtomRadialSlider)
		.Value(Value)
		.Name(LOCTEXT("DefaultParameterName", "Default Parameter"));
}

#undef LOCTEXT_NAMESPACE
