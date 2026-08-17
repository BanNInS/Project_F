/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : CriWareCoreEditor
* File     : SAtomSoundBrowser.cpp
*
****************************************************************************/

#include "SAtomSoundBrowser.h"

#include "Algo/ForEach.h"
#include "Modules/ModuleManager.h"
#include "Textures/SlateIcon.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDataSource.h"
#include "EditorStyleSet.h"
#include "Editor.h"
#include "Containers/ArrayBuilder.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "AssetToolsModule.h"
#include "IAssetTypeActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "AssetManagerEditorModule.h"
#include "AssetDefinitionRegistry.h"
#include "PropertyEditorModule.h"

#include "Atom/AtomSoundBank.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"

#include "AtomEditorStyle.h"
#include "Models/AtomSoundBankEditorCommands.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#define LOCTEXT_NAMESPACE "SAtomSoundBrowser"

const FName SAtomSoundBrowser::AssetPickerAssetContextMenuName = "AtomSound.AssetPickerAssetContextMenu";
const FName SAtomSoundBrowser::ColumnID_Index(TEXT("Index"));
const FName SAtomSoundBrowser::ColumnID_SoundBank(TEXT("SoundBank"));
const FName SAtomSoundBrowser::ColumnID_Duration(TEXT("Duration"));
const FName SAtomSoundBrowser::ColumnID_OneShot(TEXT("IsOneShot"));
const FName SAtomSoundBrowser::ColumnID_Streamed(TEXT("Streamed"));
const FName SAtomSoundBrowser::ColumnID_Format(TEXT("Format"));
const FName SAtomSoundBrowser::ColumnID_Channels(TEXT("NumChannels"));
const FName SAtomSoundBrowser::ColumnID_Frames(TEXT("Frames"));
const FName SAtomSoundBrowser::ColumnID_SampleRate(TEXT("SampleRate"));
const FName SAtomSoundBrowser::ColumnID_Locked(TEXT("Locked"));

/* SAtomSoundBrowser interface
 *****************************************************************************/

void SAtomSoundBrowser::Construct(const FArguments& InArgs, UAtomSoundBank& InSoundBank)
{
	OnSelectionChanged = InArgs._OnSelectionChanged;

	Commands = MakeShareable(new FUICommandList());
	Commands->MapAction(FAtomSoundBankEditorCommands::Get().CopyReferences, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBrowser::ExecuteCopyReference),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBrowser::IsAnythingSelected)
	));

	Commands->MapAction(FAtomSoundBankEditorCommands::Get().ViewReferences, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBrowser::ExecuteReferenceViewer),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBrowser::IsAnythingSelected)
	));

	Commands->MapAction(FAtomSoundBankEditorCommands::Get().ViewSizeMap, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBrowser::ExecuteSizeMap),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBrowser::IsAnythingSelected)
	));

	TArray<UObject*> InitialBanks;
	InitialBanks.Add(&InSoundBank);
	SetSoundBanks(InitialBanks);
}

FString SAtomSoundBrowser::GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	return GetDisplayTextForCustomColumn(AssetData, ColumnName).ToString();
}

FText SAtomSoundBrowser::GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FText OutValue;

	if (UAtomSoundBase* Sound = Cast<UAtomSoundBase>(AssetData.GetAsset()))
	{
		if (ColumnName == ColumnID_Index)
		{
			if (UAtomSoundBank* SoundBank = Cast<UAtomSoundBank>(Sound->GetOuter()))
			{
				for (int Index = 0; Index < SoundBank->GetNumSounds(); Index++)
				{
					if (SoundBank->GetSound(Index) == Sound)
					{
						OutValue = FText::FromString(FString::FromInt(Index));
						break;
					}
				}
			}
		}
		else if (ColumnName == ColumnID_SoundBank)
		{
			if (UAtomSoundBank* SoundBank = Cast<UAtomSoundBank>(Sound->GetOuter()))
			{
				OutValue = FText::FromString(SoundBank->GetName());
			}
		}
		else if (ColumnName == ColumnID_Duration)
		{
			OutValue = FText::FromString(FTimespan::FromSeconds(Sound->GetDuration()).ToString(TEXT("%h:%m:%s.%f")).RightChop(1));
		}
		else if (ColumnName == ColumnID_OneShot)
		{
			OutValue = Sound->IsOneShot() ? LOCTEXT("OneShot", "One Shot") : LOCTEXT("Looping", "Looping");
		}
		else if (ColumnName == ColumnID_Streamed)
		{
			OutValue = Sound->WaveInfo.bIsStreamed ? LOCTEXT("FromWaveBank", "From WaveBank") : LOCTEXT("No", "No");
		}
		else if (ColumnName == ColumnID_Format)
		{
			OutValue = FText::FromString(::ToString(Sound->WaveInfo.Format));
		}
		else if (ColumnName == ColumnID_Channels)
		{
			OutValue = FText::FromString(FString::FromInt(Sound->WaveInfo.NumChannels));
		}
		else if (ColumnName == ColumnID_Frames)
		{
			OutValue = FText::FromString(FString::FromInt(Sound->WaveInfo.NumFrames));
		}
		else if (ColumnName == ColumnID_SampleRate)
		{
			OutValue = FText::Format(FText::FromString("{0} Hz"), { FText::FromString(FString::FromInt(Sound->WaveInfo.SampleRate)) });
		}
		else if (ColumnName == ColumnID_Locked)
		{
			if (UAtomSoundCue* SoundCue = Cast<UAtomSoundCue>(Sound))
			{
				OutValue = SoundCue->CueInfo.bIgnorePlayerParameters ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No");
			}

			OutValue = LOCTEXT("No", "No");
		}
	}

	return OutValue;
}

void SAtomSoundBrowser::OnGetCustomSourceAssets(const FARFilter& Filter, TArray<FAssetData>& OutAssets) const
{
	for (auto SoundBank : SoundBanks)
	{
		if (SoundBank)
		{
			for (int Index = 0; Index < SoundBank->GetNumSounds(); Index++)
			{
				if (UAtomSoundBase* Sound = SoundBank->GetSound(Index))
				{
					OutAssets.Add(Sound);
				}
			}
		}
	}
}

/* SAtomSoundBrowser implementation
 *****************************************************************************/

void SAtomSoundBrowser::OpenSoundAsset(UObject* Asset)
{

}

void SAtomSoundBrowser::SetSoundBanks(const TArray<UObject*>& InSoundBanks)
{
	SoundBanks.Reset();

	TArray<UClass*> Classes;
	for (auto SoundBank : InSoundBanks)
	{
		if (auto Bank = Cast<UAtomSoundBank>(SoundBank))
		{
			SoundBanks.Emplace(Bank);
			Classes.AddUnique(Bank->GetSoundClass());
		}
	}

	// If no selection show normal sound cue assets like AtomSoundSimple
	if (Classes.IsEmpty())
	{
		Classes.Add(UAtomSoundBase::StaticClass());
	}

	// initialize asset picker
	FAssetPickerConfig AssetPickerConfig;
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
		Algo::ForEach(Classes, [&AssetPickerConfig](UClass* Class) { AssetPickerConfig.Filter.ClassPaths.Add(Class->GetClassPathName()); });
		//Algo::ForEach(SoundBanks, [&AssetPickerConfig](UAtomSoundBank* Bank) { AssetPickerConfig.Filter.SoftObjectPaths.Add(FSoftObjectPath(Bank)); });
#else
		Algo::ForEach(Classes, [&AssetPickerConfig](UClass* Class) { AssetPickerConfig.Filter.ClassNames.Add(Class->GetFName()); });
		//Algo::ForEach(SoundBanks, [&AssetPickerConfig](UAtomSoundBank* Bank) { AssetPickerConfig.Filter.ObjectPaths.Add(FAssetData(Bank).ObjectPath); });
#endif

		AssetPickerConfig.Filter.bRecursiveClasses = true;
		AssetPickerConfig.OnGetCustomSourceAssets = FOnGetCustomSourceAssets::CreateSP(this, &SAtomSoundBrowser::OnGetCustomSourceAssets);
		AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([](const FAssetData& AssetData)
		{
			auto AssetClass = AssetData.GetClass();
			return !AssetClass || !AssetClass->IsChildOf<UAtomSoundBase>();
		});
		AssetPickerConfig.AssetShowWarningText       = LOCTEXT("NoSoundFound", "No sound found.");
		AssetPickerConfig.bAllowDragging             = true;
		AssetPickerConfig.bAutohideSearchBar         = false;
		AssetPickerConfig.bAddFilterUI               = true;
		AssetPickerConfig.bCanShowClasses            = false;
		AssetPickerConfig.bCanShowDevelopersFolder   = false;
		AssetPickerConfig.bFocusSearchBoxWhenOpened  = false;
		AssetPickerConfig.bCanShowRealTimeThumbnails = true;
		AssetPickerConfig.InitialAssetViewType       = EAssetViewType::Tile;
		AssetPickerConfig.ThumbnailScale             = 0.1f;
		AssetPickerConfig.SaveSettingsName           = TEXT("CriWareAtomSoundBrowserSettings");

		// Hide asset info by default
		AssetPickerConfig.HiddenColumnNames.Add("Class");
		AssetPickerConfig.HiddenColumnNames.Add("Path");
		AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::ItemDiskSize.ToString());
		AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::VirtualizedData.ToString());
		// Hide sound cue/wave attributes by default
		AssetPickerConfig.HiddenColumnNames.Add("bLooping");
		AssetPickerConfig.HiddenColumnNames.Add("VolumeMultiplier");
		AssetPickerConfig.HiddenColumnNames.Add("PitchMultiplier");
		// Hide sound base attributes by default
		AssetPickerConfig.HiddenColumnNames.Add("SoundClassObject");
		AssetPickerConfig.HiddenColumnNames.Add("MaxDistance");
		AssetPickerConfig.HiddenColumnNames.Add(ColumnID_SoundBank.ToString());

		// Add custom columns
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Index,
			LOCTEXT("IndexColumn", "Index"),
			LOCTEXT("IndexColumnTooltip", "The index number of this sound in the bank."),
			UObject::FAssetRegistryTag::TT_Numerical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_SoundBank,
			LOCTEXT("SoundBankColumn", "Sound Bank"),
			LOCTEXT("SoundBankColumnTooltip", "The owner bank of this sound."),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Duration,
			LOCTEXT("DurationColumn", "Duration"),
			LOCTEXT("DurationColumnTooltip", "The sound duration."),
			UObject::FAssetRegistryTag::TT_Chronological,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_OneShot,
			LOCTEXT("OneShotColumn", "Is One Shot"),
			LOCTEXT("OneShotColumnTooltip", "Whether sound is Looping or One Shot."),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Streamed,
			LOCTEXT("StreamedColumn", "Streamed"),
			LOCTEXT("StreamedColumnTooltip", "Whether sound data is streamed from another file."),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Format,
			LOCTEXT("FormatColumn", "Format"),
			LOCTEXT("FormatColumnTooltip", "The sound encoded format."),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Channels,
			LOCTEXT("ChannelsColumn", "Channels"),
			LOCTEXT("ChannelsColumnTooltip", "The number of channels in the wave of this sound is using."),
			UObject::FAssetRegistryTag::TT_Numerical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Frames,
			LOCTEXT("FramesColumn", "Frames"),
			LOCTEXT("FramesColumnTooltip", "The number of frames in the wave of this sound."),
			UObject::FAssetRegistryTag::TT_Numerical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_SampleRate,
			LOCTEXT("SampleRateColumn", "Sample Rate"),
			LOCTEXT("SampleRateColumnTooltip", "The sample rate that the wave of this sound is using."),
			UObject::FAssetRegistryTag::TT_Numerical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);

		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Locked,
			LOCTEXT("LockedColumn", "Locked"),
			LOCTEXT("LockedColumnTooltip", "Whether sound parameters set through the engine are ignored at playback. (Cue Only)"),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBrowser::GetDisplayTextForCustomColumn)
		);

		AssetPickerConfig.OnAssetSelected = FOnAssetDoubleClicked::CreateSP(this, &SAtomSoundBrowser::OnAssetSelected);
		AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAtomSoundBrowser::OnAssetDoubleClicked);
		AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateSP(this, &SAtomSoundBrowser::OnAssetEnterPressed);
		AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SAtomSoundBrowser::OnGetAssetContextMenu);
		AssetPickerConfig.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
		//AssetPickerConfig.SetFilterDelegates.Add(&SetFilterDelegate);
	}

	auto& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		//.ToolTipText(LOCTEXT("DoubleClickToAddToolTip", "Double-click a sound to open it in the player."))
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		]
	];
}

void SAtomSoundBrowser::ShowSoundOpenFailedMessage()
{
	FNotificationInfo NotificationInfo(LOCTEXT("SoundOpenFailedError", "The sound failed to open. Check Output Log for details!"));
	{
		NotificationInfo.ExpireDuration = 2.0f;
	}

	FSlateNotificationManager::Get().AddNotification(NotificationInfo)->SetCompletionState(SNotificationItem::CS_Fail);
}

/* SAtomSoundBrowser callbacks
 *****************************************************************************/

void SAtomSoundBrowser::OnAssetDoubleClicked(const struct FAssetData& AssetData)
{
	// Edit
	UObject* Asset = AssetData.GetAsset();
	if (Asset && GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	}

	// play in special wave player for waves
	//OpenSoundAsset(AssetData.GetAsset());
}


void SAtomSoundBrowser::OnAssetEnterPressed(const TArray<FAssetData>& SelectedAssets)
{
	if (SelectedAssets.Num() > 0)
	{
		OpenSoundAsset(SelectedAssets[0].GetAsset());
	}
}

void SAtomSoundBrowser::OnAssetSelected(const FAssetData& SelectedAsset)
{
	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();

	if (OnSelectionChanged.IsBound())
	{
		TArray<UObject*> SelectedObjects;
		for (const FAssetData& AssetData : SelectedAssets)
		{
			if (AssetData.IsValid())
			{
				SelectedObjects.Add(AssetData.GetAsset());
			}
		}

		OnSelectionChanged.Execute(SelectedObjects);
	}
}

TSharedPtr<SWidget> SAtomSoundBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	if (SelectedAssets.Num() <= 0)
	{
		return nullptr;
	}

	UObject* SelectedAsset = SelectedAssets[0].GetAsset();
	if (SelectedAsset == nullptr)
	{
		return nullptr;
	}

	// Find common class for selected objects
	UClass* CommonClass = nullptr;
	for (int32 ObjIdx = 0; ObjIdx < SelectedAssets.Num(); ++ObjIdx)
	{
		if (CommonClass == nullptr)
		{
			CommonClass = SelectedAssets[ObjIdx].GetClass();
			continue;
		}

		// Update the CommonClass until we find a common shared class.
		UClass* Class = SelectedAssets[ObjIdx].GetClass();
		while (Class && !Class->IsChildOf(CommonClass))
		{
			CommonClass = CommonClass->GetSuperClass();
		}
	}

	FName MenuName = FName(TEXT("CriWare.AtomSoundBrowserContextMenu.") + CommonClass->GetName());
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName))
	{
		UContentBrowserAssetContextMenuContext* MenuContext = NewObject<UContentBrowserAssetContextMenuContext>();
		MenuContext->SelectedAssets = SelectedAssets;
		Menu->Context = FToolMenuContext(MenuContext);

		TArray<UObject*> SelectedObjects;
		for (const FAssetData& AssetData : SelectedAssets)
		{
			if (AssetData.IsValid())
			{
				SelectedObjects.Add(AssetData.GetAsset());
			}
		}

		// Add any type-specific context menu options
		AddAssetTypeMenuOptions(Menu, SelectedObjects, CommonClass);
		AddAssetActionMenuOptions(Menu, CommonClass);

		// Add quick access to common commands.
		AddCommonMenuOptions(Menu, SelectedObjects);

		// Add reference options
		AddReferenceMenuOptions(Menu);

		return UToolMenus::Get()->GenerateWidget(Menu);
	}

	return SNullWidget::NullWidget;
}

void SAtomSoundBrowser::AddAssetTypeMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects, const UClass* CommonClass)
{
	if (SelectedObjects.Num() > 0)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(CommonClass);

		// Label "AtomSoundAssetActions" section
		FToolMenuSection& Section = Menu->FindOrAddSection("AtomSoundAssetActions");
		if (AssetTypeActions.IsValid())
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), AssetTypeActions.Pin()->GetName());
			AssetTypeActions.Pin()->GetActions(SelectedObjects, Section);
		}
		else if (CommonClass)
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromName(CommonClass->GetFName()));
		}
		else
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromString(TEXT("Asset")));
		}
	}
}

void SAtomSoundBrowser::AddAssetActionMenuOptions(UToolMenu* Menu, const UClass* CommonClass)
{
	const UAssetDefinition* AssetDefintion = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(CommonClass);

	if (auto AtomAssetDefintion = Cast<UAtomAssetDefinition_Base>(AssetDefintion))
	{
		// Atom Sound Actions
		FToolMenuSection& Section = Menu->FindOrAddSection("AtomSoundAssetActions");
		Section.Label = FText::Format(LOCTEXT("AtomSoundAssetActionsSection", "{0} Actions"), FText::FromName(CommonClass->GetFName()));
		Section.Context = Menu->Context;
		AtomAssetDefintion->GetAssetActions(Section);
	}
}

void SAtomSoundBrowser::AddCommonMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects)
{
	FToolMenuSection& CommonSection = Menu->AddSection("AssetContextCommon", LOCTEXT("CommonMenuHeading", "Common"));

	// Edit
	CommonSection.AddMenuEntry(
		"EditAsset",
		LOCTEXT("EditAsset", "Edit..."),
		LOCTEXT("EditAssetTooltip", "Opens the selected item(s) for edit."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SAtomSoundBrowser::ExecuteEditItems, SelectedObjects),
			FCanExecuteAction::CreateSP(this, &SAtomSoundBrowser::IsAnythingSelected)
		)
	);

	// Property Matrix
	bool bCanUsePropertyMatrix = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").GetCanUsePropertyMatrix();

	if (bCanUsePropertyMatrix)
	{
		TAttribute<FText>::FGetter DynamicTooltipGetter;
		DynamicTooltipGetter.BindSP(this, &SAtomSoundBrowser::GetExecutePropertyMatrixTooltip);
		TAttribute<FText> DynamicTooltipAttribute = TAttribute<FText>::Create(DynamicTooltipGetter);

		CommonSection.AddMenuEntry(
			"PropertyMatrix",
			LOCTEXT("PropertyMatrix", "Bulk Edit via Property Matrix..."),
			DynamicTooltipAttribute,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SAtomSoundBrowser::ExecutePropertyMatrix, SelectedObjects),
				FCanExecuteAction::CreateSP(this, &SAtomSoundBrowser::IsAnythingSelected)
			)
		);
	}
}

void SAtomSoundBrowser::AddReferenceMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& ReferenceSection = Menu->AddSection("AssetContextReferences", LOCTEXT("ReferencesMenuHeading", "References"));
	// Copy Reference
	ReferenceSection.AddMenuEntryWithCommandList(FAtomSoundBankEditorCommands::Get().CopyReferences,
		Commands,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"));

	// Reference Viewer
	ReferenceSection.AddMenuEntryWithCommandList(FAtomSoundBankEditorCommands::Get().ViewReferences,
		Commands,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.ReferenceViewer"));

	// SizeMap
	ReferenceSection.AddMenuEntryWithCommandList(FAtomSoundBankEditorCommands::Get().ViewSizeMap,
		Commands,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.SizeMap"));
}

void SAtomSoundBrowser::ExecuteEditItems(TArray<UObject*> ObjectsToEdit)
{
	if (ObjectsToEdit.Num() > 0)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ObjectsToEdit);
	}
}

void SAtomSoundBrowser::ExecutePropertyMatrix(TArray<UObject*> ObjectsForPropertiesMenu)
{
	if (ObjectsForPropertiesMenu.Num() > 0)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), ObjectsForPropertiesMenu);
	}
}

FText SAtomSoundBrowser::GetExecutePropertyMatrixTooltip() const
{
	FText ResultTooltip;
	if (IsAnythingSelected())
	{
		ResultTooltip = LOCTEXT("PropertyMatrixTooltip", "Opens the property matrix editor for the selected sound assets.");
	}
	return ResultTooltip;
}

void SAtomSoundBrowser::ExecuteCopyReference()
{
	TArray<FAssetData> AssetsToReference = GetCurrentSelectionDelegate.Execute();
	if (AssetsToReference.Num() > 0)
	{
		FString ClipboardText;
		for (auto AssetData : AssetsToReference)
		{
			if (AssetData.IsValid())
			{
				if (ClipboardText.Len() > 0)
				{
					ClipboardText += LINE_TERMINATOR;
				}
				ClipboardText += AssetData.GetExportTextName();
			}
		}
		FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
	}
}

void SAtomSoundBrowser::ExecuteReferenceViewer()
{
	TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
	}
}

void SAtomSoundBrowser::ExecuteSizeMap()
{
	TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenSizeMapUI(AssetIdentifiers);
	}
}

bool SAtomSoundBrowser::IsAnythingSelected() const
{
	TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
