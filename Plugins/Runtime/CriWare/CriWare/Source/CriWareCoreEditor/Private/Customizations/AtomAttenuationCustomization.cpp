/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAttenuationCustomization.cpp
 *
 ****************************************************************************/

#include "AtomAttenuationCustomization.h"

#include "Widgets/Text/STextBlock.h"
#include "PropertyRestriction.h"
#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"

#include "Atom/Atom.h"

#define LOCTEXT_NAMESPACE "AtomAttenuationSettings"

namespace AtomAttenuationSettingsUtils
{
	bool GetValue(TSharedPtr<IPropertyHandle> InProp)
	{
		if (InProp.IsValid())
		{
			bool Val = false;
			InProp->GetValue(Val);
			return Val;
		}
		return true;
	}

	void SortCategories(const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		const IDetailCategoryBuilder* AttenuationBuilder = AllCategoryMap.FindRef("Attenuation");
		if (!AttenuationBuilder)
		{
			return;
		}

		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			int32 SortOrder = AttenuationBuilder->GetSortOrder();
			const FName CategoryName = Pair.Key;

			// Early out if attenuation category
			if (CategoryName == "Attenuation")
			{
				continue;
			}

			// Organize related categories
			if (CategoryName == "AttenuationDistance")
			{
				SortOrder += 1;
			}
			else if (CategoryName == "AttenuationSpatialization")
			{
				SortOrder += 2;
			}
			else if (CategoryName == "AttenuationOcclusion")
			{
				SortOrder += 3;
			}
			else if (CategoryName == "AttenuationBusSends")
			{
				SortOrder += 4;
			}
			/*else if (CategoryName == "AttenuationReverbSend")
			{
				SortOrder += 5;
			}*/
			else if (CategoryName == "AttenuationListenerFocus")
			{
				SortOrder += 5;
			}
			else if (CategoryName == "AttenuationPriority")
			{
				SortOrder += 6;
			}
			else if (CategoryName == "AttenuationAirAbsorption")
			{
				SortOrder += 7;
			}
			/*else if (CategoryName == "AttenuationPluginSettings")
			{
				SortOrder += 8;
			}*/
			else if (CategoryName == "AttenuationSourceDataOverride")
			{
				SortOrder += 8;
			}
			else
			{
				// Add space to any other categories interfering with space for attenuation-related categories
				const int32 ValueSortOrder = Pair.Value->GetSortOrder();
				if (ValueSortOrder >= SortOrder && ValueSortOrder < SortOrder + 9)
				{
					SortOrder += 9;
				}
				// Else, leave current sort order alone
				else
				{
					continue;
				}
			}

			Pair.Value->SetSortOrder(SortOrder);
		}
	}
}

TSharedRef<IPropertyTypeCustomization> FAtomAttenuationSettingsCustomization::MakeInstance()
{
	return MakeShared<FAtomAttenuationSettingsCustomization>();
}

void FAtomAttenuationSettingsCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Property handle here is the base struct. We are going to hide it since we're showing it's properties directly.
	StructPropertyHandle->MarkHiddenByCustomization();
}

TSharedPtr<IPropertyHandle> FAtomAttenuationSettingsCustomization::GetOverrideAttenuationHandle(TSharedRef<IPropertyHandle> StructPropertyHandle)
{
	TSharedPtr<IPropertyHandle> ParentHandle = StructPropertyHandle->GetParentHandle();
	if (TSharedPtr<IPropertyHandle> GrandParentHandle = ParentHandle->GetParentHandle())
	{
		ParentHandle = GrandParentHandle;
	}
	return ParentHandle->GetChildHandle(TEXT("bOverrideAttenuation"), true);
}

void FAtomAttenuationSettingsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// Get handle to layout builder to enable adding properties to categories
	IDetailLayoutBuilder& LayoutBuilder = ChildBuilder.GetParentCategory().GetParentLayout();

	uint32 NumChildren;
	StructPropertyHandle->GetNumChildren(NumChildren);

	TMap<FName, TSharedPtr<IPropertyHandle>> PropertyHandles;
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();
		PropertyHandles.Add(PropertyName, ChildHandle);
	}

	// Get the override attenuation handle, if it exists
	bOverrideAttenuationHandle = GetOverrideAttenuationHandle(StructPropertyHandle);

	AttenuationShapeHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, AttenuationShape));
	DistanceAlgorithmHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, DistanceAlgorithm));

	bIsOcclusionEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableOcclusion)).ToSharedRef();
	bIsSpatializedHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bSpatialize)).ToSharedRef();
	bIsAirAbsorptionEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bAttenuateWithLPF)).ToSharedRef();
	//bIsReverbSendEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableReverbSend)).ToSharedRef();
	bIsPriorityAttenuationEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnablePriorityAttenuation)).ToSharedRef();
	bIsBusSendAttenuationEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableBusSends)).ToSharedRef();
	bIsSourceDataOverrideEnabledHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableSourceDataOverride)).ToSharedRef();
	//ReverbSendMethodHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ReverbSendMethod)).ToSharedRef();
	PriorityAttenuationMethodHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PriorityAttenuationMethod)).ToSharedRef();
	AbsorptionMethodHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, AbsorptionMethod)).ToSharedRef();
	
	bIsFocusedHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableListenerFocus)).ToSharedRef();

	// Set protected member so FAtomAttenuationSettingsCustomization knows how to make attenuation settings editable
	bIsAttenuatedHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bAttenuate)).ToSharedRef();

	LayoutBuilder.AddPropertyToCategory(bIsAttenuatedHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute());

	TSharedRef<IPropertyHandle> AttenuationExtentsHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, AttenuationShapeExtents)).ToSharedRef();

	uint32 NumExtentChildren;
	AttenuationExtentsHandle->GetNumChildren(NumExtentChildren);

	TSharedPtr<IPropertyHandle> ExtentXHandle;
	TSharedPtr<IPropertyHandle> ExtentYHandle;
	TSharedPtr<IPropertyHandle> ExtentZHandle;

	for (uint32 ExtentChildIndex = 0; ExtentChildIndex < NumExtentChildren; ++ExtentChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = AttenuationExtentsHandle->GetChildHandle(ExtentChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, X))
		{
			ExtentXHandle = ChildHandle;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Y))
		{
			ExtentYHandle = ChildHandle;
		}
		else
		{
			check(PropertyName == GET_MEMBER_NAME_CHECKED(FVector, Z));
			ExtentZHandle = ChildHandle;
		}
	}

	LayoutBuilder.AddPropertyToCategory(DistanceAlgorithmHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);

	/*DbAttenuationAtMaxHandle = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, dBAttenuationAtMax));
	LayoutBuilder.AddPropertyToCategory(DbAttenuationAtMaxHandle)
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsNaturalSoundSelected))
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);*/

	/*LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FalloffMode)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsNaturalSoundSelected))
		.EditCondition(GetIsFalloffModeEnabledAttribute(), nullptr);*/

	LayoutBuilder.AddPropertyToCategory(AttenuationShapeHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);

	// Get the attenuation category directly here otherwise our category is going to be incorrect for the following custom rows (e.g. "Vector" vs "Attenuation")
	IDetailCategoryBuilder& AttenuationCategory = LayoutBuilder.EditCategory("AttenuationDistance");

	const FText RadiusLabel(LOCTEXT("RadiusLabel", "Inner Radius"));

	AttenuationCategory.AddCustomRow(RadiusLabel)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(RadiusLabel)
			.ToolTipText(LOCTEXT("RadiusToolTip", "The radius that defines when sound attenuation begins (or when a custom attenuation curve begins). Sounds played at a distance less than this will not be attenuated."))
			.Font(StructCustomizationUtils.GetRegularFont())
			.IsEnabled(GetIsAttenuationEnabledAttribute())
		]
		.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsSphereSelected))
		.IsEnabled(GetIsAttenuationEnabledAttribute());

	AttenuationCategory.AddCustomRow(LOCTEXT("ConeRadiusLabel", "Cone Radius"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConeRadiusLabel", "Cone Radius"))
			.ToolTipText(LOCTEXT("ConeRadiusToolTip", "The attenuation cone's radius."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			ExtentXHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.IsEnabled(GetIsAttenuationEnabledAttribute());

	AttenuationCategory.AddCustomRow(LOCTEXT("ConeAngleLabel", "Cone Angle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConeAngleLabel", "Cone Angle"))
			.ToolTipText(LOCTEXT("ConeAngleToolTip", "The angle of the inner edge of the attenuation cone's falloff. Inside this angle sounds will be at full volume."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			ExtentYHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.IsEnabled(GetIsAttenuationEnabledAttribute());

	AttenuationCategory.AddCustomRow(LOCTEXT("ConeFalloffAngleLabel", "Cone Falloff Angle"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ConeFalloffAngleLabel", "Cone Falloff Angle"))
			.ToolTipText(LOCTEXT("ConeFalloffAngleToolTip", "The angle of the outer edge of the attenuation cone's falloff. Outside this angle sounds will be inaudible."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			ExtentZHandle->CreatePropertyValueWidget()
		]
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.IsEnabled(GetIsAttenuationEnabledAttribute());

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ConeOutsideLevel)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);

	/*LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ConeOffset)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);*/

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FalloffDistance)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);

	/*LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ConeSphereRadius)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ConeSphereFalloffDistance)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsConeSelected))
		.EditCondition(GetIsAttenuationEnabledAttribute(), nullptr);*/

	LayoutBuilder.AddPropertyToCategory(bIsSpatializedHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute());

	// Check to see if a spatialization plugin is enabled
	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, SpatializationAlgorithm)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, BinauralRadius)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonSpatializedRadiusStart)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonSpatializedRadiusEnd)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonSpatializedRadiusMode)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, StereoSpread)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bApplyNormalizationToStereoSounds)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsSpatializationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bAttenuateWithLPF)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableListenerFocus)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, LPFRadiusMin)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, LPFRadiusMax)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, LPFFrequencyAtMin)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, LPFFrequencyAtMax)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, HPFFrequencyAtMin)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, HPFFrequencyAtMax)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableLogFrequencyScaling)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(AbsorptionMethodHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, CustomLowpassAirAbsorptionCurve)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsCustomAirAbsorptionCurveSelected))
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, CustomHighpassAirAbsorptionCurve)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsCustomAirAbsorptionCurveSelected))
		.EditCondition(GetIsAirAbsorptionEnabledAttribute(), nullptr);

	// Add the reverb send enabled handle
	/*LayoutBuilder.AddPropertyToCategory(bIsReverbSendEnabledHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(ReverbSendMethodHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ReverbWetLevelMin)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearMethodSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ReverbWetLevelMax)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearMethodSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, CustomReverbSendCurve)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsCustomReverbSendCurveSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ReverbDistanceMin)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearOrCustomReverbMethodSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ReverbDistanceMax)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearOrCustomReverbMethodSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ManualReverbSendLevel)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsManualReverbSendSelected))
		.EditCondition(GetIsReverbSendEnabledAttribute(), nullptr);*/

	// 
	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusAzimuth)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonFocusAzimuth)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusDistanceScale)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonFocusDistanceScale)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusPriorityScale)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonFocusPriorityScale)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusVolumeAttenuation)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, NonFocusVolumeAttenuation)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bEnableFocusInterpolation)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusAttackInterpSpeed)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, FocusReleaseInterpSpeed)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsFocusEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(bIsOcclusionEnabledHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, OcclusionTraceChannel)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, OcclusionLowPassFilterFrequency)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, OcclusionVolumeAttenuation)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, OcclusionInterpolationTime)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, bUseComplexCollisionForOcclusion)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsOcclusionEnabledAttribute(), nullptr);

	// Add the attenuation priority
	LayoutBuilder.AddPropertyToCategory(bIsPriorityAttenuationEnabledHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PriorityAttenuationMethodHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PriorityAttenuationMin)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsPriorityAttenuationLinearMethodSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PriorityAttenuationMax)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsPriorityAttenuationLinearMethodSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, CustomPriorityAttenuationCurve)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsCustomPriorityAttenuationCurveSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PriorityAttenuationDistanceMin)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearOrCustomPriorityAttenuationSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PriorityAttenuationDistanceMax)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsLinearOrCustomPriorityAttenuationSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, ManualPriorityAttenuation)))
		.Visibility(TAttribute<EVisibility>(this, &FAtomAttenuationSettingsCustomization::IsManualPriorityAttenuationSelected))
		.EditCondition(GetIsPriorityAttenuationEnabledAttribute(), nullptr);

	// Add the submix send priority
	LayoutBuilder.AddPropertyToCategory(bIsBusSendAttenuationEnabledHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, BusSendSettings)))
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(GetIsBusSendAttenuationEnabledAttribute(), nullptr);

	LayoutBuilder.AddPropertyToCategory(bIsSourceDataOverrideEnabledHandle)
		.Visibility(IsAttenuationOverriddenVisibleAttribute())
		.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);

	TSharedPtr<IPropertyHandle> PluginProperty = PropertyHandles.FindChecked(GET_MEMBER_NAME_CHECKED(FAtomAttenuationSettings, PluginSettings));
	uint32 NumPluginChildren = 0;
	PluginProperty->GetNumChildren(NumPluginChildren);
	for (uint32 i = 0; i < NumPluginChildren; ++i)
	{
		LayoutBuilder.AddPropertyToCategory(PluginProperty->GetChildHandle(i))
			.Visibility(IsAttenuationOverriddenVisibleAttribute())
			.EditCondition(IsAttenuationOverriddenAttribute(), nullptr);
	}

	// Set category display names
	LayoutBuilder.EditCategory("AttenuationDistance", LOCTEXT("AttenuationVolume", "Attenuation (Volume)"));
	LayoutBuilder.EditCategory("AttenuationSpatialization", LOCTEXT("AttenuationSpatialization", "Attenuation (Spatialization)"));
	LayoutBuilder.EditCategory("AttenuationOcclusion", LOCTEXT("AttenuationOcclusion", "Attenuation (Occlusion)"));
	LayoutBuilder.EditCategory("AttenuationBusSends", LOCTEXT("AttenuationBusSends", "Attenuation (Buses)"));
	//LayoutBuilder.EditCategory("AttenuationReverbSend", LOCTEXT("AttenuationReverbSend", "Attenuation (Reverb)"));
	LayoutBuilder.EditCategory("AttenuationListenerFocus", LOCTEXT("AttenuationListenerFocus", "Attenuation (Focus)"));
	LayoutBuilder.EditCategory("AttenuationPriority", LOCTEXT("AttenuationPriority", "Attenuation (Priority)"));
	LayoutBuilder.EditCategory("AttenuationAirAbsorption", LOCTEXT("AttenuationAirAbsorption", "Attenuation (Air Absorption)"));
	//LayoutBuilder.EditCategory("AttenuationPluginSettings", LOCTEXT("AttenuationPluginSettings", "Attenuation (Plugin Settings)"));
	LayoutBuilder.EditCategory("AttenuationSourceDataOverride", LOCTEXT("AttenuationSourceDataOverride", "Attenuation (Source Data Override)"));
	LayoutBuilder.SortCategories(AtomAttenuationSettingsUtils::SortCategories);

	/*if (PropertyHandles.Num() != 66)
	{
		ensureMsgf(false, TEXT("Unexpected property handle(s) customizing FAtomAttenuationSettings. %d handles found"), PropertyHandles.Num());
	}*/
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::IsAttenuationOverriddenAttribute() const
{
	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda([bInOverrideAttenuationHandle = bOverrideAttenuationHandle]()
	{
		return AtomAttenuationSettingsUtils::GetValue(bInOverrideAttenuationHandle);
	}
	));
}

TAttribute<EVisibility> FAtomAttenuationSettingsCustomization::IsAttenuationOverriddenVisibleAttribute() const
{
	return TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda([bInOverrideAttenuationHandle = bOverrideAttenuationHandle]()
	{
		return AtomAttenuationSettingsUtils::GetValue(bInOverrideAttenuationHandle)
			? EVisibility::Visible
			: EVisibility::Hidden;
	}
	));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsAttenuationEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsAttenuatedPropertyWeakPtr = bIsAttenuatedHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsAttenuatedPropertyWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakHandle = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsAttenuatedPropertyWeakHandle = bIsAttenuatedPropertyWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationPropertyWeakHandle);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsAttenuatedPropertyWeakHandle);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsFalloffModeEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsAttenuatedPropertyWeakPtr = bIsAttenuatedHandle;
	TWeakPtr<IPropertyHandle> DbAttenuationAtMaxHandleWeakPtr = DbAttenuationAtMaxHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsAttenuatedPropertyWeakPtr, DbAttenuationAtMaxHandleWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationPropertyPtr = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsAttenuatedPropertyPtr = bIsAttenuatedPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> DbAttenuationAtMaxHandlePtr = DbAttenuationAtMaxHandleWeakPtr.Pin();

		float AttenuationValue = -60.f;
		if (DbAttenuationAtMaxHandlePtr.IsValid())
		{
			DbAttenuationAtMaxHandlePtr->GetValue(AttenuationValue);
		}

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationPropertyPtr);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsAttenuatedPropertyPtr);
		Value &= AttenuationValue > -60.f;
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsFocusEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsFocusedPropertyWeakPtr = bIsFocusedHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsFocusedPropertyWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsFocusedProperty = bIsFocusedPropertyWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsFocusedProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsOcclusionEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsOcclusionPropertyWeakPtr = bIsOcclusionEnabledHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsOcclusionPropertyWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bOcclusionProperty = bIsOcclusionPropertyWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bOcclusionProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsSpatializationEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsSpatializedHandleWeakPtr = bIsSpatializedHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsSpatializedHandleWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsSpatializedProperty = bIsSpatializedHandleWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsSpatializedProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsAirAbsorptionEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsAirAbsorptionHandleWeakPtr = bIsAirAbsorptionEnabledHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsAirAbsorptionHandleWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsAirAbsorptionProperty = bIsAirAbsorptionHandleWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsAirAbsorptionProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

/*TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsReverbSendEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsReverbSendWeakPtr = bIsReverbSendEnabledHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsReverbSendWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsReverbSendProperty = bIsReverbSendWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsReverbSendProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}*/

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsPriorityAttenuationEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsPriorityAttenuationEnabledWeakPtr = bIsPriorityAttenuationEnabledHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsPriorityAttenuationEnabledWeakPtr]()
		{
			TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
			TSharedPtr<IPropertyHandle> bIsPriorityAttenuationEnabledProperty = bIsPriorityAttenuationEnabledWeakPtr.Pin();

			bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
			Value &= AtomAttenuationSettingsUtils::GetValue(bIsPriorityAttenuationEnabledProperty);
			return Value;
		};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

TAttribute<bool> FAtomAttenuationSettingsCustomization::GetIsBusSendAttenuationEnabledAttribute() const
{
	TWeakPtr<IPropertyHandle> bOverrideAttenuationPropertyWeakPtr = bOverrideAttenuationHandle;
	TWeakPtr<IPropertyHandle> bIsBusSendWeakPtr = bIsBusSendAttenuationEnabledHandle;

	auto Lambda = [bOverrideAttenuationPropertyWeakPtr, bIsBusSendWeakPtr]()
	{
		TSharedPtr<IPropertyHandle> bOverrideAttenuationProperty = bOverrideAttenuationPropertyWeakPtr.Pin();
		TSharedPtr<IPropertyHandle> bIsBusSendAttenuationEnabledProperty = bIsBusSendWeakPtr.Pin();

		bool Value = AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationProperty);
		Value &= AtomAttenuationSettingsUtils::GetValue(bIsBusSendAttenuationEnabledProperty);
		return Value;
	};

	return TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateLambda(Lambda));
}

/*EVisibility FAtomAttenuationSettingsCustomization::IsCustomReverbSendCurveSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 SendMethodValue;
	ReverbSendMethodHandle->GetValue(SendMethodValue);

	const EReverbSendMethod SendMethodType = (EReverbSendMethod)SendMethodValue;

	return (SendMethodType == EReverbSendMethod::CustomCurve ? EVisibility::Visible : EVisibility::Hidden);
}*/

/*EVisibility FAtomAttenuationSettingsCustomization::IsManualReverbSendSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 SendMethodValue;
	ReverbSendMethodHandle->GetValue(SendMethodValue);

	const EReverbSendMethod SendMethodType = (EReverbSendMethod)SendMethodValue;

	return (SendMethodType == EReverbSendMethod::Manual ? EVisibility::Visible : EVisibility::Hidden);
}*/

EVisibility FAtomAttenuationSettingsCustomization::IsConeSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAtomAttenuationShape AttenuationShape = (EAtomAttenuationShape)AttenuationShapeValue;

	return (AttenuationShape == EAtomAttenuationShape::Cone ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsSphereSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 AttenuationShapeValue;
	AttenuationShapeHandle->GetValue(AttenuationShapeValue);

	const EAtomAttenuationShape AttenuationShape = (EAtomAttenuationShape)AttenuationShapeValue;

	return (AttenuationShape == EAtomAttenuationShape::Sphere ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsCustomAirAbsorptionCurveSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 MethodValue;
	AbsorptionMethodHandle->GetValue(MethodValue);

	const EAtomAirAbsorptionMethod MethodType = (EAtomAirAbsorptionMethod)MethodValue;

	return (MethodType == EAtomAirAbsorptionMethod::CustomCurve ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsPriorityAttenuationLinearMethodSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 Value;
	PriorityAttenuationMethodHandle->GetValue(Value);

	const EPriorityAttenuationMethod MethodType = (EPriorityAttenuationMethod)Value;

	return (MethodType == EPriorityAttenuationMethod::Linear ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsCustomPriorityAttenuationCurveSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 Value;
	PriorityAttenuationMethodHandle->GetValue(Value);

	const EPriorityAttenuationMethod MethodType = (EPriorityAttenuationMethod)Value;

	return (MethodType == EPriorityAttenuationMethod::CustomCurve ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsLinearOrCustomPriorityAttenuationSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 Value;
	PriorityAttenuationMethodHandle->GetValue(Value);

	const EPriorityAttenuationMethod MethodType = (EPriorityAttenuationMethod)Value;

	return (MethodType != EPriorityAttenuationMethod::Manual ? EVisibility::Visible : EVisibility::Hidden);
}

EVisibility FAtomAttenuationSettingsCustomization::IsManualPriorityAttenuationSelected() const
{
	if (!AtomAttenuationSettingsUtils::GetValue(bOverrideAttenuationHandle))
	{
		return EVisibility::Hidden;
	}

	uint8 Value;
	PriorityAttenuationMethodHandle->GetValue(Value);

	const EPriorityAttenuationMethod MethodType = (EPriorityAttenuationMethod)Value;

	return (MethodType == EPriorityAttenuationMethod::Manual ? EVisibility::Visible : EVisibility::Hidden);
}

#undef LOCTEXT_NAMESPACE
