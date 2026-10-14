
#include "SSearchBoxForAtomCategories.h"

#include "Editor.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomConfig.h"

SAssetSearchBoxForAtomCategories::SAssetSearchBoxForAtomCategories()
{
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

SAssetSearchBoxForAtomCategories::~SAssetSearchBoxForAtomCategories()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void SAssetSearchBoxForAtomCategories::Construct(const FArguments& InArgs, const UObject* Outer, TSharedPtr<IPropertyHandle> CategoryNameProperty)
{
	//check(Outer);

	CategoryPropertyHandle = CategoryNameProperty;
	// set delegate on property change
	// this doesn't work for undo still
	CategoryPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &SAssetSearchBoxForAtomCategories::RefreshName));

	TArray<FAssetSearchBoxSuggestion> PossibleSuggestions;
	if (InArgs._IncludeAllConfigurationsForSuggestions.Get())
	{
		// lookup for all configs
	}
	else
	{
		const UAtomConfig* AtomConfig = nullptr;
		/*if (const UAtomRack* AtomRack = Cast<const UAtomRack>(Outer))
		{
			AtomConfig = AtomRack->AtomConfig;
		}
		else
		{
			AtomConfig = Cast<const UAtomConfig>(Outer);
		}*/

		AtomConfig = GCriWare->GetAtomConfiguration();
		check(AtomConfig);

		if (AtomConfig)
		{
			int NumCategories = AtomConfig->GetNumCategories();
			for (int Index = 0; Index < NumCategories; Index++)
			{
				PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion(AtomConfig->GetCategorieName(Index).ToString()));
			}
		}
	}

	// create the asset search box
	ChildSlot
	[
		SAssignNew(SearchBox, SAssetSearchBox)
		.InitialText(GetCategoryName())
		.HintText(InArgs._HintText)
		.OnTextCommitted(InArgs._OnTextCommitted)
		.PossibleSuggestions(PossibleSuggestions)
		.DelayChangeNotificationsWhileTyping(true)
		.MustMatchPossibleSuggestions(InArgs._MustMatchPossibleSuggestions)
	];
}

void SAssetSearchBoxForAtomCategories::RefreshName()
{
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(GetCategoryName());
	}
}

FText SAssetSearchBoxForAtomCategories::GetCategoryName() const
{
	FName CurValue;
	if (CategoryPropertyHandle.IsValid())
	{
		CategoryPropertyHandle->GetValue(CurValue);
	}
	return CurValue.IsNone() ? FText::GetEmpty() : FText::FromName(CurValue);
}
