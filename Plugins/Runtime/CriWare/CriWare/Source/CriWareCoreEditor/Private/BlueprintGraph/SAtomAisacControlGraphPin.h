
#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraphUtilities.h"
#include "K2Node.h"
#include "SGraphPin.h"
#include "Widgets/Input/SComboBox.h"

#include "Atom/AtomAisacPatch.h"

class SAtomAisacControlGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SAtomAisacControlGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:

	FAtomAisacControl GetValue() const;
	void SetValue(const FAtomAisacControl& NewValue);

	FAtomAisacControl AisacControl;

private: // todo: SAtomAisacControlPicker

	TSharedPtr<FAtomAisacControl> GenerateCachedControls();
	TSharedRef<SWidget> OnGenerateControlComboBoxWidget(TSharedPtr<FAtomAisacControl> Item);
	void OnControlComboBoxOpening();
	void OnControlComboBoxSelectionChanged(TSharedPtr<FAtomAisacControl> Item, ESelectInfo::Type SelectInfo);
	FText GetControlComboBoxContent() const;
	FText GetControlComboBoxToolTip() const;

	TArray<TSharedPtr<FAtomAisacControl>> CachedControlArray;
	TSharedPtr<SComboBox<TSharedPtr<FAtomAisacControl>>> ControlComboBox;
	TSharedPtr<STextBlock> ControlComboBoxContentView;

};
