
#include "AtomModulationParameterSettingsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Styling/AppStyle.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "InputCoreTypes.h"
#include "Misc/Attribute.h"
#include "PropertyRestriction.h"
#include "SCurveEditor.h"
#include "ScopedTransaction.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

#include "Atom/Atom.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationDestination.h"
#include "Atom/Modulation/AtomModulationControlBus.h"

//#include "AudioModulationStyle.h"


#define LOCTEXT_NAMESPACE "AtomModulation"

namespace AtomModParamSettingsUtils
{
	UAtomModulationParameter* GetStructParameter(TSharedRef<IPropertyHandle> StructPropertyHandle)
	{
		TArray<UObject*> Objects;
		StructPropertyHandle->GetOuterObjects(Objects);

		check(Objects.Num() == 1);
		return CastChecked<UAtomModulationParameter>(Objects[0]);
	}
}

void FAtomModulationParameterSettingsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FAtomModulationParameterSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();
		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	TSharedRef<IPropertyHandle> NormalizedValueHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomModulationParameterSettings, ValueNormalized)).ToSharedRef();
	TSharedRef<IPropertyHandle> UnitValueHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomModulationParameterSettings, ValueUnit)).ToSharedRef();
	TSharedRef<IPropertyHandle> UnitDisplayNameHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomModulationParameterSettings, UnitDisplayName)).ToSharedRef();

	// Default Value Row (Unit)
	UAtomModulationParameter* Parameter = AtomModParamSettingsUtils::GetStructParameter(StructPropertyHandle);
	ensure(Parameter);

	if (Parameter && Parameter->RequiresUnitConversion())
	{
		// When editor opens, set unit value in case bus unit has changed while editor was closed.
		Parameter->RefreshUnitValue();

		UnitValueHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([StructPropertyHandle]()
		{
			if (UAtomModulationParameter* Parameter = AtomModParamSettingsUtils::GetStructParameter(StructPropertyHandle))
			{
				Parameter->RefreshNormalizedValue();
				Parameter->RefreshUnitValue();
			}
		}));

		NormalizedValueHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([StructPropertyHandle]()
		{
			if (UAtomModulationParameter* Parameter = AtomModParamSettingsUtils::GetStructParameter(StructPropertyHandle))
			{
				Parameter->RefreshUnitValue();
			}
		}));

		ChildBuilder.AddCustomRow(StructPropertyHandle->GetPropertyDisplayName())
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::Format(LOCTEXT("BusModulationValue_DisplayNameFormat", "Default {0} Value "), StructPropertyHandle->GetPropertyDisplayName()))
				.ToolTipText(UnitValueHandle->GetToolTipText())
				.IsEnabled(false)
			]
			.ValueContent()
			.MinDesiredWidth(300.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.4f)
				.Padding(1.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					UnitValueHandle->CreatePropertyValueWidget()
				]
				+ SHorizontalBox::Slot()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(TAttribute<FText>::Create([StructPropertyHandle]()
					{
						if (UAtomModulationParameter* Parameter = AtomModParamSettingsUtils::GetStructParameter(StructPropertyHandle))
						{
							if (Parameter->RequiresUnitConversion())
							{
								return Parameter->Settings.UnitDisplayName;
							}
						}
						return FText();
					}))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.4f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					NormalizedValueHandle->CreatePropertyValueWidget()
				]
				+ SHorizontalBox::Slot()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(LOCTEXT("SoundModulationControl_UnitValueNormalized", "Normalized"))
					.IsEnabled(false)
				]
			];
	}
	// Default Value Row (No Unit)
	else
	{
		ChildBuilder.AddCustomRow(StructPropertyHandle->GetPropertyDisplayName())
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::Format(LOCTEXT("BusModulationValue_DisplayNameFormat", "Default {0} Value "), StructPropertyHandle->GetPropertyDisplayName()))
				.ToolTipText(StructPropertyHandle->GetToolTipText())
				.IsEnabled(false)
			]
			.ValueContent()
			[
				NormalizedValueHandle->CreatePropertyValueWidget()
			];
	}

	ChildBuilder.AddProperty(UnitDisplayNameHandle);
}

#undef LOCTEXT_NAMESPACE
