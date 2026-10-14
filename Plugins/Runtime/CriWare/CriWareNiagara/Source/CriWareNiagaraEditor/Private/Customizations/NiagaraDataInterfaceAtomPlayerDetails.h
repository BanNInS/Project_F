#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

// Forward Declarations
class UAtomConfigPropertyHolder_Internal;
class FSubObjectPicker;

class CRIWARENIAGARAEDITOR_API FNiagaraDataInterfaceAtomPlayerDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization interface

private:

	void GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);
	void RebuildAisacPatchArray();

	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> CreateConfigPropertyHolder();

private:

	/** AtomConfig for AISAC patch selection. */
	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> ConfigHolder;

	/** AtomAisacPatch array being edited. */
	TSharedPtr<IPropertyHandle> AdditionalAisacPatchesHandle;

	/** Array of Patch asset picker being edited. */
	TArray<TSharedPtr<FSubObjectPicker>> AdditionalAisacPatchPickers;
};
