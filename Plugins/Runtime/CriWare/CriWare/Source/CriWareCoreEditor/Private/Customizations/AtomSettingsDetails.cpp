
#include "AtomSettingsDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailGroup.h"
#include "DetailWidgetRow.h"

#include "Atom/AtomRuntime.h"
#include "Customizations/SubObjectPicker.h"

#define LOCTEXT_NAMESPACE "AtomSettingsDetails"

static FString VersionHexToString(uint32 VersionHex)
{
	FString VersionStr;
	if (VersionHex != INDEX_NONE)
	{
		VersionStr = FString::Printf(TEXT("%08x"), VersionHex);
		VersionStr.InsertAt(2, TEXT('.'));
		VersionStr.InsertAt(5, TEXT('.'));
		VersionStr.InsertAt(8, TEXT('.'));
		if (VersionStr[0] == TEXT('0'))
		{
			VersionStr.RightChopInline(1);
		}
	}
	return VersionStr;
};

TSharedRef<IDetailCustomization> FAtomSettingsDetails::MakeInstance()
{
	return MakeShareable(new FAtomSettingsDetails);
}

void FAtomSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	auto& AtomCategory = DetailLayout.EditCategory(TEXT("Atom"));

	// Atom Version Info
	{
		FText VersionLabel = LOCTEXT("VersionLabel", "Version");
		FText VersionToolTip = LOCTEXT("VersionTooltip", "The CriWare Atom library version information used by the plugin.");
		FText AtomVersion = FText::FromString(FAtomRuntime::GetAtomLibraryVersionString());

		AtomCategory.AddCustomRow(VersionLabel)
			.NameContent()
			[
				SNew(STextBlock)
					.Text(VersionLabel)
					.ToolTipText(VersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(STextBlock)
					.Text(AtomVersion)
					.ToolTipText(VersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			];
	}

	// Atom ACF version range
	{
		FText AcfVersionLabel = LOCTEXT("ACFVersionRangeLabel", "ACF Versions");
		FText AcfVersionToolTip = LOCTEXT("ACFVersionRangeTooltip", "The range of compatible Atom Configuration File versions that can be used.");

		auto GetAllowedAcfVersionText = []()
		{
			FUintVector2 AllowedACFVersionRange = FAtomRuntime::GetAllowedACFVersions();
			FString CurrentVersion = VersionHexToString(FAtomRuntime::GetCurrentACFVersion());
			FFormatNamedArguments ACFVersionArgs;
			ACFVersionArgs.Add(TEXT("Min"), FText::FromString(VersionHexToString(AllowedACFVersionRange.X)));
			ACFVersionArgs.Add(TEXT("Max"), FText::FromString(VersionHexToString(AllowedACFVersionRange.Y)));
			ACFVersionArgs.Add(TEXT("Current"), FText::FromString(CurrentVersion.Len() ? CurrentVersion : FString(TEXT("--"))));
			return FText::Format(LOCTEXT("AllowedAcfVersionsText", "Allowed ACF versions: {Min} through {Max}\nCurrent ACF version: {Current}"), ACFVersionArgs);
		};

		TSharedPtr<STextBlock> AcfVersionsTextBlock;

		AtomCategory.AddCustomRow(AcfVersionLabel)
			.NameContent()
			[
				SNew(STextBlock)
					.Text(AcfVersionLabel)
					.ToolTipText(AcfVersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SAssignNew(AcfVersionsTextBlock, STextBlock)
					.Text(GetAllowedAcfVersionText())
					.ToolTipText(AcfVersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			];

		TSharedRef<IPropertyHandle> AtomConfigHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, AtomConfig));
		AtomConfigHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSPLambda(this, [AcfVersionsTextBlock, GetAllowedAcfVersionText]()
			{
				if (AcfVersionsTextBlock.IsValid())
				{
					AcfVersionsTextBlock->SetText(GetAllowedAcfVersionText());
				}
			})
		);
	}

	// Atom ACB version range
	{
		FText AcbVersionLabel = LOCTEXT("ACBVersionRangeLabel", "ACB Versions");
		FText AcbVersionToolTip = LOCTEXT("ACBVersionRangeTooltip", "The range of compatible Atom Cuesheet Binary versions that can be used.");

		FUintVector2 AllowedACBVersionRange = FAtomRuntime::GetAllowedACBVersions();
		FFormatNamedArguments ACBVersionArgs;
		ACBVersionArgs.Add(TEXT("Min"), FText::FromString(VersionHexToString(AllowedACBVersionRange.X)));
		ACBVersionArgs.Add(TEXT("Max"), FText::FromString(VersionHexToString(AllowedACBVersionRange.Y)));
		FText AllowedAcbVersions = FText::Format(LOCTEXT("AllowedAcbVersionsText", "Allowed ACB versions: {Min} through {Max}"), ACBVersionArgs);

		AtomCategory.AddCustomRow(AcbVersionLabel)
			.NameContent()
			[
				SNew(STextBlock)
					.Text(AcbVersionLabel)
					.ToolTipText(AcbVersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(STextBlock)
					.Text(AllowedAcbVersions)
					.ToolTipText(AcbVersionToolTip)
					.Font(IDetailLayoutBuilder::GetDetailFont())
			];
	}

	TSharedRef<IPropertyHandle> PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, AtomConfig));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseInGamePreview));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MonitorCommunicationBufferSize));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MaxPitch));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DistanceFactor));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MaxSoundSources));
	AtomCategory.AddProperty(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, NumReservedSoundSources));
	AtomCategory.AddProperty(PropertyHandle);

	auto& MixGroup = AtomCategory.AddGroup("Mix", LOCTEXT("Mix", "Mix"), false, true);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, MasterRack));
	MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, SpatializationRack));
	if (FModuleManager::Get().IsModuleLoaded("CriWareAtomMixer"))
	{
		CustomizeAtomBusProperty(MixGroup, PropertyHandle, LOCTEXT("ParentSpatializationRackName", "Spatialization Bus (Rack)"));
	}
	else
	{	
		MixGroup.AddPropertyRow(PropertyHandle);
	}
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableBinauralSpatialization));
	MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseAudioLink));
	MixGroup.AddPropertyRow(PropertyHandle);
	//PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseAtomLink));
	//MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseUnrealSoundRenderer));
	MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultOutputSubmix));
	MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bAllowCenterChannel3DPanning));
	MixGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultAudioBuses));
	MixGroup.AddPropertyRow(PropertyHandle);

	auto& SoundsGroup = AtomCategory.AddGroup("Sounds", LOCTEXT("Sounds", "Sounds"));
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultSoundClassName));
	SoundsGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultManaSoundClassName));
	SoundsGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DefaultSoundConcurrencyName));
	SoundsGroup.AddPropertyRow(PropertyHandle);

	auto& VoicesGroup = AtomCategory.AddGroup("Voices", LOCTEXT("Voices", "Voices"));
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bUseAutomaticVoiceManagement));
	VoicesGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, VoicesSettings));
	VoicesGroup.AddPropertyRow(PropertyHandle);

	auto& DecodingGroup = AtomCategory.AddGroup("Decoding", LOCTEXT("Decoding", "Decoding"));
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableHcaMxDecoding));
	DecodingGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, HcaMxSettings));
	DecodingGroup.AddPropertyRow(PropertyHandle);

	auto& ModulationGroup = AtomCategory.AddGroup("Modulation", LOCTEXT("Modulation", "Modulation"));
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, ModulationParameters));
	ModulationGroup.AddPropertyRow(PropertyHandle);

	auto& DebugGroup = AtomCategory.AddGroup("Debug", LOCTEXT("Debug", "Debug"));
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, bEnableLoggingAtom));
	DebugGroup.AddPropertyRow(PropertyHandle);
	PropertyHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UCriWareCoreSettings, DebugSounds));
	DebugGroup.AddPropertyRow(PropertyHandle);
}

void FAtomSettingsDetails::CustomizeAtomBusProperty(IDetailGroup& CategoryGroup, TSharedRef<IPropertyHandle>& PropertyHandle, const FText& ParentPropertyDisplayName)
{
	if (PropertyHandle->IsValidHandle())
	{
		// Create a fake rack property to show and select buses from it.
		RackHolder = CreateRackPropertyHolder();
		TSharedPtr<IPropertyHandle> RackHandle = RackHolder.Get()->GetProperty();
		RackHandle->SetPropertyDisplayName(ParentPropertyDisplayName);

		// Set default value of the rack property.
		UObject* BusObject = nullptr;
		PropertyHandle->GetValue(BusObject);
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
		Config.ObjectHandle = PropertyHandle;
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
		BusPicker->AddParentPropertyRowContent(CategoryGroup.AddPropertyRow(RackHandle.ToSharedRef()).CustomWidget());

		// Show asset picker for bus.
		BusPicker->AddSubObjectPropertyRowContent(CategoryGroup.AddPropertyRow(PropertyHandle).CustomWidget());
	}
}

TStrongObjectPtr<UAtomRackPropertyHolder_Internal> FAtomSettingsDetails::CreateRackPropertyHolder()
{
	auto HolderPtr = TStrongObjectPtr<UAtomRackPropertyHolder_Internal>(NewObject<UAtomRackPropertyHolder_Internal>());
	if (auto Holder = HolderPtr.Get())
	{
		Holder->Init();
	}

	return HolderPtr;
}

#undef LOCTEXT_NAMESPACE
