
#include "SAtomAisacControlGraphPin.h"

#include "PropertyEditorModule.h"
#include "Widgets/Text/STextBlock.h"

#include "CriWare.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomAisacPatch.h"

#define LOCTEXT_NAMESPACE "SAtomAisacControlGraphPin"

void SAtomAisacControlGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);	
}

TSharedRef<SWidget> SAtomAisacControlGraphPin::GetDefaultValueWidget()
{
	// get value -> string to struct
	FString DefaultString = GraphPinObj->GetDefaultAsString();
	FAtomAisacControl::StaticStruct()->ImportText(*DefaultString, &AisacControl, nullptr, EPropertyPortFlags::PPF_None, GLog, FAtomAisacControl::StaticStruct()->GetName());


	TSharedPtr<FAtomAisacControl> InitialSelectedSetting = GenerateCachedControls();

	// Create widget
	return SAssignNew(ControlComboBox, SComboBox<TSharedPtr<FAtomAisacControl>>)
		.IsEnabled(this, &SGraphPin::GetDefaultValueIsEditable)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		.OptionsSource(&CachedControlArray)
		.OnGenerateWidget(this, &SAtomAisacControlGraphPin::OnGenerateControlComboBoxWidget)
		.OnComboBoxOpening(this, &SAtomAisacControlGraphPin::OnControlComboBoxOpening)
		.OnSelectionChanged(this, &SAtomAisacControlGraphPin::OnControlComboBoxSelectionChanged)
		.InitiallySelectedItem(InitialSelectedSetting)
		.ContentPadding(FMargin(2.0f, 2.0f))
		.Content()
		[
			SAssignNew(ControlComboBoxContentView, STextBlock)
			.Text(this, &SAtomAisacControlGraphPin::GetControlComboBoxContent)
			.ToolTipText(this, &SAtomAisacControlGraphPin::GetControlComboBoxToolTip)
		];
}

FAtomAisacControl SAtomAisacControlGraphPin::GetValue() const
{
	return AisacControl;
}

void SAtomAisacControlGraphPin::SetValue(const FAtomAisacControl& NewValue)
{
	AisacControl = NewValue;

	// set value -> struct to string
	FString ValueString;
	FAtomAisacControl::StaticStruct()->ExportText(ValueString, &AisacControl, &AisacControl, nullptr, EPropertyPortFlags::PPF_None, nullptr);
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, ValueString);
}

TSharedPtr<FAtomAisacControl> SAtomAisacControlGraphPin::GenerateCachedControls()
{
	TSharedPtr<FAtomAisacControl> InitialSelectedControl = MakeShared<FAtomAisacControl>();
	CachedControlArray.Reset();
	CachedControlArray.Add(InitialSelectedControl);

	if (GCriWare)
	{
		if (const UAtomConfig* AtomConfig = GCriWare->GetAtomConfiguration())
		{
			for (int Index = 0; Index < AtomConfig->GetNumAisacControls(); ++Index)
			{
				TSharedPtr<FAtomAisacControl> Item = MakeShared<FAtomAisacControl>(AtomConfig->GetAisacControl(Index));
				CachedControlArray.Add(Item);

				if (Item->Name == GetValue().Name)
				{
					InitialSelectedControl = Item;
				}
			}
		}
	}

	return InitialSelectedControl;
}

TSharedRef<SWidget> SAtomAisacControlGraphPin::OnGenerateControlComboBoxWidget(TSharedPtr<FAtomAisacControl> Item)
{
	return SNew(STextBlock)
		.Text(FText::FromName(Item->Name))
		.ToolTipText(FText::FromName(Item->Name));
}

void SAtomAisacControlGraphPin::OnControlComboBoxOpening()
{
	TSharedPtr<FAtomAisacControl> SelectedControl = GenerateCachedControls();
	ControlComboBox->SetSelectedItem(SelectedControl);
	ControlComboBox->RefreshOptions();
}

void SAtomAisacControlGraphPin::OnControlComboBoxSelectionChanged(TSharedPtr<FAtomAisacControl> Item, ESelectInfo::Type SelectInfo)
{
	if (Item)
	{
		// update the combobox content with selection
		//ControlComboBoxContentView->SetText(FText::FromName(Item->Name));

		// set the selection to control
		SetValue(*Item.Get());
	}
}

FText SAtomAisacControlGraphPin::GetControlComboBoxContent() const
{
	if (ControlComboBox)
	{
		if (auto Item = ControlComboBox->GetSelectedItem().Get())
		{
			return FText::FromName(Item->Name);
		}
	}

	return FText::FromName(NAME_None);
}

FText SAtomAisacControlGraphPin::GetControlComboBoxToolTip() const
{
	if (ControlComboBox)
	{
		if (auto Item = ControlComboBox->GetSelectedItem().Get())
		{
			return FText::FromName(Item->Name);
		}
	}

	return FText::FromName(NAME_None);
}

#undef LOCTEXT_NAMESPACE
