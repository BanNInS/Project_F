#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "PropertyHandle.h"
#include "IPropertyTypeCustomization.h"

class FAtomCategoryCustomization
	: public IPropertyTypeCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

	TSharedPtr<IPropertyHandle> CategoryPropertyHandle;

	/** Handles search box commit for name properties
	 *	@param InSearchText Text that was committed
	 *	@param CommitInfo Commit method
	 *	@param PropertyIndex Index of the name properties in internal array
	 */
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);
};
