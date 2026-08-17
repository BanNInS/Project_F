/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Customizations/SubObjectPicker.h"

/** Customization for AtomBus properties. */
class FAtomBusCustomization
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
	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> RackHolder;

	/** AtomBus being edited. */
	TSharedPtr<IPropertyHandle> BusHandle;

	TSharedPtr<FSubObjectPicker> BusPicker;

	bool CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle);
	TStrongObjectPtr<UAtomRackPropertyHolder_Internal> CreateRackPropertyHolder();
};
