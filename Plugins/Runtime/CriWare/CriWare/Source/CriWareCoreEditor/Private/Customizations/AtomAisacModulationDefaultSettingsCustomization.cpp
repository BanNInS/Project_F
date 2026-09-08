/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacModulationDefaultSettingsCustomization.cpp
 *
 ****************************************************************************/

#include "AtomAisacModulationDefaultSettingsCustomization.h"

#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"

#include "Atom/Atom.h"
#include "Atom/AtomConfig.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomAisacModulation.h"

#define LOCTEXT_NAMESPACE "AtomAisacModulationDefaultSettings"

/* FAtomAisacModulationDefaultSettingsCustomization class
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FAtomAisacModulationDefaultSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FAtomAisacModulationDefaultSettingsCustomization);
}

void FAtomAisacModulationDefaultSettingsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	//StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomAisacModulationDefaultSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	IDetailChildrenBuilder& ModulationCategoryBuilder = ChildBuilder;

	// Get handle to layout builder to enable adding properties to categories
	//IDetailLayoutBuilder& DetailLayout = ChildBuilder.GetParentCategory().GetParentLayout();

	// Get AISAC category.
	//IDetailCategoryBuilder& ModulationCategoryBuilder = DetailLayout.EditCategory(TEXT("AISAC"));

	// Cue/Class AISACs - not yet supported
	TSharedPtr<IPropertyHandle> AisacPatchesHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomAisacModulationDefaultSettings, AisacPatches));
	AisacPatchesHandle->MarkHiddenByCustomization();

	// Additional Global AISACs
	AdditionalAisacPatchesHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomAisacModulationDefaultSettings, AdditionalAisacPatches));

	// Create a fake config property to show it and select AISAC patch.
	ConfigHolder = CreateConfigPropertyHolder();
	ConfigHandle = ConfigHolder.Get()->GetProperty();

	// Set default value from first elements if available.
	ConfigHandle->SetValue((UObject*)nullptr);
	TSharedPtr<IPropertyHandleArray> ArrayHandle = AdditionalAisacPatchesHandle->AsArray();
	if (ArrayHandle.IsValid())
	{
		uint32 NumElements;
		ArrayHandle->GetNumElements(NumElements);

		if (NumElements > 0)
		{
			TSharedRef<IPropertyHandle> ElementHandle = ArrayHandle->GetElement(0);
			UObject* ElementObject = nullptr;
			ElementHandle->GetValue(ElementObject);

			if (UAtomAisacPatch* Patch = Cast<UAtomAisacPatch>(ElementObject))
			{
				ConfigHandle->SetValue(Patch->GetConfig());
			}
		}

		// Show config property.
		ModulationCategoryBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			ConfigHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			ConfigHandle->CreatePropertyValueWidget()
		];

		TSharedRef<FDetailArrayBuilder> PatchArrayBuilder = MakeShareable(new FDetailArrayBuilder(AdditionalAisacPatchesHandle.ToSharedRef(), true));
		PatchArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FAtomAisacModulationDefaultSettingsCustomization::GenerateAisacPatchWidget));
		ModulationCategoryBuilder.AddCustomBuilder(PatchArrayBuilder);
	}

	// AISAC Controls
	TSharedPtr<IPropertyHandle> AisacControlsHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomAisacModulationDefaultSettings, AisacControlModulations));
	ModulationCategoryBuilder.AddProperty(AisacControlsHandle.ToSharedRef());
}

void FAtomAisacModulationDefaultSettingsCustomization::GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	if (ArrayIndex == 0)
	{
		AdditionalAisacPatchPickers.Empty();
	}

	if (ConfigHolder.IsValid())
	{
		FSubObjectPickerConfig Config;
		Config.ParentHandle = ConfigHandle;
		Config.ObjectHandle = PropertyHandle;
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

		auto PatchPicker = MakeShared<FSubObjectPicker>(Config);
		AdditionalAisacPatchPickers.Emplace(PatchPicker);

		IDetailPropertyRow& PropertyRow = ChildrenBuilder.AddProperty(PropertyHandle);
		PatchPicker->AddSubObjectPropertyRowContent(PropertyRow.CustomWidget());
	}
}

TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> FAtomAisacModulationDefaultSettingsCustomization::CreateConfigPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomConfigPropertyHolder_Internal>(NewObject<UAtomConfigPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

bool FAtomAisacModulationDefaultSettingsCustomization::CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	if (StructPropertyHandle->IsValidHandle())
	{
		// do not customize object in parent object
		bool bIsParentProperty = StructPropertyHandle->GetOuterBaseClass() == UAtomConfig::StaticClass();

		// only customize if AtomAisacPatch exactly, by default derived class doesn't customize
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
