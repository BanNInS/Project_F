/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAttenuationCustomization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

#include "Atom/AtomAttenuation.h"

class FAtomAttenuationSettingsCustomization
	: public IPropertyTypeCustomization
{
public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//~ End IPropertyTypeCustomization

	TAttribute<bool> IsAttenuationOverriddenAttribute() const;
	TAttribute<EVisibility> IsAttenuationOverriddenVisibleAttribute() const;

protected:

	EVisibility IsSphereSelected() const;
	EVisibility IsConeSelected() const;
	EVisibility IsCustomAirAbsorptionCurveSelected() const;
	EVisibility IsPriorityAttenuationLinearMethodSelected() const;
	EVisibility IsCustomPriorityAttenuationCurveSelected() const;
	EVisibility IsLinearOrCustomPriorityAttenuationSelected() const;
	EVisibility IsManualPriorityAttenuationSelected() const;

	TAttribute<bool> GetIsAttenuationEnabledAttribute() const;
	TAttribute<bool> GetIsFalloffModeEnabledAttribute() const;
	TSharedPtr<IPropertyHandle> GetOverrideAttenuationHandle(TSharedRef<IPropertyHandle> StructPropertyHandle);

	TSharedPtr<IPropertyHandle> bIsAttenuatedHandle;
	TSharedPtr<IPropertyHandle> AttenuationShapeHandle;
	TSharedPtr<IPropertyHandle> DistanceAlgorithmHandle;
	TSharedPtr<IPropertyHandle> DbAttenuationAtMaxHandle;
	TSharedPtr<IPropertyHandle> bOverrideAttenuationHandle;
	TSharedPtr<IPropertyHandle> bIsSpatializedHandle;
	TSharedPtr<IPropertyHandle> bIsAirAbsorptionEnabledHandle;
	TSharedPtr<IPropertyHandle> bIsFocusedHandle;
	TSharedPtr<IPropertyHandle> bIsOcclusionEnabledHandle;
	//TSharedPtr<IPropertyHandle> bIsReverbSendEnabledHandle;
	TSharedPtr<IPropertyHandle> bIsPriorityAttenuationEnabledHandle;
	TSharedPtr<IPropertyHandle> bIsBusSendAttenuationEnabledHandle;
	TSharedPtr<IPropertyHandle> bIsSourceDataOverrideEnabledHandle;
	TSharedPtr<IPropertyHandle> AbsorptionMethodHandle;
	//TSharedPtr<IPropertyHandle> ReverbSendMethodHandle;
	TSharedPtr<IPropertyHandle> PriorityAttenuationMethodHandle;

	TAttribute<bool> GetIsFocusEnabledAttribute() const;
	TAttribute<bool> GetIsOcclusionEnabledAttribute() const;
	TAttribute<bool> GetIsSpatializationEnabledAttribute() const;
	TAttribute<bool> GetIsAirAbsorptionEnabledAttribute() const;
	//TAttribute<bool> GetIsReverbSendEnabledAttribute() const;
	TAttribute<bool> GetIsPriorityAttenuationEnabledAttribute() const;
	TAttribute<bool> GetIsBusSendAttenuationEnabledAttribute() const;

};
