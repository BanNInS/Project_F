/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomConfigFactoryDialog.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

namespace EAtomStatisticsExpandMode
{
	enum Type
	{
		ExpandAll,
		CollapseAll
	};
}

namespace EAtomStatisticsItemType
{
	enum Type
	{
		Runtime,
		Rack,
		Listener,
		VoicePool,
		ActiveSound,
		PlaybackInstance,
		VirtualLoop,
		ConcurrencyGroup
	};
}

/** Default metrics for outliner tree items */
struct FAtomStatisticsDefaultTreeItemMetrics
{
	static int32	RowHeight() { return 20; };
	static int32	IconSize() { return 16; };
	static FMargin	IconPadding() { return FMargin(0.f, 1.f, 6.f, 1.f); };
};

// todo: an interfce and classes for each item type
struct FAtomStatisticsTreeItem
{
	TArray<TSharedPtr<FAtomStatisticsTreeItem>> Children;

	EAtomStatisticsItemType::Type Type;
	FString			Label;
	FString			Value;
	int32			ID;
	uint64			ComponentID = INDEX_NONE;
	uint8			Active : 1;

	FAtomStatisticsTreeItem(EAtomStatisticsItemType::Type InType,
						int32 InID,
						const FString&	InLabel,
						bool InActive, 
						const FString& InValue = FString(),
						uint64	InComponentID = INDEX_NONE)
		: Type(InType)
		, Label(InLabel)
		, Value(InValue)
		, ID(InID)
		, ComponentID(InComponentID)
		, Active(InActive)
	{}

	const TCHAR* GetTypeString()
	{
		switch (Type)
		{
		case EAtomStatisticsItemType::Runtime: return TEXT("Runtime");
		case EAtomStatisticsItemType::Rack: return TEXT("Rack");
		case EAtomStatisticsItemType::Listener: return TEXT("Listener");
		case EAtomStatisticsItemType::VoicePool: return TEXT("VoicePool");
		case EAtomStatisticsItemType::ActiveSound: return TEXT("ActiveSound");
		case EAtomStatisticsItemType::PlaybackInstance: return TEXT("Playback");
		case EAtomStatisticsItemType::VirtualLoop: return TEXT("VirtualLoop");
		case EAtomStatisticsItemType::ConcurrencyGroup: return TEXT("Concurrency");
		}
		return TEXT("Unknown"); 
	}
};

class SAtomStatisticsDialog
	: public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAtomStatisticsDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedRef<SWidget> GenerateWidgetForItemAndColumn(TSharedPtr<FAtomStatisticsTreeItem> Item, const FName& ColumnID, const TSharedRef<ITableRow>& TableRow);

private:

	// Atom Runtime List

	EActiveTimerReturnType UpdateGridPanels(double InCurrentTime, float InDeltaTime);
	TSharedRef<SWidget> GetGridPanel();
	SVerticalBox::FSlot* GridSlot = nullptr;

	// Atom Runtime Ouliner

	EActiveTimerReturnType UpdateOutlinerTreeView(double InCurrentTime, float InDeltaTime);
	TSharedRef<SWidget> ConstructOutlinerTreeView();
	void UpdateOutlinerTreeViewItems();
	void OnOutlinerTreeDoubleClick(TSharedPtr<FAtomStatisticsTreeItem> TreeItem);
	SVerticalBox::FSlot* TreeSlot = nullptr;

	TSharedRef<ITableRow> OnGenerateWidgetForTree(TSharedPtr<FAtomStatisticsTreeItem> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildrenForTree(TSharedPtr<FAtomStatisticsTreeItem> InItem, TArray<TSharedPtr<FAtomStatisticsTreeItem>>& OutChildren);

	static const FName ColumnID_Expander;
	static const FName ColumnID_Active;
	static const FName ColumnID_Label;
	static const FName ColumnID_Value;
	static const FName ColumnID_Type;
	static const FName ColumnID_ID;

	/** A Button used to toggle node expansion. */
	TSharedPtr<SButton> ToggleExpandButton;

	FReply OnExpandAllButtonClicked();
	const FSlateBrush* GetExpanderImage() const;

	TArray<TSharedPtr<FAtomStatisticsTreeItem>> Items;
	TSharedPtr<STreeView<TSharedPtr<FAtomStatisticsTreeItem>>> TreeView;

	/** Current way we are expanding nodes */
	EAtomStatisticsExpandMode::Type ExpandMode;
};

/** Widget that represents a row in the AtomStatisticstDialog's list view.  Generates widgets for each column on demand. */
class SAtomStatisticsListTableRow : public SMultiColumnTableRow< TSharedPtr<FAtomStatisticsTreeItem> >
{
public:

	SLATE_BEGIN_ARGS(SAtomStatisticsListTableRow) {}

	/** The AtomStatisticsDialog that owns the tree.  We'll only keep a weak reference to it. */
	SLATE_ARGUMENT(TSharedPtr<SAtomStatisticsDialog>, StatisticDialog)

		/** The list item for this row */
		SLATE_ARGUMENT(TSharedPtr<FAtomStatisticsTreeItem>, Item)

		SLATE_END_ARGS()


		/** Construct function for this widget */
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView);

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list row. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:

	/** Weak reference to the AtomStatisticsDialog widget that owns our list */
	TWeakPtr<SAtomStatisticsDialog> StatisticDialogWeak;

	/** The item associated with this row of data */
	TSharedPtr<FAtomStatisticsTreeItem> Item;
};
