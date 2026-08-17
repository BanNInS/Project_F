/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankDetails.cpp
 *
 ****************************************************************************/

#include "AtomSoundBankDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "ToolMenus.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Commands/GenericCommands.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "HAL/PlatformApplicationMisc.h"
#include "AssetManagerEditorModule.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTypeActions.h"
#include "Runtime/Launch/Resources/Version.h"

#include "Atom/AtomSoundBank.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"

#include "Widgets/SAssetView.h"

#define LOCTEXT_NAMESPACE	"AtomSoundBankDetailsInspector"

const FName FAtomSoundBankDetails::ColumnID_Index(TEXT("Index"));
const FName FAtomSoundBankDetails::ColumnID_Sound(TEXT("Sound"));
const FName FAtomSoundBankDetails::ColumnID_Duration(TEXT("Duration"));
const FName FAtomSoundBankDetails::ColumnID_OneShot(TEXT("OneShot"));
const FName FAtomSoundBankDetails::ColumnID_Streamed(TEXT("Streamed"));
const FName FAtomSoundBankDetails::ColumnID_Format(TEXT("Format"));
const FName FAtomSoundBankDetails::ColumnID_Channels(TEXT("Channels"));
const FName FAtomSoundBankDetails::ColumnID_Frames(TEXT("Frames"));
const FName FAtomSoundBankDetails::ColumnID_SampleRate(TEXT("SampleRate"));

TSharedRef<IDetailCustomization> FAtomSoundBankDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundBankDetails);
}

void FAtomSoundBankDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// get category and handle to customize
	FName SoundArrayCategoryName;
	TSharedPtr<IPropertyHandleArray> SoundArrayHandle;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	DetailViewParentWidget = DetailLayout.GetDetailsViewSharedPtr()->GetParentWidget();
#else
	DetailViewParentWidget = DetailLayout.GetDetailsView()->GetParentWidget();
#endif

	TSharedPtr<IPropertyHandle> SoundCuesHandle = DetailLayout.GetProperty("SoundCues");
	if (SoundCuesHandle->IsValidHandle())
	{
		SoundArrayCategoryName = TEXT("Cues");
		SoundArrayHandle = SoundCuesHandle->AsArray();
		SoundCuesHandle->MarkHiddenByCustomization();
	}

	TSharedPtr<IPropertyHandle> SoundWavesHandle = DetailLayout.GetProperty("SoundWaves");
	if (SoundWavesHandle->IsValidHandle())
	{
		SoundArrayCategoryName = TEXT("Waves");
		SoundArrayHandle = SoundWavesHandle->AsArray();
		SoundWavesHandle->MarkHiddenByCustomization();
	}

	TSharedPtr<IPropertyHandle> WaveBanksHandle = DetailLayout.GetProperty("WaveBanks");
	if (WaveBanksHandle->IsValidHandle())
	{
		TSharedPtr<IPropertyHandleArray> ArrayHandle = WaveBanksHandle->AsArray();
		if (ArrayHandle.IsValid())
		{
			uint32 NumElements;
			ArrayHandle->GetNumElements(NumElements);

			for (uint32 Index = 0; Index < NumElements; Index++)
			{
				TSharedPtr<IPropertyHandle> WaveBankHandle = ArrayHandle->GetElement(Index);
				if (WaveBankHandle->IsValidHandle())
				{
					WaveBankHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FAtomSoundBankDetails::OnSoundListChanged));
				}
			}
		}
	}

	// customize the sound array
	if (SoundArrayHandle.IsValid())
	{
		IDetailCategoryBuilder& SoundArrayCategory = DetailLayout.EditCategory(SoundArrayCategoryName);

		// collect the soundbase objects
		uint32 NumSounds;
		SoundArrayHandle->GetNumElements(NumSounds);

		if (NumSounds > 0)
		{
			for (uint32 SoundIndex = 0; SoundIndex < NumSounds; SoundIndex++)
			{
				TSharedRef<IPropertyHandle> SoundHandle = SoundArrayHandle->GetElement(SoundIndex);

				UObject* SoundBaseObject = nullptr;
				SoundHandle->GetValue(SoundBaseObject);
				if (UAtomSoundBase* Sound = Cast<UAtomSoundBase>(SoundBaseObject))
				{
					SoundListItems.Add(MakeShared<FAtomSoundListItem>(FAtomSoundListItem{ SoundIndex, Sound }));
				}
			}
		}

		// add the list view of the sounds
		auto Row = SoundArrayCategory.AddCustomRow(FText())
			.WholeRowContent()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(4.0f, 4.0f, 0.f, 0.f)
				.MaxHeight(TAttribute<float>::Create(TAttribute<float>::FGetter::CreateSP(this, &FAtomSoundBankDetails::GetListViewMaxHeight)))
				[
					SAssignNew(SoundListView, SListView<TSharedPtr<FAtomSoundListItem>>)
					//.ItemHeight(24)
					.ListItemsSource(&SoundListItems)
					.OnGenerateRow(this, &FAtomSoundBankDetails::OnGenerateWidgetForAtomSoundList)
					//.OnContextMenuOpening(this, &FAtomSoundBankDetails::OnContextMenuOpening)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(ColumnID_Index)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderDuration", "Index"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Index)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_Sound)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderSound", "Sound"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Sound)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.3)
						/* + SHeaderRow::Column(ColumnID_Duration)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderDuration", "Duration"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Duration)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_OneShot)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderOneShot", "One Shot"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_OneShot)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_Streamed)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderStreamed", "Streamed"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Streamed)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_Format)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderFormat", "Format"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Format)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_Channels)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderNumChannels", "Channels"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Channels)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_Samples)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderNumSamples", "Samples"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_Samples)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)
						+ SHeaderRow::Column(ColumnID_SampleRate)
						.DefaultLabel(LOCTEXT("AtomSoundListHeaderSampleRate", "Sample Rate"))
						.SortMode(this, &FAtomSoundBankDetails::GetColumnSortMode, ColumnID_SampleRate)
						.OnSort(this, &FAtomSoundBankDetails::OnColumnSortChanged)
						.FillWidth(0.15)*/
					)
				]
			];
	}
}

float FAtomSoundBankDetails::GetListViewMaxHeight() const
{
	//if (DetailViewParentWidget.IsValid())
	//{
	//	float DesiredHeight = DetailViewParentWidget->GetDesiredSize().Y;
	//	UE_LOG(LogTemp, Warning, TEXT("H: %f"), DesiredHeight);
	//	return FMath::Max(DesiredHeight - 400.0f, 500.0f);
	//}

	return 350.0f;
}

TSharedRef<ITableRow> FAtomSoundBankDetails::OnGenerateWidgetForAtomSoundList(TSharedPtr<FAtomSoundListItem> InListItem, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	return SNew(SAtomSoundListTableRow, InOwnerTableView)
		.Item(InListItem);
}

TSharedPtr<SWidget> FAtomSoundBankDetails::OnContextMenuOpening()
{
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("CriWare.AtomSoundContextMenu"))
	{
		// Add any type-specific context menu options
		AddAssetTypeMenuOptions(Menu);

		// Add quick access to common commands.
		AddCommonMenuOptions(Menu);

		// Add reference options
		AddReferenceMenuOptions(Menu);

		return UToolMenus::Get()->GenerateWidget(Menu);
	}

	return SNullWidget::NullWidget;
}

void FAtomSoundBankDetails::AddAssetTypeMenuOptions(UToolMenu* Menu)
{
	TArray<UObject*> SelectedObjects;
	GetSelectedAssets(SelectedObjects);

	if (SelectedObjects.Num() > 0)
	{
		UObject* Object = SelectedObjects[0];

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Object->GetClass());

		// Label "AtomSoundAssetActions" section
		FToolMenuSection& Section = Menu->FindOrAddSection("AtomSoundAssetActions");
		if (AssetTypeActions.IsValid())
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), AssetTypeActions.Pin()->GetName());
			AssetTypeActions.Pin()->GetActions(SelectedObjects, Section);
		}
		else if (Object->GetClass())
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromName(Object->GetClass()->GetFName()));
		}
		else
		{
			Section.Label = FText::Format(LOCTEXT("AssetSpecificOptionsMenuHeading", "{0} Actions"), FText::FromString(TEXT("Asset")));
		}
	}
}

void FAtomSoundBankDetails::AddCommonMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& CommonSection = Menu->AddSection("AssetContextCommon", LOCTEXT("CommonMenuHeading", "Common"));

	// Edit
	CommonSection.AddMenuEntry(
		"EditAsset",
		LOCTEXT("EditAsset", "Edit..."),
		LOCTEXT("EditAssetTooltip", "Opens the selected item(s) for edit."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAtomSoundBankDetails::ExecuteEditItems),
			FCanExecuteAction::CreateSP(this, &FAtomSoundBankDetails::CanExecuteEditItems)
		)
	);

	// Property Matrix
	bool bCanUsePropertyMatrix = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").GetCanUsePropertyMatrix();

	if (bCanUsePropertyMatrix)
	{
		TAttribute<FText>::FGetter DynamicTooltipGetter;
		DynamicTooltipGetter.BindSP(this, &FAtomSoundBankDetails::GetExecutePropertyMatrixTooltip);
		TAttribute<FText> DynamicTooltipAttribute = TAttribute<FText>::Create(DynamicTooltipGetter);

		CommonSection.AddMenuEntry(
			"PropertyMatrix",
			LOCTEXT("PropertyMatrix", "Bulk Edit via Property Matrix..."),
			DynamicTooltipAttribute,
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAtomSoundBankDetails::ExecutePropertyMatrix),
				FCanExecuteAction::CreateSP(this, &FAtomSoundBankDetails::CanExecutePropertyMatrix)
			)
		);
	}
}

void FAtomSoundBankDetails::AddReferenceMenuOptions(UToolMenu* Menu)
{
	FToolMenuSection& ReferenceSection = Menu->AddSection("AssetContextReferences", LOCTEXT("ReferencesMenuHeading", "References"));

	// Copy Reference
	ReferenceSection.AddMenuEntry(
		"CopyReference",
		LOCTEXT("CopyReference", "Copy Reference"),
		LOCTEXT("CopyReferenceTooltip", "Copies reference paths for the selected sound assets to the clipboard."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateSP(this, &FAtomSoundBankDetails::ExecuteCopyReference))
	);

	// Reference Viewer
	ReferenceSection.AddMenuEntry(
		"ReferenceViewer",
		LOCTEXT("ReferenceViewer", "Reference Viewer..."),
		LOCTEXT("ReferenceViewerTooltip", "Launches the reference viewer showing the selected parent sound bank assets' references"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.ReferenceViewer"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FAtomSoundBankDetails::ExecuteReferenceViewer),
			FCanExecuteAction::CreateSP(this, &FAtomSoundBankDetails::CanExecuteReferenceViewer)
		)
	);

	//FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::R)
}

void FAtomSoundBankDetails::GetSelectedAssets(TArray<UObject*>& Assets)
{
	// grab selected sounds
	if (SoundListView->IsEnabled())
	{
		TArray<TSharedPtr<FAtomSoundListItem>> Selection = SoundListView->GetSelectedItems();
		for (auto Item : Selection)
		{
			Assets.Add(Item->Sound);
		}
	}
}

void FAtomSoundBankDetails::ExecutePlaySound()
{

}

bool FAtomSoundBankDetails::CanExecutePlaySound() const
{
	return true;
}

void FAtomSoundBankDetails::ExecuteStopSound()
{

}

bool FAtomSoundBankDetails::CanExecuteStopSound() const
{
	return true;
}

void FAtomSoundBankDetails::ExecuteEditItems()
{
	TArray<UObject*> ObjectsToEdit;
	GetSelectedAssets(ObjectsToEdit);

	if (ObjectsToEdit.Num() > 0)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ObjectsToEdit);
	}
}

bool FAtomSoundBankDetails::CanExecuteEditItems() const
{
	// todo check if soundbank is editable
	return true;
}

void FAtomSoundBankDetails::ExecutePropertyMatrix()
{
	TArray<UObject*> ObjectsForPropertiesMenu;
	GetSelectedAssets(ObjectsForPropertiesMenu);

	if (ObjectsForPropertiesMenu.Num() > 0)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), ObjectsForPropertiesMenu);
	}
}

bool FAtomSoundBankDetails::CanExecutePropertyMatrix() const
{
	return true;
}

FText FAtomSoundBankDetails::GetExecutePropertyMatrixTooltip() const
{
	FText ResultTooltip;
	if (CanExecutePropertyMatrix())
	{
		ResultTooltip = LOCTEXT("PropertyMatrixTooltip", "Opens the property matrix editor for the selected sound assets.");
	}
	return ResultTooltip;
}

void FAtomSoundBankDetails::ExecuteCopyReference()
{
	TArray<UObject*> ObjectsToReference;
	GetSelectedAssets(ObjectsToReference);

	if (ObjectsToReference.Num() > 0)
	{
		FString ClipboardText;
		for (auto Object : ObjectsToReference)
		{
			const FAssetData AssetData = FAssetData(Object);
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

void FAtomSoundBankDetails::ExecuteReferenceViewer()
{
	TArray<UObject*> ObjectsToReference;
	GetSelectedAssets(ObjectsToReference);

	TArray<FAssetData> Assets;
	for (auto Object : ObjectsToReference)
	{
		const FAssetData AssetData = FAssetData(Object);
		if (AssetData.IsValid())
		{
			Assets.Add(AssetData);
		}
	}

	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
	}
}

bool FAtomSoundBankDetails::CanExecuteReferenceViewer() const
{
	return SoundListView->GetNumItemsSelected() > 0;
}

EColumnSortMode::Type FAtomSoundBankDetails::GetColumnSortMode(const FName ColumnID) const
{
	if (ColumnID == SortColumnID)
	{
		return SortMode;
	}

	return EColumnSortMode::None;
}

void FAtomSoundBankDetails::OnColumnSortChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnID, const EColumnSortMode::Type NewSortMode)
{
	// If already sorted, flip direction
	if (SortColumnID == ColumnID)
	{
		SortMode = (SortMode == EColumnSortMode::Descending) ? EColumnSortMode::Ascending : EColumnSortMode::Descending;
	}
	else
	{
		SortColumnID = ColumnID;
	}

	SortItemList();

	// When underlying array changes, refresh list
	SoundListView->RequestListRefresh();
}

void FAtomSoundBankDetails::SortItemList()
{
	auto Compare = [&](FAtomSoundListItem* Lhs, FAtomSoundListItem* Rhs) -> bool
	{
		if (Lhs && Rhs && Lhs->Sound && Rhs->Sound)
		{
			if (SortColumnID == ColumnID_Index)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Index < Rhs->Index : Lhs->Index > Rhs->Index;
			}
			else if (SortColumnID == ColumnID_Sound)
			{
				UAtomSoundCue* SoundCueLhs = Cast<UAtomSoundCue>(Lhs->Sound);
				UAtomSoundCue* SoundCueRhs = Cast<UAtomSoundCue>(Rhs->Sound);
				if (SoundCueLhs && SoundCueRhs)
				{
					return SortMode == EColumnSortMode::Ascending ? SoundCueLhs->GetCueName() < SoundCueRhs->GetCueName() : SoundCueLhs->GetCueName() > SoundCueRhs->GetCueName();
				}

				return SortMode == EColumnSortMode::Ascending ? Lhs->Index < Rhs->Index : Lhs->Index > Rhs->Index;
			}
			else if (SortColumnID == ColumnID_Duration)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->GetDuration() < Rhs->Sound->GetDuration() : Lhs->Sound->GetDuration() > Rhs->Sound->GetDuration();
			}
			else if (SortColumnID == ColumnID_OneShot)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->IsOneShot() && !Rhs->Sound->IsOneShot() : !Lhs->Sound->IsOneShot() && Rhs->Sound->IsOneShot();
			}
			else if (SortColumnID == ColumnID_Streamed)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->WaveInfo.bIsStreamed && !Rhs->Sound->WaveInfo.bIsStreamed : !Lhs->Sound->WaveInfo.bIsStreamed && Rhs->Sound->WaveInfo.bIsStreamed;
			}
			else if (SortColumnID == ColumnID_Format)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->WaveInfo.Format < Rhs->Sound->WaveInfo.Format : Lhs->Sound->WaveInfo.Format > Rhs->Sound->WaveInfo.Format;
			}
			else if (SortColumnID == ColumnID_Channels)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->WaveInfo.NumChannels < Rhs->Sound->WaveInfo.NumChannels : Lhs->Sound->WaveInfo.NumChannels > Rhs->Sound->WaveInfo.NumChannels;
			}
			else if (SortColumnID == ColumnID_Frames)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->WaveInfo.NumFrames < Rhs->Sound->WaveInfo.NumFrames : Lhs->Sound->WaveInfo.NumFrames > Rhs->Sound->WaveInfo.NumFrames;
			}
			else if (SortColumnID == ColumnID_SampleRate)
			{
				return SortMode == EColumnSortMode::Ascending ? Lhs->Sound->WaveInfo.SampleRate < Rhs->Sound->WaveInfo.SampleRate : Lhs->Sound->WaveInfo.SampleRate > Rhs->Sound->WaveInfo.SampleRate;
			}
		}

		return false;
	};

	SoundListItems.Sort([&](const TSharedPtr<FAtomSoundListItem>& Lhs, const TSharedPtr<FAtomSoundListItem>& Rhs)
	{
		return Compare(Lhs.Get(), Rhs.Get());
	});
}

void SAtomSoundListTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	Item = InArgs._Item;

	SMultiColumnTableRow<TSharedPtr<FAtomSoundListItem>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list row. */
TSharedRef<SWidget> SAtomSoundListTableRow::GenerateWidgetForColumn(const FName& ColumnID)
{
	if (FAtomSoundListItem* SoundListItem = Item.Get())
	{
		if (UAtomSoundBase* Sound = SoundListItem->Sound)
		{
			auto GetColumnWidget = [&]() -> TSharedRef<SWidget>
			{
				if (ColumnID == FAtomSoundBankDetails::ColumnID_Index)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(FString::FromInt(SoundListItem->Index)));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_Sound)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(SoundListItem->Sound->GetName()));
					/*return SNew(SAssetView)
						.ThumbnailSize(64.0f)
						.Object(Sound)
						.OpenAssetOnDoubleClick(true)
						.AllowAssetDragging(true);*/
				}
				/*else if (ColumnID == FAtomSoundBankDetails::ColumnID_Duration)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(FTimespan::FromSeconds(Sound->GetDuration()).ToString(TEXT("%h:%m:%s.%f")).RightChop(1)));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_OneShot)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(Sound->IsOneShot() ? TEXT("One Shot") : TEXT("Looping")));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_Streamed)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(Sound->WaveInfo.bIsStreamed ? TEXT("From WaveBank") : TEXT("No")));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_Format)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(::ToString(Sound->WaveInfo.Format)));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_Channels)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(FString::FromInt(Sound->WaveInfo.NumChannels)));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_Samples)
				{
					return SNew(STextBlock)
						.Text(FText::FromString(FString::FromInt(Sound->WaveInfo.NumSamples)));
				}
				else if (ColumnID == FAtomSoundBankDetails::ColumnID_SampleRate)
				{
					return SNew(STextBlock)
						.Text(FText::Format(FText::FromString("{0} Hz"), { FText::FromString(FString::FromInt(Sound->WaveInfo.SampleRate)) }));
				}*/

				return SNullWidget::NullWidget;
			};

			const FMargin RowPadding(3, 0, 0, 0);

			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(RowPadding)
				[
					GetColumnWidget()
				];
		}
	}

	return SNullWidget::NullWidget;
}

void FAtomSoundBankDetails::OnSoundListChanged()
{
	SoundListView->RebuildList();
}

#undef LOCTEXT_NAMESPACE
