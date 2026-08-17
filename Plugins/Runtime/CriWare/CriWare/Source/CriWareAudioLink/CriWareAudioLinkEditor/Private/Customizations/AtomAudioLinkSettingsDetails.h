#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"
#include "UObject/StrongObjectPtr.h"
#include "Customizations/SubObjectPicker.h"

class CRIWARECOREEDITOR_API FAtomAudioLinkSettingsDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

private:

	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> CreateRackPropertyHolder();

	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> RackHolder;
	TSharedPtr<FSubObjectPicker> BusPicker;
};
