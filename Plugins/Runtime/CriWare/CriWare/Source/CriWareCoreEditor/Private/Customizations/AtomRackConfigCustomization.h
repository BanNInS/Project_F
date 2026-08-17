/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackConfigCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "IDetailCustomization.h"

class FAtomRackConfigCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

private:

	TSharedRef<SWidget> CreateInlinedPropertyWidget(TSharedRef<IPropertyHandle> Handle) const;
	TSharedRef<SWidget> CreatePropertyOuputPortNameWidget(FName PortName) const;
	TAttribute<bool> GetIsOuputPortEditableAttribute(FName PortName) const;

	TSharedPtr<IPropertyHandle> bOverrideDefaultPortsHandle;
};
