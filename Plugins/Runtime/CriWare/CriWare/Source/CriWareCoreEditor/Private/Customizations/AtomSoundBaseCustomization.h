/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBaseCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Atom/Atom.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomCueSheet.h"

#include "Customizations/SubObjectPicker.h"

/** Customization for AtomSoundBase properties with any derived type support. */
class FAtomSoundBaseCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

private:

	bool CanCustomize(UClass* PropertyClass);
	TStrongObjectPtr<UParentObjectPropertyHolder_Base> CreateSoundBankPropertyHolder(UClass* PropertyClass);

	/** local sound bank. */
	TStrongObjectPtr<UParentObjectPropertyHolder_Base> SoundBankHolder;

	/** AtomSoundBank to select sound from. */
	TSharedPtr<IPropertyHandle> SoundBankHandle;

	/** AtomSoundBase being edited. */
	TSharedPtr<IPropertyHandle> SoundBaseHandle;

	/** The sub-object picker. */
	TSharedPtr<FSubObjectPicker> SoundBasePicker;

	bool IsSoundPickerEnabled() const;

	EVisibility IsSoundBankSelected() const;

	void OnSoundBankPropertyChanged();
	//void OnSoundBasePropertyChanged();

	// SAssetView
	bool OnIsAssetAcceptableForDropWithReason(const FAssetData& Asset, FText& OutReason);
	void OnAssetDropped(const FDragDropEvent&, const FAssetData& Asset);
};
