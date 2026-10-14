/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundWaveDetails.cpp
 *
 ****************************************************************************/

#include "AtomSoundWaveDetails.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

#include "CriWareEditor.h"
#include "Atom/AtomSoundWave.h"

// Forward Definitions
class UAtomSoundBase;

#define LOCTEXT_NAMESPACE "AtomSoundWaveDetails"

TSharedRef<IDetailCustomization> FAtomSoundWaveDetails::MakeInstance()
{
	return MakeShareable(new FAtomSoundWaveDetails);
}

void FAtomSoundWaveDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Force root level categories order
	DetailLayout.SortCategories([=](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		IDetailCategoryBuilder* const* WaveCategoryPtr = AllCategoryMap.Find("Wave");

		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : AllCategoryMap)
		{
			int32 SortOrder = WaveCategoryPtr ? (*WaveCategoryPtr)->GetSortOrder() : 0;
			const FName CategoryName = Pair.Key;

			if (CategoryName == "Wave")
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
