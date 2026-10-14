
#include "AtomCategoryCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"

#include "Atom/AtomSoundClass.h"

#include "Widgets/SSearchBoxForAtomCategories.h"

#define LOCTEXT_NAMESPACE "AtomCategoryCustomization"

TSharedRef<IPropertyTypeCustomization> FAtomCategoryCustomization::MakeInstance()
{
	return MakeShareable(new FAtomCategoryCustomization);
}

void FAtomCategoryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	PropertyHandle->MarkHiddenByCustomization();
}

void FAtomCategoryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	//UObject* Outer = NewObject<UAtomConfig>();
	UObject* Outer = nullptr; // normally the source object of the category names

	if (/*Outer &&*/ PropertyHandle->IsValidHandle())
	{
		CategoryPropertyHandle = PropertyHandle;

		auto NameProperty = CategoryPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomCategory, Name));

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
					SNew(SAssetSearchBoxForAtomCategories, Outer, NameProperty)
					.IncludeAllConfigurationsForSuggestions(false)
					.MustMatchPossibleSuggestions(false)
					.HintText(LOCTEXT("Hint Text", "Category Name..."))
					.OnTextCommitted(this, &FAtomCategoryCustomization::OnSearchBoxCommitted)
				];
		//	}
		//}
	}
}

void FAtomCategoryCustomization::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	TSharedPtr<IPropertyHandle> NameProperty = CategoryPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomCategory, Name));
	TSharedPtr<IPropertyHandle> IDProperty = CategoryPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomCategory, ID));
	
	if (NameProperty->IsValidHandle())
	{
		NameProperty->SetValue(InSearchText.ToString());
	}
	if (IDProperty->IsValidHandle())
	{
		IDProperty->SetValue(10);
	}
}

#undef LOCTEXT_NAMESPACE
