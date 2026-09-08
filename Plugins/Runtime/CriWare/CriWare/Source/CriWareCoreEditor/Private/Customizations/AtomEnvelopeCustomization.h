#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"

// Forward Declarations
class SCheckBox;

class FAtomEnvelopeCustomization : public IPropertyTypeCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

	bool IsShowSegments() const;
	float GetParameterValue(TSharedRef<IPropertyHandle> ParameterProperty) const;

	TSharedPtr<IPropertyHandle> EnvelopePropertyHandle;
	TSharedPtr<SCheckBox>	ShowSegmentsCheckBox;
};
