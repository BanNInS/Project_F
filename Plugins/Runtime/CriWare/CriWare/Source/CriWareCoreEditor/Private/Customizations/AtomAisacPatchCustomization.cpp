/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacPatchCustomization.cpp
 *
 ****************************************************************************/

#include "AtomAisacPatchCustomization.h"

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
#include "Atom/AtomAisacPatch.h"

#define LOCTEXT_NAMESPACE "AtomAisacPatch"

namespace AtomAisacPatchUtils
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

/* FAtomAisacPatchCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomAisacPatchCustomization::MakeInstance()
{
	return MakeShareable(new FAtomAisacPatchCustomization);
}

void FAtomAisacPatchCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomAisacPatchCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
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

	PatchHandle = StructPropertyHandle;

	// Create a fake sound property to show sound bank and select property from it.
	ConfigHolder = CreateConfigPropertyHolder();
	TSharedPtr<IPropertyHandle> ConfigHandle = ConfigHolder.Get()->GetProperty();

	// Set default value
	UObject* PatchObject = nullptr;
	PatchHandle->GetValue(PatchObject);
	if (UAtomAisacPatch* Patch = Cast<UAtomAisacPatch>(PatchObject))
	{
		ConfigHandle->SetValue(Patch->GetConfig());
	}
	else
	{
		ConfigHandle->SetValue((UObject*)nullptr);
	}

	// -- Setup a subobject picker to select an AtomAisacPatch from an AtomRack

	FSubObjectPickerConfig Config;
	Config.ParentHandle = ConfigHandle;
	Config.ObjectHandle = PatchHandle;
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

	PatchPicker = MakeShared<FSubObjectPicker>(Config);
	PatchPicker->AddParentPropertyRowContent(ChildBuilder.AddCustomRow(FText::GetEmpty()));
	PatchPicker->AddSubObjectPropertyRowContent(ChildBuilder.AddCustomRow(FText::GetEmpty())); // hidden if parent is null
}

TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> FAtomAisacPatchCustomization::CreateConfigPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomConfigPropertyHolder_Internal>(NewObject<UAtomConfigPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

bool FAtomAisacPatchCustomization::CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	if (StructPropertyHandle->IsValidHandle())
	{
		// do not customize object in parent object
		bool bIsParentProperty = StructPropertyHandle->GetOuterBaseClass() == UAtomConfig::StaticClass();

		// only customize for any sound asset if AtomAisacPatch exactly, by default derived class doesn't customize
		auto ObjectProperty = CastField<FObjectProperty>(StructPropertyHandle->GetProperty());
		bool bIsPatchClass = ObjectProperty && ObjectProperty->PropertyClass == UAtomAisacPatch::StaticClass();

		if (!ObjectProperty)
		{
			auto SoftObjectProperty = CastField<FSoftObjectProperty>(StructPropertyHandle->GetProperty());
			bIsPatchClass = SoftObjectProperty && SoftObjectProperty->PropertyClass == UAtomAisacPatch::StaticClass();
		}

		return !bIsParentProperty && bIsPatchClass;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
