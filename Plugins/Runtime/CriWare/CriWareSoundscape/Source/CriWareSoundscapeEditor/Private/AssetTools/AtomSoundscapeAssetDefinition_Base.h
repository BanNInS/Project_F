/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareSoundscape
 * File     : AtomSoundscapeAssetDefinition_Base.h
 *
 ****************************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"

#include "AtomSoundscapeAssetDefinition_Base.generated.h"

UCLASS(Abstract, MinimalAPI)
class UAtomSoundscapeAssetDefinition_Base : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:

	// UAssetDefinitionDefault Begin
	CRIWARESOUNDSCAPEEDITOR_API virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	CRIWARESOUNDSCAPEEDITOR_API virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& AssetData) const override;
	// UAssetDefinitionDefault End

	CRIWARESOUNDSCAPEEDITOR_API virtual EVisibility GetIconOverlayVisibility(const FAssetData& AssetData) const { return EVisibility::Visible; }
};
