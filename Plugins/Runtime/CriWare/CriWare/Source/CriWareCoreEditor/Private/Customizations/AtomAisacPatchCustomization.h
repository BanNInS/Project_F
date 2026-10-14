/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacPatchCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Customizations/SubObjectPicker.h"

/** Customization for AtomAisacPatch properties. */
class FAtomAisacPatchCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

private:

	/** local rack. */
	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> ConfigHolder;

	/** AtomAisacPatch being edited. */
	TSharedPtr<IPropertyHandle> PatchHandle;

	TSharedPtr<FSubObjectPicker> PatchPicker;

	bool CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle);
	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> CreateConfigPropertyHolder();
};
