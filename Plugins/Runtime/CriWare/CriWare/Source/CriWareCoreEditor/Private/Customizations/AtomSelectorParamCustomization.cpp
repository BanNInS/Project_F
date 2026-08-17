
#include "AtomSelectorParamCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"

#include "Atom/AtomParameter.h"

#include "Widgets/SSearchBoxForAtomSelectors.h"

#define LOCTEXT_NAMESPACE "AtomSelectorCustomization"

TSharedRef<IPropertyTypeCustomization> FAtomSelectorParamCustomization::MakeInstance()
{
	return MakeShareable(new FAtomSelectorParamCustomization);
}

void FAtomSelectorParamCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	PropertyHandle->MarkHiddenByCustomization();
}

void FAtomSelectorParamCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//UObject* Outer = NewObject<UAtomConfig>();
	UObject* Outer = nullptr; // normally the source object of the category names

	if (/*Outer &&*/ PropertyHandle->IsValidHandle())
	{
		SelectorParamPropertyHandle = PropertyHandle;

		auto NameProperty = SelectorParamPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomSelectorParam, Name));
		auto LabelProperty = SelectorParamPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomSelectorParam, Label));
		
		// get all the possible suggestions for the bones and sockets.
		//if (const UAnimationAsset* AnimAsset = Cast<const UAnimationAsset>(Notify->GetOuter()))
		//{
		//	if (const USkeleton* Skeleton = AnimAsset->GetSkeleton())
		//	{
				ChildBuilder.AddProperty(PropertyHandle)
				.CustomWidget()
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FMargin(0, 1, 0, 1))
					[
						SNew(STextBlock)
						.Text(PropertyHandle->GetPropertyDisplayName())
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
					]
				]
				.ValueContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SAssetSearchBoxForAtomSelectors, Outer, NameProperty)
						.IncludeAllConfigurationsForSuggestions(false)
						.MustMatchPossibleSuggestions(false)
						.HintText(LOCTEXT("Hint Text", "Selector Name..."))
						.SuggestionType(ESearchBoxForAtomConfigType::SelectorName)
						.OnTextCommitted(this, &FAtomSelectorParamCustomization::OnSearchBoxNameCommitted)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SAssetSearchBoxForAtomSelectors, Outer, LabelProperty)
						.IncludeAllConfigurationsForSuggestions(false)
						.MustMatchPossibleSuggestions(false)
						.HintText(LOCTEXT("Hint Text", "Selector Label..."))
						.SuggestionType(ESearchBoxForAtomConfigType::SelectorLabel)
						.OnTextCommitted(this, &FAtomSelectorParamCustomization::OnSearchBoxLabelCommitted)
					]
				];
		//	}
		//}

		
	}
}

void FAtomSelectorParamCustomization::OnSearchBoxNameCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	TSharedPtr<IPropertyHandle> NameProperty = SelectorParamPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomSelectorParam, Name));
	TSharedPtr<IPropertyHandle> LabelProperty = SelectorParamPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomSelectorParam, Label));
	
	if (NameProperty->IsValidHandle())
	{
		NameProperty->SetValue(InSearchText.ToString());
	}

	// check if current value is valid otherwise set it to first label of this selector
	if (LabelProperty->IsValidHandle())
	{
		//LabelProperty->Get 

		//LabelProperty->SetValue();
	}
}

void FAtomSelectorParamCustomization::OnSearchBoxLabelCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	TSharedPtr<IPropertyHandle> LabelProperty = SelectorParamPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomSelectorParam, Label));

	if (LabelProperty->IsValidHandle())
	{
		LabelProperty->SetValue(InSearchText.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
