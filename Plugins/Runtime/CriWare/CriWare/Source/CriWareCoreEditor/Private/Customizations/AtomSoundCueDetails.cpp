/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundCueDetails.cpp
 *
 ****************************************************************************/

#include "AtomSoundCueDetails.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"

#include "CriWareEditor.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundCue.h"

// Forward Definitions
class UAtomSoundBase;

#define LOCTEXT_NAMESPACE "AtomSoundCueDetails"

TSharedRef<IDetailCustomization> FAtomSoundCueDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundCueDetails);
}

void FAtomSoundCueDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& DetailCategory = DetailLayout.EditCategory("Voice Management", FText::GetEmpty(), ECategoryPriority::Default);
	
	TSharedRef<IPropertyHandle> OverrideVirtualizationModeHandle = DetailLayout.GetProperty("bOverrideVirtualizationMode", UAtomSoundBase::StaticClass());
	TSharedRef<IPropertyHandle> VirtualizationModeHandle = DetailLayout.GetProperty("VirtualizationMode", UAtomSoundBase::StaticClass());
	TSharedRef<IPropertyHandle> UseDynamicVoicesHandle = DetailLayout.GetProperty("bUseDynamicVoices");
	TSharedRef<IPropertyHandle> VoiceLimitHandle = DetailLayout.GetProperty("VoiceLimit");
	TSharedRef<IPropertyHandle> VoiceSettingsOverridesHandle = DetailLayout.GetProperty("VoiceSettingsOverrides");

	DetailCategory.AddProperty(OverrideVirtualizationModeHandle);
	DetailCategory.AddProperty(VirtualizationModeHandle);

	if (UseDynamicVoicesHandle->IsValidHandle())
	{
		DetailCategory.AddProperty(UseDynamicVoicesHandle).Visibility(TAttribute<EVisibility>::CreateLambda([this]()
			{
				auto Runtime = FAtomRuntimeManager::Get()->GetMainAtomRuntimeHandle();
				return Runtime.IsValid() && Runtime->IsUsingFixedVoicePools() ? EVisibility::Visible : EVisibility::Hidden;
			}));
	}

	if (VoiceLimitHandle->IsValidHandle())
	{
		DetailCategory.AddProperty(VoiceLimitHandle).EditCondition(TAttribute<bool>::CreateLambda([this, UseDynamicVoicesHandle]()
			{
				bool bUseDynamicVoices = false;
				UseDynamicVoicesHandle->GetValue(bUseDynamicVoices);
				auto Runtime = FAtomRuntimeManager::Get()->GetMainAtomRuntimeHandle();
				return Runtime.IsValid() && Runtime->IsUsingFixedVoicePools() ? bUseDynamicVoices : true;
			}), nullptr);
	}

	if (VoiceSettingsOverridesHandle->IsValidHandle())
	{
		DetailCategory.AddProperty(VoiceSettingsOverridesHandle).EditCondition(TAttribute<bool>::CreateLambda([this, UseDynamicVoicesHandle]()
		{
				bool bUseDynamicVoices = false;
				UseDynamicVoicesHandle->GetValue(bUseDynamicVoices);
				auto Runtime = FAtomRuntimeManager::Get()->GetMainAtomRuntimeHandle();
				return Runtime.IsValid() && Runtime->IsUsingFixedVoicePools() ? bUseDynamicVoices : true;
		}), nullptr);
	}

	// Force root level categories order
	DetailLayout.SortCategories([=](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		IDetailCategoryBuilder* const* CueCategoryPtr = AllCategoryMap.Find("Cue");

		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			int32 SortOrder = CueCategoryPtr ? (*CueCategoryPtr)->GetSortOrder() : 0;
			const FName CategoryName = Pair.Key;

			if (CategoryName == "Cue")
			{
				SortOrder += 0;
			}
			else if (CategoryName == "Sound")
			{
				SortOrder += 10;
			}
			else if (CategoryName == "Attenuation")
			{
				SortOrder += 20;
			}
			else if (CategoryName == "Modulation")
			{
				SortOrder += 30;
			}
			else if (CategoryName == "Effects")
			{
				SortOrder += 40;
			}
			else if (CategoryName == "Voice Management")
			{
				SortOrder += 50;
			}
			else if (CategoryName == "Developer")
			{
				SortOrder += 60;
			}
			else if (CategoryName == "Info")
			{
				SortOrder += 70;
			}
			else if (CategoryName == "Advanced")
			{
				SortOrder += 80;
			}

			Pair.Value->SetSortOrder(SortOrder);
		}
	});
}

#undef LOCTEXT_NAMESPACE
