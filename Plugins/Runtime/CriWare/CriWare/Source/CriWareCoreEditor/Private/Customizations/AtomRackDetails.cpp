/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackDetails.cpp
 *
 ****************************************************************************/

#include "AtomRackDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#include "Atom/AtomRack.h"
#include "Atom/AtomConfig.h"

#include "Customizations/SubObjectPicker.h"

#define LOCTEXT_NAMESPACE	"AtomRackDetailsInspector"

TSharedRef<IDetailCustomization> FAtomRackDetails::MakeInstance()
{
	return MakeShareable(new FAtomRackDetails);
}

void FAtomRackDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Obtain object on customization
	/*TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		return;
	}
	CachedRack = CastChecked<UAtomRack>(Objects[0].Get());*/

	TArray<FName> CategoryNames;
	DetailLayout.GetCategoryNames(CategoryNames);

	// Get the master bus.
	TSharedPtr<IPropertyHandle> MasterBusHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, MasterBus), UAtomRackWithParentBase::StaticClass());
	
	// Hide master bus name.
	TSharedPtr<IPropertyHandle> BusNameHandle = MasterBusHandle->GetChildHandle(TEXT("BusName"));
	BusNameHandle->MarkHiddenByCustomization();

	// Rename Level Mixer category of master bus.
	DetailLayout.EditCategory(TEXT("Level Mixer"), LOCTEXT("MasterBusLevelMixerDisplayName", "Level Mixer (Master Bus)"));

	// Rename Envelope Follower category of master bus.
	DetailLayout.EditCategory(TEXT("Envelope Follower"), LOCTEXT("MasterBusEnvelopeFollowerDisplayName", "Envelope Follower (Master Bus)"));
	
	// Show only Effects category if master bus uses effects.
	UObject* BusObject = nullptr;
	MasterBusHandle->GetValue(BusObject);
	if (UAtomBus* MasterBus = Cast<UAtomBus>(BusObject))
	{
		if (MasterBus->GetNumEffectPresets() > 0)
		{
			DetailLayout.EditCategory(TEXT("Effects"), LOCTEXT("MasterBusEffectsDisplayName", "Effects (Master Bus)"));
		}
		else
		{
			DetailLayout.HideCategory(TEXT("Effects"));
		}
	}

	// Get Buses category.
	IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(TEXT("Buses"));

	// Set AtomConfig property in first.
	AtomConfigHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, AtomConfig));
	AtomConfigHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomRackDetails::OnAtomConfigChanged));
	CategoryBuilder.AddProperty(AtomConfigHandle);

	// Set DspBusSetting property in first
	DspBusSettingHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, DspBusSetting));
	DspBusSettingHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomRackDetails::OnDspBusSettingChanged));
	// replace picker customization because we want to share the AtomConfig handle both Mixer settings and snapshots

	// Configure filter for asset picker.
	FSubObjectPickerConfig Config;
	Config.ParentHandle = AtomConfigHandle;
	Config.ObjectHandle = DspBusSettingHandle;
	Config.OnGetSubObjects = FOnGetSubObjectAssets::CreateLambda([this](TSharedPtr<IPropertyHandle>, TArray<FAssetData>& Assets)
	{
		UObject* ConfigObject = nullptr;
		AtomConfigHandle->GetValue(ConfigObject);
		if (UAtomConfig* AtomConfig = Cast<UAtomConfig>(ConfigObject))
		{
			for (int Index = 0; Index < AtomConfig->GetNumDspBusSettings(); Index++)
			{
				Assets.Add(AtomConfig->GetDspBusSetting(Index));
			}
		}
	});
	Config.bOpenAssetOnDoubleClick = true;
	Config.bAllowAssetDragging = true;
	Config.bAllowAssetDropping = true;

	DspBusSettingPicker = MakeShared<FSubObjectPicker>(Config);

	IDetailPropertyRow& BusRow = CategoryBuilder.AddProperty(DspBusSettingHandle.ToSharedRef());
	BusRow.CustomWidget()
		.NameContent()
		[
			DspBusSettingPicker->CreateSubObjectPropertyNameWidget()
		]
		.ValueContent()
		[
			DspBusSettingPicker->CreateSubObjectPropertyValueWidget()
		];

	// Set DspBusSettingName next (read only)
	DspBusSettingNameHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, DspBusSettingName));
	IDetailPropertyRow& DspBusSettingNameRow = CategoryBuilder.AddProperty(DspBusSettingNameHandle);

	// Customize Snapshot with a combo-box
	SnapshotHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, SnapshotName));
	IDetailPropertyRow& SnapshotRow = CategoryBuilder.AddProperty(SnapshotHandle);

	// Drop box names
	TSharedPtr<FString> InitialSelectedSnapshotName = GenerateCachedSnapshotNames();

	// Snapshot customization
	SnapshotRow.CustomWidget()
		.NameContent()
		[
			SnapshotHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SAssignNew(SnapshotComboBox, SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&CachedSnapshotNameArray)
			.IsEnabled(this, &FAtomRackDetails::IsSnapshotComboBoxEnabled)
			.OnGenerateWidget(this, &FAtomRackDetails::OnGenerateSnapshotComboBoxWidget)
			.OnComboBoxOpening(this, &FAtomRackDetails::OnSnapshotComboBoxOpening)
			.OnSelectionChanged(this, &FAtomRackDetails::OnSnapshotComboBoxSelectionChanged)
			.InitiallySelectedItem(InitialSelectedSnapshotName)
			.ContentPadding(FMargin(2.0f, 2.0f))
			.Content()
			[
				SNew(STextBlock)
				.Text(this, &FAtomRackDetails::GetSnapshotComboBoxContent)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ToolTipText(this, &FAtomRackDetails::GetSnapshotComboBoxToolTip)
			]
		];

	// Set buses in last position
	auto BusesHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomRack, Buses));
	CategoryBuilder.AddProperty(BusesHandle);

	// Keep original categories order
	DetailLayout.SortCategories([=](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			const FName CategoryName = Pair.Key;
			int32 SortOrder = CategoryNames.Find(CategoryName);
			Pair.Value->SetSortOrder(SortOrder);
		}
	});
}

void FAtomRackDetails::OnAtomConfigChanged()
{
	OnDspBusSettingChanged();
}

void FAtomRackDetails::OnDspBusSettingChanged()
{
	if (SnapshotComboBox)
	{
		TSharedPtr<FString> SelectedName = GenerateCachedSnapshotNames();
		SnapshotComboBox->SetSelectedItem(SelectedName);
		SnapshotComboBox->RefreshOptions();
	}
}

TSharedPtr<FString> FAtomRackDetails::GenerateCachedSnapshotNames()
{
	TSharedPtr<FString> InitialSelectedSnapshot = MakeShared<FString>(TEXT("None"));
	CachedSnapshotNameArray.Reset();
	CachedSnapshotNameArray.Add(InitialSelectedSnapshot);

	UObject* DspBusSettingObject = nullptr;
	DspBusSettingHandle->GetValue(DspBusSettingObject);
	if (UAtomDspBusSetting* DspBusSetting = Cast<UAtomDspBusSetting>(DspBusSettingObject))
	{
		FString RackSnapshot;
		SnapshotHandle->GetValue(RackSnapshot);

		for (int Index = 0; DspBusSetting && Index < DspBusSetting->GetNumSnapshots(); ++Index)
		{
			TSharedPtr<FString> SnapshotName = MakeShared<FString>(DspBusSetting->GetSnapshotName(Index));
			CachedSnapshotNameArray.Add(SnapshotName);

			if (!RackSnapshot.IsEmpty() && RackSnapshot == *SnapshotName)
			{
				InitialSelectedSnapshot = SnapshotName;
			}
		}
	}

	return InitialSelectedSnapshot;
}

bool FAtomRackDetails::IsSnapshotComboBoxEnabled() const
{
	UObject* DspBusSetting = nullptr;
	DspBusSettingHandle->GetValue(DspBusSetting);
	return DspBusSetting != nullptr;
}

TSharedRef<SWidget> FAtomRackDetails::OnGenerateSnapshotComboBoxWidget(TSharedPtr<FString> StringItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*StringItem))
		.Font(IDetailLayoutBuilder::GetDetailFont());
}

void FAtomRackDetails::OnSnapshotComboBoxOpening()
{
	TSharedPtr<FString> SelectedName = GenerateCachedSnapshotNames();
	SnapshotComboBox->SetSelectedItem(SelectedName);
	SnapshotComboBox->RefreshOptions();
}

void FAtomRackDetails::OnSnapshotComboBoxSelectionChanged(TSharedPtr<FString> NameItem, ESelectInfo::Type SelectInfo)
{
	if (NameItem.IsValid())
	{
		SnapshotHandle->SetValue(*NameItem != TEXT("None") ? *NameItem : FString());
	}
}

FText FAtomRackDetails::GetSnapshotComboBoxContent() const
{
	FString SnapshotName;
	SnapshotHandle->GetValue(SnapshotName);
	return FText::FromString(!SnapshotName.IsEmpty() ? SnapshotName : TEXT("None"));
}

FText FAtomRackDetails::GetSnapshotComboBoxToolTip() const
{
	FString SnapshotName;
	SnapshotHandle->GetValue(SnapshotName);
	return FText::FromString(!SnapshotName.IsEmpty() ? SnapshotName : TEXT("None"));
}

TSharedRef<IDetailCustomization> FAtomSoundfieldRackDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundfieldRackDetails);
}

void FAtomSoundfieldRackDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<FName> CategoryNames;
	DetailLayout.GetCategoryNames(CategoryNames);

	// Get the master bus.
	MasterBusHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundfieldRack, MasterBus), UAtomRackWithParentBase::StaticClass());

	// Hide master bus name.
	TSharedPtr<IPropertyHandle> BusNameHandle = MasterBusHandle->GetChildHandle(TEXT("BusName"));
	BusNameHandle->MarkHiddenByCustomization();
	
	// Rename Level Mixer category of master bus and edit behavior.
	IDetailCategoryBuilder& LevelMixerCategory = DetailLayout.EditCategory(TEXT("Level Mixer"), LOCTEXT("MasterBusLevelMixerDisplayName", "Level Mixer (Master Bus)"));

	// Hide bus sends.
	TSharedPtr<IPropertyHandle> SendsHandle = MasterBusHandle->GetChildHandle(TEXT("Sends"));
	SendsHandle->MarkHiddenByCustomization();

	// Get the renderer type.
	RendererTypeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundfieldRack, SoundfieldRendererType));
	RendererTypeHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomSoundfieldRackDetails::OnSoundfieldRendererTypeChanged));

	// Get the source mixer's spatialization type.
	TSharedPtr<IPropertyHandle> SpatializationHandle = MasterBusHandle->GetChildHandle(TEXT("SpatializationType"));
	LevelMixerCategory.AddProperty(SpatializationHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Set the mixer panning.
	TSharedPtr<IPropertyHandle> PanningHandle = MasterBusHandle->GetChildHandle(TEXT("Panning"));
	LevelMixerCategory.AddProperty(PanningHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Set the mixer level matrix.
	TSharedPtr<IPropertyHandle> MatrixHandle = MasterBusHandle->GetChildHandle(TEXT("ChannelLevelMatrix"));
	LevelMixerCategory.AddProperty(MatrixHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Set the Wet level.
	TSharedPtr<IPropertyHandle> WetLevelModulationHandle = MasterBusHandle->GetChildHandle(TEXT("WetLevelModulation"));
	LevelMixerCategory.AddProperty(WetLevelModulationHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Set the Dry level.
	TSharedPtr<IPropertyHandle> DryLevelModulationHandle = MasterBusHandle->GetChildHandle(TEXT("DryLevelModulation"));
	LevelMixerCategory.AddProperty(DryLevelModulationHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Rename Envelope Follower category of master bus and edit behavior.
	IDetailCategoryBuilder& EnvelopeFollowerCategory = DetailLayout.EditCategory(TEXT("Envelope Follower"), LOCTEXT("MasterBusEnvelopeFollowerDisplayName", "Envelope Follower (Master Bus)"));
	
	// Set the Attack Time.
	TSharedPtr<IPropertyHandle> EnvelopeFollowerAttackTimeHandle = MasterBusHandle->GetChildHandle(TEXT("EnvelopeFollowerAttackTime"));
	EnvelopeFollowerCategory.AddProperty(EnvelopeFollowerAttackTimeHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));
	
	// Set the realse Time
	TSharedPtr<IPropertyHandle> EnvelopeFollowerReleaseTimeHandle = MasterBusHandle->GetChildHandle(TEXT("EnvelopeFollowerReleaseTime"));
	EnvelopeFollowerCategory.AddProperty(EnvelopeFollowerReleaseTimeHandle.ToSharedRef())
		.Visibility(TAttribute<EVisibility>(this, &FAtomSoundfieldRackDetails::IsSpatializationVisible));

	// Show only Effects category if master bus uses effects.
	UObject* BusObject = nullptr;
	MasterBusHandle->GetValue(BusObject);
	if (UAtomBus* MasterBus = Cast<UAtomBus>(BusObject))
	{
		if (MasterBus->GetNumEffectPresets() > 0)
		{
			DetailLayout.EditCategory(TEXT("Effects"), LOCTEXT("MasterBusEffectsDisplayName", "Effects (Master Bus)"));
		}
		else
		{
			DetailLayout.HideCategory(TEXT("Effects"));
		}
	}

	// Keep original categories order
	DetailLayout.SortCategories([=](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			const FName CategoryName = Pair.Key;
			int32 SortOrder = CategoryNames.Find(CategoryName);
			Pair.Value->SetSortOrder(SortOrder);
		}
	});
}

EVisibility FAtomSoundfieldRackDetails::IsSpatializationVisible() const
{
	EAtomSoundfieldRendererType Type;
	RendererTypeHandle->GetValue((uint8&)Type);

	if (Type == EAtomSoundfieldRendererType::Ambisonics || Type == EAtomSoundfieldRendererType::SoundObject)
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}

void FAtomSoundfieldRackDetails::OnSoundfieldRendererTypeChanged()
{
	EAtomSoundfieldRendererType Type;
	RendererTypeHandle->GetValue((uint8&)Type);

	if (Type == EAtomSoundfieldRendererType::Ambisonics || Type == EAtomSoundfieldRendererType::SoundObject)
	{
		UObject* BusObject = nullptr;
		MasterBusHandle->GetValue(BusObject);
		if (UAtomBus* MasterBus = Cast<UAtomBus>(BusObject))
		{
			MasterBus->SpatializationType = EAtomSpatializationType::SendToChannel;
		}
	}
}

TSharedRef<IDetailCustomization> FAtomEndpointRackDetails::MakeInstance()
{
	return MakeShareable(new FAtomEndpointRackDetails);
}

void FAtomEndpointRackDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{

}

TSharedRef<IDetailCustomization> FAtomSoundfieldEndpointRackDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundfieldEndpointRackDetails);
}

void FAtomSoundfieldEndpointRackDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{

}

#undef LOCTEXT_NAMESPACE
