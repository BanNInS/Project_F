
#include "SSearchBoxForAtomSelectors.h"

#include "Editor.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomConfig.h"

SAssetSearchBoxForAtomSelectors::SAssetSearchBoxForAtomSelectors()
{
	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}
}

SAssetSearchBoxForAtomSelectors::~SAssetSearchBoxForAtomSelectors()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

void SAssetSearchBoxForAtomSelectors::Construct(const FArguments& InArgs, const UObject* Outer, TSharedPtr<IPropertyHandle> CategoryNameProperty)
{
	//check(Outer);

	TargetPropertyHandle = CategoryNameProperty;
	// set delegate on property change
	// this doesn't work for undo still
	TargetPropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &SAssetSearchBoxForAtomSelectors::RefreshName));

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
		//check(AtomConfig);

		if (AtomConfig)
		{
			switch (InArgs._SuggestionType)
			{
			case ESearchBoxForAtomConfigType::SelectorName:
				{
					int NumSelectors = AtomConfig->GetNumSelectors();
					for (int Index = 0; Index < NumSelectors; Index++)
					{
						PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion(AtomConfig->GetSelectorName(Index).ToString()));
					}
				}
				break;
			case ESearchBoxForAtomConfigType::SelectorLabel:
				{
					auto Selector = AtomConfig->GetSelector(InArgs._SuggestionName.Get());
					if (Selector && !Selector->GetName().IsNone())
					{
						int NumLabels = Selector->GetNumLabels();
						for (int Index = 0; Index < NumLabels; Index++)
						{
							PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion(Selector->GetLabel(Index).ToString()));
						}
					}
				}
				break;
			default:
				{
					int NumCategories = AtomConfig->GetNumCategories();
					for (int Index = 0; Index < NumCategories; Index++)
					{
						PossibleSuggestions.Add(FAssetSearchBoxSuggestion::MakeSimpleSuggestion(AtomConfig->GetCategorieName(Index).ToString()));
					}
				}
				break;
			}
		}
	}

	// create the asset search box
	ChildSlot
	[
		SAssignNew(SearchBox, SAssetSearchBox)
		.InitialText(GetPropertyName())
		.HintText(InArgs._HintText)
		.OnTextCommitted(InArgs._OnTextCommitted)
		.PossibleSuggestions(PossibleSuggestions)
		.DelayChangeNotificationsWhileTyping(true)
		.MustMatchPossibleSuggestions(InArgs._MustMatchPossibleSuggestions)
	];
}

void SAssetSearchBoxForAtomSelectors::RefreshName()
{
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(GetPropertyName());
	}
}

FText SAssetSearchBoxForAtomSelectors::GetPropertyName() const
{
	FName CurValue;
	if (TargetPropertyHandle.IsValid())
	{
		TargetPropertyHandle->GetValue(CurValue);
	}
	return CurValue.IsNone() ? FText::GetEmpty() : FText::FromName(CurValue);
}
