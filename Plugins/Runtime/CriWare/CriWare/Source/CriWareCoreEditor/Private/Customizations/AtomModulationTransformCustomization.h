
#pragma once

#include "Curves/SimpleCurve.h"
#include "IPropertyTypeCustomization.h"
#include "Layout/Visibility.h"
#include "Misc/Attribute.h"
#include "PropertyHandle.h"
#include "SCurveEditor.h"
#include "WaveTableTransformLayout.h"


// Forward Declarations
struct FAtomControlModulationInput;
struct FAtomModulationTransform;
class UAtomModulationPatch;

class FAtomModulationTransformCustomization : public WaveTable::Editor::FTransformLayoutCustomizationBase
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<FAtomModulationTransformCustomization>();
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{ }
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{ }

protected:
	//~ Begin FTransformLayoutCustomizationBase
	virtual TSet<EWaveTableCurve> GetSupportedCurves() const override;
	virtual FWaveTableTransform* GetTransform() const override;
	virtual bool IsBipolar() const override;
	//~ End FTransformLayoutCustomizationBase
};
