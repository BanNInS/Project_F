/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareAudioLinkEditor
 * File     : AtomAudioLinkSettingsAssetDefinition.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "AtomAudioLinkSettings.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomAudioLinkSettingsAssetDefinition.generated.h"

UCLASS()
class UAtomAudioLinkSettingsAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomAudioLinkSettings", "Atom AudioLink Settings"); }
	virtual FLinearColor GetAssetColor() const override { return FColor::Silver; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomAudioLinkSettings::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomAudioLinkSubMenu", "AudioLink") };
		return Categories;
	}
	// UAssetDefinition End
};
