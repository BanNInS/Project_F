/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : CriWareWindowsEditor
* File     : CriWareWindowsSettingsDetails.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Customizations/AtomTargetPlatformCustomization.h"

// Forward Declarations
class IDetailLayoutBuilder;
class FCriWareNameSelectorGenerator;

class CRIWAREWINDOWSEDITOR_API FCriWareWindowsSettingsDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:

	TSharedPtr<FCriWareNameSelectorGenerator> Hardware1DeviceSelectorGenerator;
	TSharedPtr<FCriWareNameSelectorGenerator> Hardware2DeviceSelectorGenerator;
	TSharedPtr<FCriWareNameSelectorGenerator> Hardware3DeviceSelectorGenerator;
	TSharedPtr<FCriWareNameSelectorGenerator> Hardware4DeviceSelectorGenerator;

	FAtomPluginWidgetManager AtomPluginWidgetManager;
};
