/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomRackActionMenu.cpp
 *
 ****************************************************************************/

#include "SAtomRackActionMenu.h"

#include "EdGraph/EdGraph.h"
#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"
#include "GraphEditor.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "SGraphActionMenu.h"

#include "Graphs/AtomRackGraphSchema.h"

#define LOCTEXT_NAMESPACE "SAtomRackActionMenu"

/*
 * SAtomRackActionMenuItem Compound Widget
 *****************************************************************************/

void SAtomRackActionMenuItem::Construct(const FArguments& InArgs, TSharedPtr<FEdGraphSchemaAction> InAction, TWeakPtr<SAtomRackActionMenu> InOwner)
{
	check(InAction.IsValid());

	this->Owner = InOwner;

	bool bIsNewAtomRack = false;
	if (InAction->GetTypeId() == FAtomRackGraphSchemaAction_NewNode::StaticGetTypeId())
	{
		bIsNewAtomRack = true;
	}

	// The new sound class widget requires 2 lines as it has a text entry box also.	
	if (!bIsNewAtomRack)
	{
		this->ChildSlot
			[
				SNew(SHorizontalBox)
				.ToolTipText(InAction->GetTooltipDescription())
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.Text(InAction->GetMenuDescription())
					.HighlightText(InArgs._HighlightText)
				]
			];
	}
	else
	{
		TSharedRef<SWidget> NewAtomRackWidget = CreateNewAtomRackWidget(InAction->GetMenuDescription(), InAction->GetTooltipDescription(), FCoreStyle::GetDefaultFontStyle("Regular", 9), InAction);
		// Promote requires 2 'slots'
		this->ChildSlot
			[
				NewAtomRackWidget
			];
	}
}

TSharedRef<SWidget> SAtomRackActionMenuItem::CreateNewAtomRackWidget(const FText& DisplayText, const FText& InToolTip, const FSlateFontInfo& NameFont, TSharedPtr<FEdGraphSchemaAction>& InAction)
{
	FString AtomRack;
	FAtomRackGraphSchemaAction_NewNode* Action = static_cast<FAtomRackGraphSchemaAction_NewNode*>(InAction.Get());
	if (Action)
	{
		AtomRack = Action->NewAtomRackName;
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(DisplayText)
			.Font(NameFont)
			.ToolTipText(InToolTip)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(3, 0))
			.VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Text(FText::FromString(AtomRack))
				.ToolTipText(InToolTip)
				.OnTextCommitted(this, &SAtomRackActionMenuItem::OnNewAtomRackNameEntered, InAction)
				.OnTextChanged(this, &SAtomRackActionMenuItem::OnNewAtomRackNameChanged, InAction)
				.SelectAllTextWhenFocused(true)
				.RevertTextOnEscape(true)
			]
		];
}

void SAtomRackActionMenuItem::OnNewAtomRackNameChanged(const FText& NewText, TSharedPtr<FEdGraphSchemaAction> InAction)
{
	FAtomRackGraphSchemaAction_NewNode* Action = static_cast<FAtomRackGraphSchemaAction_NewNode*>(InAction.Get());
	Action->NewAtomRackName = NewText.ToString();
}

void SAtomRackActionMenuItem::OnNewAtomRackNameEntered(const FText& NewText, ETextCommit::Type CommitInfo, TSharedPtr<FEdGraphSchemaAction> InAction)
{
	// Do nothing if we aborted
	if (CommitInfo != ETextCommit::OnEnter)
	{
		return;
	}

	FAtomRackGraphSchemaAction_NewNode* Action = static_cast<FAtomRackGraphSchemaAction_NewNode*>(InAction.Get());
	Action->NewAtomRackName = *NewText.ToString();

	TArray< TSharedPtr<FEdGraphSchemaAction> > ActionList;
	ActionList.Add(InAction);

	Owner.Pin()->OnActionSelected(ActionList, ESelectInfo::OnKeyPress);
}

/*
 * SAtomRackActionMenu Widget
 *****************************************************************************/

SAtomRackActionMenu::~SAtomRackActionMenu()
{
	OnClosedCallback.ExecuteIfBound();
}

void SAtomRackActionMenu::Construct(const FArguments& InArgs)
{
	this->GraphObj = InArgs._GraphObj;
	this->DraggedFromPins = InArgs._DraggedFromPins;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->OnClosedCallback = InArgs._OnClosedCallback;
	this->bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;

	// Build the widget layout
	SBorder::Construct(SBorder::FArguments()
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(5)
		[
			SNew(SBox)
			[
				SAssignNew(GraphActionMenu, SGraphActionMenu)
				.OnActionSelected(this, &SAtomRackActionMenu::OnActionSelected)
				.OnCreateWidgetForAction(SGraphActionMenu::FOnCreateWidgetForAction::CreateSP(this, &SAtomRackActionMenu::OnCreateWidgetForAction))
				.OnCollectAllActions(this, &SAtomRackActionMenu::CollectAllActions)
				.AutoExpandActionMenu(bAutoExpandActionMenu)
				.ShowFilterTextBox(false)
			]
		]
	);
}

void SAtomRackActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	// Build up the context object
	FGraphContextMenuBuilder ContextMenuBuilder(GraphObj);
	if (DraggedFromPins.Num() > 0)
	{
		ContextMenuBuilder.FromPin = DraggedFromPins[0];
	}

	// Determine all possible actions
	GraphObj->GetSchema()->GetGraphContextActions(ContextMenuBuilder);

	// Copy the added options back to the main list
	OutAllActions.Append(ContextMenuBuilder);
}

TSharedRef<SWidget> SAtomRackActionMenu::OnCreateWidgetForAction(struct FCreateWidgetForActionData* const InCreateData)
{
	return SNew(SAtomRackActionMenuItem, InCreateData->Action, SharedThis(this))
		.HighlightText(InCreateData->HighlightText);
}


void SAtomRackActionMenu::OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedActions, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || SelectedActions.Num() == 0)
	{
		if (GraphObj != NULL)
		{
			for (int32 ActionIndex = 0; ActionIndex < SelectedActions.Num(); ActionIndex++)
			{
				TSharedPtr<FEdGraphSchemaAction> CurrentAction = SelectedActions[ActionIndex];

				if (CurrentAction.IsValid())
				{
					FSlateApplication::Get().DismissAllMenus();

					CurrentAction->PerformAction(GraphObj, DraggedFromPins, NewNodePosition);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
