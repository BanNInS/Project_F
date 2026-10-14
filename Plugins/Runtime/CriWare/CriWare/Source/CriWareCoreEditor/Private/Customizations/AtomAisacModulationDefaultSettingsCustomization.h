/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacModulationDefaultSettingsCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Customizations/SubObjectPicker.h"

/** Customization for AtomAisacModulationDefaultSettings properties. */
class FAtomAisacModulationDefaultSettingsCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

private:

	void GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> CreateConfigPropertyHolder();
	bool CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle);

	/** AtomConfig for AISAC patch selection. */
	TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> ConfigHolder;
	TSharedPtr<IPropertyHandle> ConfigHandle;

	/** AtomAisacPatch array being edited. */
	TSharedPtr<IPropertyHandle> AdditionalAisacPatchesHandle;

	/** Array of Patch asset picker being edited. */
	TArray<TSharedPtr<FSubObjectPicker>> AdditionalAisacPatchPickers;
};
