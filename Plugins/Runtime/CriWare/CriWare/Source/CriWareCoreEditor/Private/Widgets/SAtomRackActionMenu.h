/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomRackActionMenu.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "GraphEditor.h"

// Forward Declarations
class SGraphActionMenu;

/*
 * SAtomRackActionMenuItem Compound Widget
 *****************************************************************************/

/** Widget for displaying a single item  */
class SAtomRackActionMenuItem 
	: public SCompoundWidget
{
public:
						
	SLATE_BEGIN_ARGS(SAtomRackActionMenuItem)
		{}

		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FEdGraphSchemaAction> InAction, TWeakPtr<class SAtomRackActionMenu> InOwner);

private:

	/* Create the new Atom rack widget */
	TSharedRef<SWidget> CreateNewAtomRackWidget(const FText& DisplayText, const FText& ToolTip, const FSlateFontInfo& NameFont, TSharedPtr<FEdGraphSchemaAction>& InAction);

	/** Called when confirming name for new Atom rack */
	void OnNewAtomRackNameEntered(const FText& NewText, ETextCommit::Type CommitInfo, TSharedPtr<FEdGraphSchemaAction> InAction);

	/** Called when text is changed for a new Atom rack name */
	void OnNewAtomRackNameChanged(const FText& NewText, TSharedPtr<FEdGraphSchemaAction> InAction);

	TWeakPtr<class SAtomRackActionMenu> Owner;
};

/*
 * SAtomRackActionMenu Widget
 *****************************************************************************/

class SAtomRackActionMenu
	: public SBorder
{
	friend class SAtomRackActionMenuItem;

public:

	SLATE_BEGIN_ARGS(SAtomRackActionMenu)
		: _GraphObj(static_cast<UEdGraph*>(NULL))
		, _NewNodePosition(FVector2f::ZeroVector)
		, _AutoExpandActionMenu(true)
		{}

		SLATE_ARGUMENT(UEdGraph*, GraphObj)
		SLATE_ARGUMENT(FDeprecateSlateVector2D, NewNodePosition)
		SLATE_ARGUMENT(TArray<UEdGraphPin*>, DraggedFromPins)
		SLATE_ARGUMENT(SGraphEditor::FActionMenuClosed, OnClosedCallback)
		SLATE_ARGUMENT(bool, AutoExpandActionMenu)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SAtomRackActionMenu();

protected:

	UEdGraph* GraphObj;
	TArray<UEdGraphPin*> DraggedFromPins;
	FDeprecateSlateVector2D NewNodePosition;
	bool bAutoExpandActionMenu;

	SGraphEditor::FActionMenuClosed OnClosedCallback;

	TSharedPtr<SGraphActionMenu> GraphActionMenu;

	void OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedActions, ESelectInfo::Type InSelectionType);

	TSharedRef<SWidget> OnCreateWidgetForAction(struct FCreateWidgetForActionData* const InCreateData);

	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
};
