/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackConfigCustomization.cpp
 *
 ****************************************************************************/

#include "AtomRackConfigCustomization.h"

#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"

#include "CriWare.h"
#include "Atom/AtomConfig.h"

#include "Widgets/SAssetView.h"

#define LOCTEXT_NAMESPACE "AtomRackConfig"

FAtomRackConfig* GetRackConfigFromPropertyHandle(TSharedPtr<IPropertyHandle> Handle)
{
	TArray<void*> RawData;
	Handle->AccessRawData(RawData);
	return RawData.Num() == 1 ? static_cast<FAtomRackConfig*>(RawData[0]) : nullptr;
}

TSharedRef<IPropertyTypeCustomization> FAtomRackConfigCustomization::MakeInstance()
{
	return MakeShareable(new FAtomRackConfigCustomization);
}

void FAtomRackConfigCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

void FAtomRackConfigCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Get handle to layout builder to enable adding properties to categories
	//IDetailLayoutBuilder& LayoutBuilder = ChildBuilder.GetParentCategory().GetParentLayout();

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// Master Rack
	TSharedPtr<IPropertyHandle> MasterRackHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomRackConfig, MasterRack));
	check(MasterRackHandle.IsValid());

	ChildBuilder.AddProperty(MasterRackHandle.ToSharedRef());

	// Spatialization Rack
	TSharedPtr<IPropertyHandle> SpatializationRackHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomRackConfig, SpatializationRack));
	check(SpatializationRackHandle.IsValid());

	ChildBuilder.AddProperty(SpatializationRackHandle.ToSharedRef());

	// default output ports override switch
	bOverrideDefaultPortsHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomRackConfig, bOverrideDefaultOutputPorts));
	check(bOverrideDefaultPortsHandle.IsValid());

	ChildBuilder.AddCustomRow(bOverrideDefaultPortsHandle->GetPropertyDisplayName())
		.NameContent()
		[
			CreateInlinedPropertyWidget(bOverrideDefaultPortsHandle.ToSharedRef())
		];

	// Output Port to Rack mapping
	TSharedPtr<IPropertyHandle> OutputPortsHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomRackConfig, OutputPorts));
	check(OutputPortsHandle.IsValid());

	TSharedPtr<IPropertyHandleArray> ArrayHandle = OutputPortsHandle->AsArray();
	check(ArrayHandle.IsValid());
	
	uint32 NumPorts = 0;
	ArrayHandle->GetNumElements(NumPorts);
	for (uint32 Index = 0; Index < NumPorts; ++Index)
	{
		TSharedRef<IPropertyHandle> OutputPortHandle = ArrayHandle->GetElement(Index);
		TSharedPtr<IPropertyHandle> PortNameHandle = OutputPortHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomOutputPort, Name));
		check(PortNameHandle.IsValid());

		TSharedPtr<IPropertyHandle> TargetRackHandle = OutputPortHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAtomOutputPort, TargetRack));
		check(TargetRackHandle.IsValid());

		FName PortName;
		PortNameHandle->GetValue(PortName);

		ChildBuilder.AddCustomRow(FText::FromName(PortName))
			.NameContent()
			[
				CreatePropertyOuputPortNameWidget(PortName)
			]
			.ValueContent()
			[
				TargetRackHandle->CreatePropertyValueWidget()
			]
			.EditCondition(GetIsOuputPortEditableAttribute(PortName), nullptr);
	}
}

TSharedRef<SWidget> FAtomRackConfigCustomization::CreateInlinedPropertyWidget(TSharedRef<IPropertyHandle> Handle) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.Padding(FMargin(-2, 0, 0, 0))
		.AutoWidth()
		[
			Handle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.Padding(FMargin(4, 0, 0, 0))
		.AutoWidth()
		[
			Handle->CreatePropertyNameWidget()
		];
}

TSharedRef<SWidget> FAtomRackConfigCustomization::CreatePropertyOuputPortNameWidget(FName PortName) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(FMargin(0, 1, 0, 1))
		.FillWidth(1)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("PropertyWindow.NoOverlayColor"))
			.Padding(FMargin(0.0f, 2.0f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromName(PortName))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		];
}

TAttribute<bool> FAtomRackConfigCustomization::GetIsOuputPortEditableAttribute(FName PortName) const
{
	TWeakPtr<IPropertyHandle> bOverrideDefaultPortsPropertyWeakPtr = bOverrideDefaultPortsHandle;

	auto Lambda = [bOverrideDefaultPortsPropertyWeakPtr, PortName]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakHandle = bOverrideDefaultPortsPropertyWeakPtr.Pin();

		bool Value = false;
		bOverrideAttenuationPropertyWeakHandle->GetValue(Value);
		Value |= !PortName.ToString().StartsWith(TEXT("_"));
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

#undef LOCTEXT_NAMESPACE
