/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAttenuationActions.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomAttenuation.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomAttenuationAssetDefinition.generated.h"

UCLASS()
class UAtomAttenuationAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomAttenuation", "Atom Attenuation"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultAttenuationColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomAttenuation::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto PinnedCategories = { Super::GetAssetCategories().Last() };
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetSoundAttenuationSubMenu", "Spatialization") };

		if (GetDefault<UCriWareCoreEditorAtomSettings>()->bPinAtomSoundAttenuationInAssetMenu)
		{
			return PinnedCategories;
		}		
		return Categories;
	}
	// UAssetDefinition End
};
