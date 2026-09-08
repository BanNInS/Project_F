
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

enum class ESearchBoxForAtomConfigType : uint8
{
	SelectorName,
	SelectorLabel,
};

class SAssetSearchBoxForAtomSelectors : public SCompoundWidget, public FEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(SAssetSearchBoxForAtomSelectors)
		: _OnTextCommitted()
		, _HintText()
		, _MustMatchPossibleSuggestions(false)
		, _IncludeAllConfigurationsForSuggestions(false)
		, _SuggestionType(ESearchBoxForAtomConfigType::SelectorName)
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

		/** Which possible suggestions from AtomConfig to search in */
		SLATE_ARGUMENT(ESearchBoxForAtomConfigType, SuggestionType)

		/** An attribute name used depending the SuggestionType. */
		SLATE_ATTRIBUTE(FName, SuggestionName)

	SLATE_END_ARGS()

	SAssetSearchBoxForAtomSelectors();
	~SAssetSearchBoxForAtomSelectors();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const UObject* Outer, TSharedPtr<IPropertyHandle> SelectorNameProperty);

	/** Refresh bone name */
	void RefreshName();

	virtual void PostUndo(bool bSuccess) { RefreshName(); };
	virtual void PostRedo(bool bSuccess) { RefreshName(); };

private:

	TSharedPtr<IPropertyHandle>	TargetPropertyHandle;
	TSharedPtr<SAssetSearchBox>	SearchBox;

	/** Get the propeorty name to display */
	FText GetPropertyName() const;
};
