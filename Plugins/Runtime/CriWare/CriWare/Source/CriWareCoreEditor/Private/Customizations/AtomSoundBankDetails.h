/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankDetails.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

// Forward declarations
class UAtomSoundBank;
class UAtomSoundBase;
class UToolMenu;

struct FAtomSoundListItem
{
public:

	uint32 Index = 0;
	TObjectPtr<UAtomSoundBase> Sound = nullptr;
};

class CRIWARECOREEDITOR_API FAtomSoundBankDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

public:

	static const FName ColumnID_Index;
	static const FName ColumnID_Sound;
	static const FName ColumnID_Duration;
	static const FName ColumnID_OneShot;
	static const FName ColumnID_Streamed;
	static const FName ColumnID_Format;
	static const FName ColumnID_Channels;
	static const FName ColumnID_Frames;
	static const FName ColumnID_SampleRate;

	// Current way we are sorting sounds.
	FName SortColumnID = ColumnID_Index;
	// Current way we are setting sort direction.
	EColumnSortMode::Type SortMode = EColumnSortMode::Descending;

private:

	TSharedPtr<SListView<TSharedPtr<FAtomSoundListItem>>> SoundListView;
	TArray<TSharedPtr<FAtomSoundListItem>> SoundListItems;

	TSharedPtr<SWidget> DetailViewParentWidget;

	float GetListViewMaxHeight() const;
	TSharedRef<ITableRow> OnGenerateWidgetForAtomSoundList(TSharedPtr<FAtomSoundListItem> InListItem, const TSharedRef<STableViewBase>& InOwnerTableView);
	TSharedPtr<SWidget> OnContextMenuOpening();
	EColumnSortMode::Type GetColumnSortMode(const FName ColumnID) const;
	void OnColumnSortChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnID, const EColumnSortMode::Type NewSortMode);
	void SortItemList();
	void OnSoundListChanged();

	void AddAssetTypeMenuOptions(UToolMenu* Menu);
	void AddCommonMenuOptions(UToolMenu* Menu);
	void AddReferenceMenuOptions(UToolMenu* Menu);

	void GetSelectedAssets(TArray<UObject*>& Assets);

	void ExecutePlaySound();
	bool CanExecutePlaySound() const;

	void ExecuteStopSound();
	bool CanExecuteStopSound() const;
	
	void ExecuteEditItems();
	bool CanExecuteEditItems() const;
	
	void ExecutePropertyMatrix();
	bool CanExecutePropertyMatrix() const;
	FText GetExecutePropertyMatrixTooltip() const;

	void ExecuteCopyReference();

	void ExecuteReferenceViewer();
	bool CanExecuteReferenceViewer() const;
};

/** Widget that represents a row in the AtomSoundBankDetails' list view. Generates widgets for each column on demand. */
class SAtomSoundListTableRow
	: public SMultiColumnTableRow<TSharedPtr<FAtomSoundListItem>>
{
public:

	SLATE_BEGIN_ARGS(SAtomSoundListTableRow) {}

		/** The list item for this row */
		SLATE_ARGUMENT(TSharedPtr<FAtomSoundListItem>, Item)

	SLATE_END_ARGS()

	/** Construct function for this widget */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/** The item associated with this row of data */
	TSharedPtr<FAtomSoundListItem> Item;
};
