/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundProxyDetails.cpp
 *
 ****************************************************************************/

#include "AtomSoundProxyDetails.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundProxy.h"

#define LOCTEXT_NAMESPACE "AtomSoundProxyDetails"

TSharedRef<IDetailCustomization> FAtomSoundProxyDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundProxyDetails);
}

void FAtomSoundProxyDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Force root level categories order
	DetailLayout.SortCategories([=](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
		{
			IDetailCategoryBuilder* const* WaveCategoryPtr = AllCategoryMap.Find("Sound");

			for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
			{
				int32 SortOrder = WaveCategoryPtr ? (*WaveCategoryPtr)->GetSortOrder() : 0;
				const FName CategoryName = Pair.Key;

				if (CategoryName == "Sound")
				{
					SortOrder += 0;
				}
				else if (CategoryName == "Attenuation")
				{
					SortOrder += 10;
				}
				else if (CategoryName == "Modulation")
				{
					SortOrder += 20;
				}
				else if (CategoryName == "Effects")
				{
					SortOrder += 30;
				}
				else if (CategoryName == "Voice Management")
				{
					SortOrder += 40;
				}
				else if (CategoryName == "Developer")
				{
					SortOrder += 50;
				}
				else if (CategoryName == "Info")
				{
					SortOrder += 60;
				}
				else if (CategoryName == "Advanced")
				{
					SortOrder += 70;
				}

				Pair.Value->SetSortOrder(SortOrder);
			}
		});

	// Rename properties
	TSharedRef<IPropertyHandle> SoundClassHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, SoundClassObject), UAtomSoundBase::StaticClass());
	SoundClassHandle->SetPropertyDisplayName(LOCTEXT("SoundProxy_SoundClassName", "Class Overrides"));

	TSharedRef<IPropertyHandle> AttenuationSettingsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, AttenuationSettings), UAtomSoundBase::StaticClass());
	AttenuationSettingsHandle->SetPropertyDisplayName(LOCTEXT("SoundProxy_AttenuationName", "Attenuation Settings Overrides"));

	TSharedRef<IPropertyHandle> AtomRackHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, AtomRack), UAtomSoundBase::StaticClass());
	AtomRackHandle->SetPropertyDisplayName(LOCTEXT("SoundProxy_RackName", "Rack Overrides"));

	TSharedRef<IPropertyHandle> OverPriHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, bOverridePriority));
	TSharedRef<IPropertyHandle> PriHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, Priority), UAtomSoundBase::StaticClass());

	// Add edit conditions for properties
	if (IDetailPropertyRow* DetailProperty = DetailLayout.EditDefaultProperty(PriHandle))
	{
		DetailProperty->EditCondition(TAttribute<bool>::CreateSP(this, &FAtomSoundProxyDetails::CanEditProperty, OverPriHandle), {});
	}

	TSharedRef<IPropertyHandle> OverModHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, bOverrideModulationSettings));
	TSharedRef<IPropertyHandle> ModHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UAtomSoundProxy, ModulationSettings), UAtomSoundBase::StaticClass());

	if (IDetailPropertyRow* DetailProperty = DetailLayout.EditDefaultProperty(ModHandle))
	{
		DetailProperty->EditCondition(TAttribute<bool>::CreateSP(this, &FAtomSoundProxyDetails::CanEditProperty, OverModHandle), {});
	}
}

bool FAtomSoundProxyDetails::CanEditProperty(TSharedRef<IPropertyHandle> OverridePropertyHandle) const
{
	bool Value = false;
	if (OverridePropertyHandle->IsValidHandle())
	{
		OverridePropertyHandle->GetValue(Value);
	}
	return Value;
}


#undef LOCTEXT_NAMESPACE
