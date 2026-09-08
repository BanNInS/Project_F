/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : CriWareWindowsEditor
* File     : CriWareWindowsSettingsDetails.cpp
*
****************************************************************************/

#include "CriWareWindowsSettingsDetails.h"

#include "Misc/PackageName.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"

#include "CriWareWindowsSettings.h"
#include "CriWare.h"
#include "Customizations/CriWareNameSelector.h"

#if PLATFORM_WINDOWS
#include "Atom/AtomRuntimeWindows.h"
#endif

#define LOCTEXT_NAMESPACE "CriWareWindowsSettingsDetailsInspector"

TSharedRef<IDetailCustomization> FCriWareWindowsSettingsDetails::MakeInstance()
{
	return MakeShareable(new FCriWareWindowsSettingsDetails);
}

void FCriWareWindowsSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Get the currently available audio devices
	TArray<FName> AvailableDeviceNames;

#if PLATFORM_WINDOWS
	if (GCriWare && GCriWare->GetPlatform().Atom())
	{
		if (FAtomRuntime* AtomRuntime = GCriWare->GetCriWareCore()->GetMainAtomRuntimeRaw())
		{
			if (!AtomRuntime->IsUsingUnrealSoundRenderer())
			{
				FAtomRuntimeWindows* AtomRuntimeWindows = static_cast<FAtomRuntimeWindows*>(AtomRuntime);
				AtomRuntimeWindows->GetAvailableAudioDevices(true).GenerateValueArray(AvailableDeviceNames);
			}
		}

		// Add a default entry
		AvailableDeviceNames.Insert(FName(*LOCTEXT("AtomDefaultDeviceLabel", "Default Device").ToString()), 0);
	}
#endif

	// Prepare dropdown menus
	if (!Hardware1DeviceSelectorGenerator)
	{
		Hardware1DeviceSelectorGenerator = MakeShareable(new FCriWareNameSelectorGenerator());
	}
	if (!Hardware2DeviceSelectorGenerator)
	{
		Hardware2DeviceSelectorGenerator = MakeShareable(new FCriWareNameSelectorGenerator());
	}
	if (!Hardware3DeviceSelectorGenerator)
	{
		Hardware3DeviceSelectorGenerator = MakeShareable(new FCriWareNameSelectorGenerator());
	}
	if (!Hardware4DeviceSelectorGenerator)
	{
		Hardware4DeviceSelectorGenerator = MakeShareable(new FCriWareNameSelectorGenerator());
	}

	// Add a group
	IDetailCategoryBuilder& AtomCategory = DetailLayout.EditCategory(TEXT("Atom"));
	IDetailGroup& SoundRendererGroup = AtomCategory.AddGroup("Sound Renderer", LOCTEXT("SoundRendererLabel", "Sound Renderer"));
	IDetailGroup& HardwareDevicesGroup = SoundRendererGroup.AddGroup(TEXT("Hardware Devices"), LOCTEXT("HardwareDevicesLabel", "Hardware Devices"));

	// Put existing property from sub-category to group (UE does not support edition of sub-categories).
	TSharedPtr<IPropertyHandle> MappingsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, EndpointMappings));
	check(MappingsHandle);
	SoundRendererGroup.AddPropertyRow(MappingsHandle.ToSharedRef());

	// Arrays need special handling and will create an array builder
	//TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(new FDetailArrayBuilder(MappingsHandle.ToSharedRef()));

	// Setup properties customizations
	auto AddHardwareProperty = [&](const FName& PropertyPath, TSharedPtr<FCriWareNameSelectorGenerator>& HardwareDeviceSelectorGenerator)
	{
		TSharedPtr<IPropertyHandle> HardwareHandle = DetailLayout.GetProperty(PropertyPath);
		check(HardwareHandle);
		IDetailPropertyRow& HardwarePropertyRow = HardwareDevicesGroup.AddPropertyRow(HardwareHandle.ToSharedRef());

		// Set up callbacks for the name selector dropdown menu:
		FCriWareNameSelectorGenerator::FCriWareNameSelectorCallbacks Callbacks =
		{
			// void OnNameChanged(FName InName)
			[HardwareHandle](FName SelectedName)
			{
				if (HardwareHandle.IsValid() && HardwareHandle->IsValidHandle())
				{
					HardwareHandle->SetValue(SelectedName);
				}
			},
			// FName GetCurrentName()
			[HardwareHandle]()
			{
				FName SelectedName;

				if (HardwareHandle.IsValid() && HardwareHandle->IsValidHandle())
				{
					HardwareHandle->GetValue(SelectedName);
				}

				return FName(SelectedName.ToString().TrimEnd());
			},
			// FString GetTooltipText()
			[]()
			{
				return FString(TEXT("Use this to assign the audio device of this hardware endpoint."));
			}
		};

		HardwarePropertyRow.CustomWidget()
			.NameContent()
			[
				HardwareHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MaxDesiredWidth(1000.0f)
			.MinDesiredWidth(50.0f)
			[
				HardwareDeviceSelectorGenerator->MakeNameSelectorWidget(AvailableDeviceNames, MoveTemp(Callbacks))
			];
	};

	AddHardwareProperty(GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, Hardware1), Hardware1DeviceSelectorGenerator);
	AddHardwareProperty(GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, Hardware2), Hardware2DeviceSelectorGenerator);
	AddHardwareProperty(GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, Hardware3), Hardware3DeviceSelectorGenerator);
	AddHardwareProperty(GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, Hardware4), Hardware4DeviceSelectorGenerator);

	// plugins

	AtomPluginWidgetManager.BuildAtomCategory(DetailLayout, TEXT("Windows"));
}

#undef LOCTEXT_NAMESPACE
