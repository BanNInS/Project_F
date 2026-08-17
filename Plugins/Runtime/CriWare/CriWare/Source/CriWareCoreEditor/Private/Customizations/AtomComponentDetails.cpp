/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomComponentDetails.cpp
 *
 ****************************************************************************/

#include "AtomComponentDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"

#include "Atom/AtomComponent.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomSoundCue.h"

#include "AtomAisacPatchCustomization.h"

#define LOCTEXT_NAMESPACE	"AtomComponentDetailsInspector"

TSharedRef<IDetailCustomization> FAtomComponentDetails::MakeInstance()
{
	return MakeShareable(new FAtomComponentDetails);
}

void FAtomComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	SoundHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, Sound));

	/*// -- AISAC

	// Get AISAC category.
	IDetailCategoryBuilder& AisacCategoryBuilder = DetailLayout.EditCategory(TEXT("AISAC"));

	// Cue AISACs - not yet supported
	TSharedRef<IPropertyHandle> AisacPatchesHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, AisacPatches));
	AisacPatchesHandle->MarkHiddenByCustomization();

	// Additonal Global AISACs
	AdditionalAisacPatchesHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, AdditionalAisacPatches));

	// Create a fake sound property to show sound bank and select property from it.
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
		AisacCategoryBuilder.AddProperty(ConfigHandle.ToSharedRef()).CustomWidget()
			.NameContent()
			[
				ConfigHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				ConfigHandle->CreatePropertyValueWidget()
			];

		TSharedRef<FDetailArrayBuilder> PatchArrayBuilder = MakeShareable(new FDetailArrayBuilder(AdditionalAisacPatchesHandle.ToSharedRef(), true));
		PatchArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FAtomComponentDetails::GenerateAisacPatchWidget));
		AisacCategoryBuilder.AddCustomBuilder(PatchArrayBuilder);
	}

	// AISAC Controls
	TSharedRef<IPropertyHandle> AisacControlsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, AisacControlModulations));
	AisacCategoryBuilder.AddProperty(AisacControlsHandle);
	*/

	// -- Sound Cue

	// Get Sound Cue category.
	IDetailCategoryBuilder& CueCategoryBuilder = DetailLayout.EditCategory(TEXT("Sound Cue"));

	// Get cue properties.
	TSharedRef<IPropertyHandle> CueSelectorsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, CueSelectors));
	TSharedRef<IPropertyHandle> CueFirstBlockIndexHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, CueFirstBlockIndex));

	// Enable properties if a sound cue is selected.
	TAttribute<bool> EnabledAttribute = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FAtomComponentDetails::IsSoundSelected, UAtomSoundCue::StaticClass()));
	CueCategoryBuilder.AddProperty(CueSelectorsHandle).IsEnabled(EnabledAttribute);
	CueCategoryBuilder.AddProperty(CueFirstBlockIndexHandle).IsEnabled(EnabledAttribute);
}

/*void FAtomComponentDetails::GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
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
}*/

TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> FAtomComponentDetails::CreateConfigPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomConfigPropertyHolder_Internal>(NewObject<UAtomConfigPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

bool FAtomComponentDetails::CanCustomize(TSharedRef<IPropertyHandle> StructPropertyHandle)
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

bool FAtomComponentDetails::IsSoundSelected(UClass* Class) const
{
	if (SoundHandle.IsValid() && SoundHandle->IsValidHandle())
	{
		UObject* Val = nullptr;
		SoundHandle->GetValue(Val);
		return (Val && Val->GetClass()->IsChildOf(Class));
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
