/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacControlCustomization.cpp
 *
 ****************************************************************************/

#include "AtomAisacControlCustomization.h"

#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"

#include "CriWare.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomAisacPatch.h"

#include "Widgets/SAssetView.h"

#define LOCTEXT_NAMESPACE "AtomAisacControl"

FAtomAisacControl* GetAisacControlFromPropertyHandle(TSharedPtr<IPropertyHandle> Handle)
{
	TArray<void*> RawData;
	Handle->AccessRawData(RawData);
	return RawData.Num() == 1 ? static_cast<FAtomAisacControl*>(RawData[0]) : nullptr;
}

void SetAisacControlToPropertyHandle(TSharedPtr<IPropertyHandle> Handle, FAtomAisacControl* Value)
{
	Handle->NotifyPreChange();
	if (auto Target = GetAisacControlFromPropertyHandle(Handle))
	{
		*Target = Value ? *Value : FAtomAisacControl();
	}
	Handle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

TSharedRef<IPropertyTypeCustomization> FAtomAisacControlCustomization::MakeInstance()
{
	return MakeShareable(new FAtomAisacControlCustomization);
}

void FAtomAisacControlCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomAisacControlCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Get handle to layout builder to enable adding properties to categories
	//IDetailLayoutBuilder& LayoutBuilder = ChildBuilder.GetParentCategory().GetParentLayout();

	ControlHandle = StructPropertyHandle;
	//ControlHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomAisacControlCustomization::OnControlPropertyChanged));

	// -- Setup a ComboBox to select an AISAC control from AtomConfig
	TSharedPtr<FAtomAisacControl> InitialSelectedSetting = GenerateCachedControls();

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SAssignNew(ControlComboBox, SComboBox<TSharedPtr<FAtomAisacControl>>)
			.OptionsSource(&CachedControlArray)
			.IsEnabled(this, &FAtomAisacControlCustomization::IsControlComboBoxEnabled)
			.OnGenerateWidget(this, &FAtomAisacControlCustomization::OnGenerateControlComboBoxWidget)
			.OnComboBoxOpening(this, &FAtomAisacControlCustomization::OnControlComboBoxOpening)
			.OnSelectionChanged(this, &FAtomAisacControlCustomization::OnControlComboBoxSelectionChanged)
			.InitiallySelectedItem(InitialSelectedSetting)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.Content()
			[
				SAssignNew(ControlComboBoxContentView, STextBlock)
				.Text(this, &FAtomAisacControlCustomization::GetControlComboBoxContent)
				.ToolTipText(this, &FAtomAisacControlCustomization::GetControlComboBoxToolTip)
			]
		];
}

void FAtomAisacControlCustomization::OnControlPropertyChanged()
{

}

TSharedPtr<FAtomAisacControl> FAtomAisacControlCustomization::GenerateCachedControls()
{
	TSharedPtr<FAtomAisacControl> InitialSelectedControl = MakeShared<FAtomAisacControl>();
	CachedControlArray.Reset();
	CachedControlArray.Add(InitialSelectedControl);

	// get default value
	FAtomAisacControl* ControlObject = GetAisacControlFromPropertyHandle(ControlHandle);

	if (GCriWare)
	{
		if (const UAtomConfig* AtomConfig = GCriWare->GetAtomConfiguration())
		{
			for (int Index = 0; Index < AtomConfig->GetNumAisacControls(); ++Index)
			{
				TSharedPtr<FAtomAisacControl> Item = MakeShared<FAtomAisacControl>(AtomConfig->GetAisacControl(Index));
				CachedControlArray.Add(Item);

				if (ControlObject && Item->Name == ControlObject->Name)
				{
					InitialSelectedControl = Item;
				}
			}
		}
	}

	return InitialSelectedControl;
}

bool FAtomAisacControlCustomization::IsControlComboBoxEnabled() const
{
	return ControlHandle ? ControlHandle->IsEditable() : false;
}

TSharedRef<SWidget> FAtomAisacControlCustomization::OnGenerateControlComboBoxWidget(TSharedPtr<FAtomAisacControl> Item)
{
	return SNew(STextBlock)
		.Text(FText::FromName(Item->Name))
		.ToolTipText(FText::FromName(Item->Name));
}

void FAtomAisacControlCustomization::OnControlComboBoxOpening()
{
	TSharedPtr<FAtomAisacControl> SelectedControl = GenerateCachedControls();
	ControlComboBox->SetSelectedItem(SelectedControl);
	ControlComboBox->RefreshOptions();
}

void FAtomAisacControlCustomization::OnControlComboBoxSelectionChanged(TSharedPtr<FAtomAisacControl> Item, ESelectInfo::Type SelectInfo)
{
	if (Item)
	{
		// update the combobox content with selection
		//ControlComboBoxContentView->SetText(FText::FromName(Item->Name));

		// set the selection to property
		SetAisacControlToPropertyHandle(ControlHandle, Item.Get());
	}
}

FText FAtomAisacControlCustomization::GetControlComboBoxContent() const
{
	if (ControlComboBox)
	{
		if (auto Item = ControlComboBox->GetSelectedItem().Get())
		{
			return FText::FromName(Item->Name);
		}
	}

	return FText::FromName(NAME_None);
}

FText FAtomAisacControlCustomization::GetControlComboBoxToolTip() const
{
	if (ControlComboBox)
	{
		if (auto Item = ControlComboBox->GetSelectedItem().Get())
		{
			return FText::FromName(Item->Name);
		}
	}

	return FText::FromName(NAME_None);
}

#undef LOCTEXT_NAMESPACE
