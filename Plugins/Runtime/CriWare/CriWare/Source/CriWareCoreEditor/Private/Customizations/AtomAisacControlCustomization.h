/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAisacControlCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomAisacPatch.h"

class FAtomAisacControlCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

private:

	/** AtomAisacControl being edited. */
	TSharedPtr<IPropertyHandle> ControlHandle;

	void OnControlPropertyChanged();
	TSharedPtr<FAtomAisacControl> GenerateCachedControls();
	bool IsControlComboBoxEnabled() const;
	TSharedRef<SWidget> OnGenerateControlComboBoxWidget(TSharedPtr<FAtomAisacControl> Item);
	void OnControlComboBoxOpening();
	void OnControlComboBoxSelectionChanged(TSharedPtr<FAtomAisacControl> Item, ESelectInfo::Type SelectInfo);
	FText GetControlComboBoxContent() const;
	FText GetControlComboBoxToolTip() const;

	TArray<TSharedPtr<FAtomAisacControl>> CachedControlArray;
	TSharedPtr<SComboBox<TSharedPtr<FAtomAisacControl>>> ControlComboBox;
	TSharedPtr<STextBlock> ControlComboBoxContentView;
};
