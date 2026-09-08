
#include "CoreMinimal.h"

// Utility class to build combo boxes out of arrays of names.
class CRIWARECOREEDITOR_API FCriWareNameSelectorGenerator
	: public TSharedFromThis<FCriWareNameSelectorGenerator>
{
public:

	struct FCriWareNameSelectorCallbacks
	{
		TUniqueFunction<void(FName)> OnNewNameSelected;
		TUniqueFunction<FName()> GetCurrentlySelectedName;
		TUniqueFunction<FString()> GetTooltipText;
	};

	// Use this to generate a combo box widget.
	TSharedRef<SWidget> MakeNameSelectorWidget(TArray<FName>& InNameArray, FCriWareNameSelectorCallbacks&& InCallbacks);

protected:

	void OnSelectionChanged(TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> HandleResponseComboBoxGenerateWidget(TSharedPtr<FName> StringItem);
	FText GetComboBoxToolTip() const;
	FText GetComboBoxContent() const;

	TArray<TSharedPtr<FName>> CachedNameArray;
	FCriWareNameSelectorCallbacks CachedCallbacks;
};
