/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomEffectEditor.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"

class UAtomRackBase;

struct FAtomEffectEditorAppearanceInfo
{
	FText CornerText;
};

class SAtomEffectEditor
	: public SCompoundWidget
{
public:

	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, const FGraphPanelSelectionSet&)

	DECLARE_DELEGATE_OneParam(FSingleNodeEvent, const TSharedRef<SAtomEffectEditor>&); // to change to SAtomEffectEditorNode

	DECLARE_DELEGATE_OneParam(FOnFocused, const TSharedRef<SAtomEffectEditor>&);

	/** Info about events occurring in/on the effect editor */
	struct FAtomEffectEditorEvents
	{
		/** Called when selection changes */
		FOnSelectionChanged OnSelectionChanged;
		/** Called when a node is double clicked */
		FSingleNodeEvent OnNodeDoubleClicked;
		/* Called when focus moves to graph */
		FOnFocused OnFocused;
	};

	SLATE_BEGIN_ARGS(SAtomEffectEditor)
		: _AdditionalCommands(static_cast<FUICommandList*>(NULL))
		, _IsEditable(true)
		, _RackToEdit(nullptr)
		, _ShowGraphStateOverlay(true)
		{}
	
		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, AdditionalCommands)
		SLATE_ATTRIBUTE(bool, IsEditable)
		SLATE_ATTRIBUTE(FAtomEffectEditorAppearanceInfo, Appearance)
		SLATE_ARGUMENT(UAtomRackBase*, RackToEdit)
		SLATE_ARGUMENT(FAtomEffectEditorEvents, EffectEvents)

		/** Show overlay elements for the graph state such as the PIE and read-only borders and text */
		SLATE_ATTRIBUTE(bool, ShowGraphStateOverlay)
	SLATE_END_ARGS()

	/**
	 * Constructs a AtomEffectEditor as a child of this widget.
	 *
	 * @param InArgs   Declaration params from which to construct the widget.
	 */
	CRIWARECOREEDITOR_API void Construct(const FArguments& InArgs);

private:

	EActiveTimerReturnType UpdateGraph(double InCurrentTime, float InDeltaTime);

	TSharedRef<SWidget> GetGraphPanel();

	SVerticalBox::FSlot* GridSlot = nullptr;

	TWeakObjectPtr<UAtomRackBase> RackToEdit;
};
