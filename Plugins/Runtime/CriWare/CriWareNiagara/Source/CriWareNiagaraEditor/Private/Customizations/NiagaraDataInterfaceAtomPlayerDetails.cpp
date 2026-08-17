
#include "NiagaraDataInterfaceAtomPlayerDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"

#include "CriWare.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomAisacPatch.h"

#include "Customizations/AtomAisacPatchCustomization.h"
#include "Widgets/SubObjectPicker.h"

#include "NiagaraDataInterfaceAtomPlayer.h"

#define LOCTEXT_NAMESPACE	"NiagaraDataInterfaceAtomPlayerInspector"

TSharedRef<IDetailCustomization> FNiagaraDataInterfaceAtomPlayerDetails::MakeInstance()
{
	return MakeShareable(new FNiagaraDataInterfaceAtomPlayerDetails);
}

void FNiagaraDataInterfaceAtomPlayerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// -- AISAC

	// Get AISAC category.
	IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(TEXT("AISAC"));

	// Additonal Global AISACs
	AdditionalAisacPatchesHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceAtomPlayer, AdditionalAisacPatches));

	// Create a fake sound property to show sound bank and select property from it.
	ConfigHolder = CreateConfigPropertyHolder();
	TSharedPtr<IPropertyHandle> ConfigHandle = ConfigHolder.Get()->GetProperty();

	// Set default value from first element if available.
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
		CategoryBuilder.AddProperty(ConfigHandle.ToSharedRef()).CustomWidget()
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
		CategoryBuilder.AddCustomBuilder(PatchArrayBuilder);
	}

	// AISAC Controls
	TSharedRef<IPropertyHandle> AisacControlsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomComponent, AisacControlModulations));
	CategoryBuilder.AddProperty(AisacControlsHandle);
}

void FNiagaraDataInterfaceAtomPlayerDetails::GenerateAisacPatchWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	if (ArrayIndex == 0)
	{
		AdditionalAisacPatchPickers.Empty();
	}

	if (ConfigHolder.IsValid())
	{
		FSubObjectPickerConfig Config;
		Config.ParentHolder = ConfigHolder;
		Config.ObjectHandle = PropertyHandle;
		Config.bOpenAssetOnDoubleClick = true;
		Config.bAllowAssetDragging = true;
		Config.bAllowAssetDropping = true;

		auto PatchPicker = MakeShared<FSubObjectPicker>(Config);
		AdditionalAisacPatchPickers.Emplace(PatchPicker);

		IDetailPropertyRow& PropertyRow = ChildrenBuilder.AddProperty(PropertyHandle);
		PatchPicker->AddSubObjectPropertyRowContent(PropertyRow.CustomWidget());
	}
}

TStrongObjectPtr<UAtomConfigPropertyHolder_Internal> FNiagaraDataInterfaceAtomPlayerDetails::CreateConfigPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomConfigPropertyHolder_Internal>(NewObject<UAtomConfigPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}


#undef LOCTEXT_NAMESPACE
