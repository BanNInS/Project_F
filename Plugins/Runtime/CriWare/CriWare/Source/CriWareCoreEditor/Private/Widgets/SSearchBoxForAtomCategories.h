
#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"
#include "PropertyHandle.h"
#include "SAssetSearchBox.h"
#include "EditorUndoClient.h"

// Forward Declarations
class UObject;

class SAssetSearchBoxForAtomCategories : public SCompoundWidget, public FEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(SAssetSearchBoxForAtomCategories)
		: _OnTextCommitted()
		, _HintText()
		, _MustMatchPossibleSuggestions(false)
		, _IncludeAllConfigurationsForSuggestions(false)
	{}

		/** Where to place the suggestion list */
		SLATE_ARGUMENT(EMenuPlacement, SuggestionListPlacement)

		/** Invoked whenever the text is committed (e.g. user presses enter) */
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

		/** Hint text to display for the search text when there is no value */
		SLATE_ATTRIBUTE(FText, HintText)

		/** Whether the SearchBox allow entries that don't match the possible suggestions */
		SLATE_ATTRIBUTE(bool, MustMatchPossibleSuggestions)

		/** Whether the possible suggestions should include any AtomConfig in project */
		SLATE_ATTRIBUTE(bool, IncludeAllConfigurationsForSuggestions)

	SLATE_END_ARGS()

	SAssetSearchBoxForAtomCategories();
	~SAssetSearchBoxForAtomCategories();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const UObject* Outer, TSharedPtr<IPropertyHandle> CategoryNameProperty);

	/** Refresh bone name */
	void RefreshName();

	virtual void PostUndo(bool bSuccess) { RefreshName(); };
	virtual void PostRedo(bool bSuccess) { RefreshName(); };

private:

	TSharedPtr<IPropertyHandle>	CategoryPropertyHandle;
	TSharedPtr<SAssetSearchBox>	SearchBox;

	/** Get the selector name to display */
	FText GetCategoryName() const;
};
