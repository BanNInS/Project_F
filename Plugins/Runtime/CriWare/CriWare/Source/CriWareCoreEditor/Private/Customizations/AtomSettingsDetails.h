#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

// Forward Declarations
class UAtomRackPropertyHolder_Internal;
class UAtomSounBankPropertyHolder_Internal;
class FSubObjectPicker;
class IDetailGroup;

class CRIWARECOREEDITOR_API FAtomSettingsDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:

	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> RackHolder;

	TSharedPtr<FSubObjectPicker> BusPicker;

	void CustomizeAtomBusProperty(IDetailGroup& CategoryGroup, TSharedRef<IPropertyHandle>& BusHandle, const FText& ParentPropertyDisplayName);
	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> CreateRackPropertyHolder();
};
