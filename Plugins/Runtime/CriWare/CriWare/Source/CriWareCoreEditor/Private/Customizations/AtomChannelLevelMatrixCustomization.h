/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomChannelLevelMatrixCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Atom/AtomBus.h"

// Forward declarations
class IDetailChildrenBuilder;
struct FAtomChannelLevelMatrix;

class CRIWARECOREEDITOR_API FAtomChannelLevelMatrixCustomization
	: public IPropertyTypeCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// IPropertyTypeCustomization interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// End of IPropertyTypeCustomization interface

private:

	TSharedRef<SWidget> GenerateLevelMatrixWidget();

	TOptional<float> GetAudioIntensity(EAtomSpeaker InCh, EAtomSpeaker OutCh) const;
	void OnSetAudioIntensity(float NewValue, ETextCommit::Type CommitInfo, EAtomSpeaker InCh, EAtomSpeaker OutCh);

	TSharedPtr<IPropertyHandle> ProxyProperty;
	TSharedPtr<IPropertyHandleArray> LevelMatrixProperty;
};
