/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomDspBusSettingCustomization.cpp
 *
 ****************************************************************************/

#include "AtomDspBusSettingCustomization.h"

#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "ISinglePropertyView.h"
#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"

#include "Atom/Atom.h"
#include "Atom/AtomConfig.h"

#define LOCTEXT_NAMESPACE "AtomDspBusSetting"

namespace AtomDspBusSettingUtils
{
	bool IsPropertyValueChildOf(TSharedPtr<IPropertyHandle> InProp, const UStruct* Class)
	{
		if (InProp.IsValid())
		{
			UObject* Val = nullptr;
			InProp->GetValue(Val);
			return (Val && Val->GetClass()->IsChildOf(Class));
		}
		return false;
	}
}

/* FAtomDspBusSettingCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomDspBusSettingCustomization::MakeInstance()
{
	return MakeShareable(new FAtomDspBusSettingCustomization);
}

void FAtomDspBusSettingCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomDspBusSettingCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Sound type without bank are not customized.
	if (!CanCustomize(StructPropertyHandle))
	{
		ChildBuilder.AddCustomRow(FText::GetEmpty())
			.NameContent()
			[
				StructPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				StructPropertyHandle->CreatePropertyValueWidget()
			];
		return;
	}

	// Get handle to layout builder to enable adding properties to categories
	//IDetailLayoutBuilder& LayoutBuilder = ChildBuilder.GetParentCategory().GetParentLayout();

	DspBusSettingHandle = StructPropertyHandle;

	// Create a fake sound property to show sound bank and select property from it.
	ConfigHolder = CreateConfigPropertyHolder();
	TSharedPtr<IPropertyHandle> ConfigHandle = ConfigHolder.Get()->GetProperty();

	// Set default value
	UObject* DspBusSettingObject = nullptr;
	DspBusSettingHandle->GetValue(DspBusSettingObject);
	if (UAtomDspBusSetting* DspBusSetting = Cast<UAtomDspBusSetting>(DspBusSettingObject))
	{
		ConfigHandle->SetValue(DspBusSetting->GetConfig());
	}
	else
	{
		ConfigHandle->SetValue((UObject*)nullptr);
	}

	// -- Setup a subobject picker to select an AtomDspBusSetting from an AtomConfig

	FSubObjectPickerConfig Config;
	Config.ParentHandle = ConfigHandle;
	Config.ObjectHandle = DspBusSettingHandle;
	Config.OnGetSubObjects = FOnGetSubObjectAssets::CreateLambda([this](TSharedPtr<IPropertyHandle>, TArray<FAssetData>& Assets)
	{
		if (ConfigHolder.IsValid())
		{
			for (int Index = 0; Index < ConfigHolder->GetNumSubObjects(); Index++)
			{ 
				Assets.Add(ConfigHolder->GetSubObject(Index));
			} 
		}
	});
	Config.bOpenAssetOnDoubleClick = true;
	Config.bAllowAssetDragging = true;
	Config.bAllowAssetDropping = true;

	DspBusSettingPicker = MakeShared<FSubObjectPicker>(Config);

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			ConfigHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			ConfigHandle->CreatePropertyValueWidget()
		];
	
	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			DspBusSettingPicker->CreateSubObjectPropertyNameWidget()
		]
		.ValueContent()
		[
			DspBusSettingPicker->CreateSubObjectPropertyValueWidget()
		];
}

TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> FAtomDspBusSettingCustomization::CreateConfigPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomConfigPropertyHolder_Internal>(NewObject<UAtomConfigPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init(UAtomConfigPropertyHolder_Internal::Target::MixerSettings);
	}

	return HolderPtr;
}

bool FAtomDspBusSettingCustomization::CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	if (StructPropertyHandle->IsValidHandle())
	{
		// do not customize object in parent object
		bool bIsParentProperty = StructPropertyHandle->GetOuterBaseClass() == UAtomConfig::StaticClass();

		// only customize for any sound asset if AtomDspBusSetting exactly, by default derived class doesn't customize
		auto ObjectProperty = CastField<FObjectProperty>(StructPropertyHandle->GetProperty());
		bool bIsDspBusSettingClass = ObjectProperty && ObjectProperty->PropertyClass == UAtomDspBusSetting::StaticClass();
		
		if (!ObjectProperty)
		{
			auto SoftObjectProperty = CastField<FSoftObjectProperty>(StructPropertyHandle->GetProperty());
			bIsDspBusSettingClass = SoftObjectProperty && SoftObjectProperty->PropertyClass == UAtomDspBusSetting::StaticClass();
		}

		return !bIsParentProperty && bIsDspBusSettingClass;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
