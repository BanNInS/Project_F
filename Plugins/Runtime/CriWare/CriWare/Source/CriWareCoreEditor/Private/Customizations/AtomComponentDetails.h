/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomComponentDetails.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

#include "Customizations/SubObjectPicker.h"

 // Forward Declarations
class UAtomConfigPropertyHolder_Internal;

class CRIWARECOREEDITOR_API FAtomComponentDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

private:

	//void GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> CreateConfigPropertyHolder();
	bool CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle);

	bool IsSoundSelected(UClass* Class) const;

private:

	/** The sound property. */
	TSharedPtr<IPropertyHandle> SoundHandle;

	/** AtomConfig for AISAC patch selection. */
	//TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> ConfigHolder;
	//TSharedPtr<IPropertyHandle> ConfigHandle;

	/** AtomAisacPatch array being edited. */
	//TSharedPtr<IPropertyHandle> AdditionalAisacPatchesHandle;

	/** Array of Patch asset picker being edited. */
	//TArray<TSharedPtr<FSubObjectPicker>> AdditionalAisacPatchPickers;
};
