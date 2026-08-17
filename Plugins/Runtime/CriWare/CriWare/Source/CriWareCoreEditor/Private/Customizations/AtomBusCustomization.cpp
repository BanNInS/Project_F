/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusCustomization.cpp
 *
 ****************************************************************************/

#include "AtomBusCustomization.h"

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
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"

#define LOCTEXT_NAMESPACE "AtomBus"

namespace AtomBusUtils
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

/* FAtomBusCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomBusCustomization::MakeInstance()
{
	return MakeShareable(new FAtomBusCustomization);
}

void FAtomBusCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomBusCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
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

	BusHandle = StructPropertyHandle;

	// Create a fake sound property to show sound bank and select property from it.
	RackHolder = CreateRackPropertyHolder();
	TSharedPtr<IPropertyHandle> RackHandle = RackHolder.Get()->GetProperty();

	// Set default value
	UObject* BusObject = nullptr;
	BusHandle->GetValue(BusObject);
	if (UAtomBus* Bus = Cast<UAtomBus>(BusObject))
	{
		RackHandle->SetValue(Bus->GetRack());
	}
	else
	{
		RackHandle->SetValue((UObject*)nullptr);
	}

	// -- Setup a subobject picker to select an AtomBus from an AtomRack

	FSubObjectPickerConfig Config;
	Config.ParentHandle = RackHandle;
	Config.ObjectHandle = BusHandle;
	Config.OnGetSubObjects = FOnGetSubObjectAssets::CreateLambda([this](TSharedPtr<IPropertyHandle>, TArray<FAssetData>& Assets)
	{
		if (RackHolder.IsValid())
		{
			for (int Index = 0; Index < RackHolder->GetNumSubObjects(); Index++)
			{ 
				Assets.Add(RackHolder->GetSubObject(Index));
			} 
		}
	});
	Config.bOpenAssetOnDoubleClick = true;
	Config.bAllowAssetDragging = true;
	Config.bAllowAssetDropping = true;

	BusPicker = MakeShared<FSubObjectPicker>(Config);

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			RackHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			RackHandle->CreatePropertyValueWidget()
		];
	
	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			BusPicker->CreateSubObjectPropertyNameWidget()
		]
		.ValueContent()
		[
			BusPicker->CreateSubObjectPropertyValueWidget()
		];
}

TStrongObjectPtr<UAtomRackPropertyHolder_Internal> FAtomBusCustomization::CreateRackPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomRackPropertyHolder_Internal>(NewObject<UAtomRackPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

bool FAtomBusCustomization::CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	if (StructPropertyHandle->IsValidHandle())
	{
		// do not customize object in parent object
		bool bIsParentProperty = StructPropertyHandle->GetOuterBaseClass() == UAtomRack::StaticClass();

		// only customize for any sound asset if AtomBus exactly, by default derived class doesn't customize
		auto ObjectProperty = CastField<FObjectProperty>(StructPropertyHandle->GetProperty());
		bool bIsBusClass = ObjectProperty && ObjectProperty->PropertyClass == UAtomBus::StaticClass();
		
		if (!ObjectProperty)
		{
			auto SoftObjectProperty = CastField<FSoftObjectProperty>(StructPropertyHandle->GetProperty());
			bIsBusClass = SoftObjectProperty && SoftObjectProperty->PropertyClass == UAtomBus::StaticClass();
		}

		return !bIsParentProperty && bIsBusClass;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
