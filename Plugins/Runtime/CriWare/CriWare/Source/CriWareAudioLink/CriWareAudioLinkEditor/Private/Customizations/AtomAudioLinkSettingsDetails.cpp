
#include "AtomAudioLinkSettingsDetails.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "CriWare.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomBusEffectPreset.h"

#include "AtomAudioLinkSettings.h"

#define LOCTEXT_NAMESPACE	"AtomAudioLinkSettingsDetailsInspector"

TSharedRef<IDetailCustomization> FAtomAudioLinkSettingsDetails::MakeInstance()
{
	return MakeShareable(new FAtomAudioLinkSettingsDetails);
}

void FAtomAudioLinkSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Get category.
	IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(TEXT("Routing"));
	
	// Set the route 1st
	TSharedPtr<IPropertyHandle> RouteHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomAudioLinkSettings, DestinationRoute));
	check(RouteHandle);
	CategoryBuilder.AddProperty(RouteHandle.ToSharedRef());

	// Get bus properties to customize.
	TSharedPtr<IPropertyHandle> BusHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomAudioLinkSettings, DestinationBus));
	check(BusHandle);

	// Create a fake rack property to show and select buses from it.
	RackHolder = CreateRackPropertyHolder();
	TSharedPtr<IPropertyHandle> RackHandle = RackHolder.Get()->GetProperty();
	check(RackHandle);

	// Set default value of the rack property.
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

	// Configure filter for asset picker.
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
	check(BusPicker);

	// Show the rack property.
	BusPicker->AddParentPropertyRowContent(CategoryBuilder.AddProperty(RackHandle.ToSharedRef()).CustomWidget());

	// Show asset picker for bus.
	BusPicker->AddSubObjectPropertyRowContent(CategoryBuilder.AddProperty(BusHandle.ToSharedRef()).CustomWidget());
}

TStrongObjectPtr<UAtomRackPropertyHolder_Internal> FAtomAudioLinkSettingsDetails::CreateRackPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomRackPropertyHolder_Internal>(NewObject<UAtomRackPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

#undef LOCTEXT_NAMESPACE
