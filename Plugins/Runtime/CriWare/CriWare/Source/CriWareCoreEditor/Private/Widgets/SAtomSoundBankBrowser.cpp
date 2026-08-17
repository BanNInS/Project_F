/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : CriWareCoreEditor
* File     : SAtomSoundBankBrowser.cpp
*
****************************************************************************/

#include "SAtomSoundBankBrowser.h"

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
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWaveBank.h"

#include "AtomEditorStyle.h"
#include "Models/AtomSoundBankEditorCommands.h"
#include "Widgets/SAtomSoundBrowser.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#define LOCTEXT_NAMESPACE "SAtomSoundBankBrowser"

const FName SAtomSoundBankBrowser::AssetPickerAssetContextMenuName = "AtomSoundBank.AssetPickerAssetContextMenu";
const FName SAtomSoundBankBrowser::ColumnID_WaveBank(TEXT("WaveBank"));
const FName SAtomSoundBankBrowser::ColumnID_Loading(TEXT("Loading"));

/* SAtomSoundBankBrowser interface
 *****************************************************************************/

void SAtomSoundBankBrowser::Construct(const FArguments& InArgs, UAtomSoundBank& InSoundBank)
{
	SoundBank = &InSoundBank;
	OnSelectionChanged = InArgs._OnSelectionChanged;

	Commands = MakeShareable(new FUICommandList());
	Commands->MapAction(FAtomSoundBankEditorCommands::Get().CopyReferences, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::ExecuteCopyReference),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::IsAnythingSelected)
	));

	Commands->MapAction(FAtomSoundBankEditorCommands::Get().ViewReferences, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::ExecuteReferenceViewer),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::IsAnythingSelected)
	));

	Commands->MapAction(FAtomSoundBankEditorCommands::Get().ViewSizeMap, FUIAction(
		FExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::ExecuteSizeMap),
		FCanExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::IsAnythingSelected)
	));

	FName PackagePath = FName(SoundBank->GetPackage()->GetPathName());

	// initialize asset picker
	FAssetPickerConfig AssetPickerConfig;
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
		AssetPickerConfig.Filter.ClassPaths.Add(UAtomSoundBank::StaticClass()->GetClassPathName());
#else
		AssetPickerConfig.Filter.ClassNames.Add(UAtomSoundBank::StaticClass()->GetFName());
#endif
		AssetPickerConfig.Filter.bRecursiveClasses = true;

		AssetPickerConfig.AssetShowWarningText     = LOCTEXT("NoSoundBankFound", "No sound bank found.");
		AssetPickerConfig.bAllowDragging           = true;
		AssetPickerConfig.bAutohideSearchBar       = false;
		AssetPickerConfig.bAddFilterUI             = true;
		AssetPickerConfig.bCanShowClasses          = false;
		AssetPickerConfig.bCanShowDevelopersFolder = false;
		AssetPickerConfig.InitialAssetViewType     = EAssetViewType::Column;
		AssetPickerConfig.ThumbnailScale           = 0.1f;
		AssetPickerConfig.InitialAssetSelection    = SoundBank;
		AssetPickerConfig.SaveSettingsName = TEXT("CriWareAtomSoundBankBrowserSettings");

		// Hide asset info by default
		AssetPickerConfig.HiddenColumnNames.Add("Class");
		AssetPickerConfig.HiddenColumnNames.Add("Path");
		AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::ItemDiskSize.ToString());
		AssetPickerConfig.HiddenColumnNames.Add(ContentBrowserItemAttributes::VirtualizedData.ToString());
		// Hide sound bank attributes by default

		// Add custom columns
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_WaveBank,
			LOCTEXT("IndexColumn", "WaveBanks"),
			LOCTEXT("OneShotColumnTooltip", "If sound bank use wavebanks this show the fisrt WaveBank name."),
			UObject::FAssetRegistryTag::TT_Numerical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBankBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBankBrowser::GetDisplayTextForCustomColumn)
		);
		AssetPickerConfig.CustomColumns.Emplace(ColumnID_Loading,
			LOCTEXT("DurationColumn", "Loading Behavior"),
			LOCTEXT("DurationColumnTooltip", "The loading behavior setting of the sound bank."),
			UObject::FAssetRegistryTag::TT_Alphabetical,
			FOnGetCustomAssetColumnData::CreateSP(this, &SAtomSoundBankBrowser::GetStringValueForCustomColumn),
			FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAtomSoundBankBrowser::GetDisplayTextForCustomColumn)
		);

		AssetPickerConfig.OnAssetSelected = FOnAssetDoubleClicked::CreateSP(this, &SAtomSoundBankBrowser::OnAssetSelected);
		AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAtomSoundBankBrowser::OnAssetDoubleClicked);
		AssetPickerConfig.OnAssetEnterPressed = FOnAssetEnterPressed::CreateSP(this, &SAtomSoundBankBrowser::OnAssetEnterPressed);
		AssetPickerConfig.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SAtomSoundBankBrowser::OnGetAssetContextMenu);
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

FString SAtomSoundBankBrowser::GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	return GetDisplayTextForCustomColumn(AssetData, ColumnName).ToString();
}

FText SAtomSoundBankBrowser::GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const
{
	FText OutValue;

	if (ColumnName == ColumnID_WaveBank)
	{
		if (UAtomSoundBank* Bank = Cast<UAtomSoundBank>(AssetData.GetAsset()))
		{
			OutValue = LOCTEXT("WaveBankNotAvailable", "Not Available");

			if (UAtomCueSheet* CueSheet = Cast<UAtomCueSheet>(Bank))
			{
				if (CueSheet->WaveBanks.Num() > 0)
				{
					UAtomWaveBank* WaveBank = CueSheet->WaveBanks[0];
					OutValue = WaveBank ? FText::FromName(WaveBank->GetFName()) : LOCTEXT("WaveBankNotSet", "Not Set");
				}
			}
		}
	}
	else if (ColumnName == ColumnID_Loading)
	{
		if (UAtomSoundBank* Bank = Cast<UAtomSoundBank>(AssetData.GetAsset()))
		{
			OutValue = FText::FromString(::ToString(Bank->LoadingBehavior));
		}
	}

	return OutValue;
}

/* SAtomSoundBankBrowser callbacks
 *****************************************************************************/

void SAtomSoundBankBrowser::OnAssetDoubleClicked(const struct FAssetData& AssetData)
{
	// Edit
	UObject* Asset = AssetData.GetAsset();
	if (Asset && GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	}
}


void SAtomSoundBankBrowser::OnAssetEnterPressed(const TArray<FAssetData>& SelectedAssets)
{

}

void SAtomSoundBankBrowser::OnAssetSelected(const FAssetData& SelectedAsset)
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

TSharedPtr<SWidget> SAtomSoundBankBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
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

	FName MenuName = FName(TEXT("CriWare.AtomSoundBankBrowserContextMenu.") + CommonClass->GetName());
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

		// Add file context menu options
		AddAssetFileContextMenuOptions(Menu);

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

void SAtomSoundBankBrowser::AddAssetFileContextMenuOptions(UToolMenu* Menu)
{
	
	FToolMenuSection& FileSection = Menu->AddSection("AssetContext", LOCTEXT("AssetSectionLabel", "Asset"));

	TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	FileSection.AddMenuEntry(
		"BrowseToAsset",
		LOCTEXT("Browse", "Browse to Asset"),
		LOCTEXT("BrowseTooltip", "Browses to the associated asset and selects it in the most recently used Content Browser (summoning one if necessary)"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser.Small"),
		FUIAction(
			FExecuteAction::CreateLambda([SelectedAssets]()
				{
					if (SelectedAssets.Num() > 0)
					{
						FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
						ContentBrowserModule.Get().SyncBrowserToAssets(SelectedAssets);
					}
				}),
			FCanExecuteAction::CreateLambda([]() { return true; })
		)
	);
}

void SAtomSoundBankBrowser::AddAssetTypeMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects, const UClass* CommonClass)
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

void SAtomSoundBankBrowser::AddAssetActionMenuOptions(UToolMenu* Menu, const UClass* CommonClass)
{
	const UAssetDefinition* AssetDefintion = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(CommonClass);

	if (auto AtomAssetDefintion = Cast<UAtomAssetDefinition_Base>(AssetDefintion))
	{
		// Atom Sound Bank Actions
		FToolMenuSection& Section = Menu->FindOrAddSection("AtomSoundBankAssetActions");
		Section.Label = FText::Format(LOCTEXT("AtomSoundBankAssetActionsSection", "{0} Actions"), FText::FromName(CommonClass->GetFName()));
		Section.Context = Menu->Context;
		AtomAssetDefintion->GetAssetActions(Section);
	}
}

void SAtomSoundBankBrowser::AddCommonMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects)
{
	FToolMenuSection& CommonSection = Menu->AddSection("AssetContextCommon", LOCTEXT("CommonMenuHeading", "Common"));

	// Edit - disable: used by editor toolkit only
	/*CommonSection.AddMenuEntry(
		"EditAsset",
		LOCTEXT("EditAsset", "Edit..."),
		LOCTEXT("EditAssetTooltip", "Opens the selected item(s) for edit."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::ExecuteEditItems, SelectedObjects),
			FCanExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::IsAnythingSelected)
		)
	);*/

	// Property Matrix
	bool bCanUsePropertyMatrix = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").GetCanUsePropertyMatrix();

	if (bCanUsePropertyMatrix)
	{
		TAttribute<FText>::FGetter DynamicTooltipGetter;
		DynamicTooltipGetter.BindSP(this, &SAtomSoundBankBrowser::GetExecutePropertyMatrixTooltip);
		TAttribute<FText> DynamicTooltipAttribute = TAttribute<FText>::Create(DynamicTooltipGetter);

		CommonSection.AddMenuEntry(
			"PropertyMatrix",
			LOCTEXT("PropertyMatrix", "Bulk Edit via Property Matrix..."),
			DynamicTooltipAttribute,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(
				FExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::ExecutePropertyMatrix, SelectedObjects),
				FCanExecuteAction::CreateSP(this, &SAtomSoundBankBrowser::IsAnythingSelected)
			)
		);
	}
}

void SAtomSoundBankBrowser::AddReferenceMenuOptions(UToolMenu* Menu)
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

void SAtomSoundBankBrowser::ExecuteEditItems(TArray<UObject*> ObjectsToEdit)
{
	if (ObjectsToEdit.Num() > 0)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ObjectsToEdit);
	}
}

void SAtomSoundBankBrowser::ExecutePropertyMatrix(TArray<UObject*> ObjectsForPropertiesMenu)
{
	if (ObjectsForPropertiesMenu.Num() > 0)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), ObjectsForPropertiesMenu);
	}
}

FText SAtomSoundBankBrowser::GetExecutePropertyMatrixTooltip() const
{
	FText ResultTooltip;
	if (IsAnythingSelected())
	{
		ResultTooltip = LOCTEXT("PropertyMatrixTooltip", "Opens the property matrix editor for the selected sound assets.");
	}
	return ResultTooltip;
}

void SAtomSoundBankBrowser::ExecuteCopyReference()
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

void SAtomSoundBankBrowser::ExecuteReferenceViewer()
{
	TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
	}
}

void SAtomSoundBankBrowser::ExecuteSizeMap()
{
	TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenSizeMapUI(AssetIdentifiers);
	}
}

bool SAtomSoundBankBrowser::IsAnythingSelected() const
{
	TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
