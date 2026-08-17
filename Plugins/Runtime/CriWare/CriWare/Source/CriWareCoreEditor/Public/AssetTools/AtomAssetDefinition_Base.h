/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAssetDefinition_Base.h
 *
 ****************************************************************************/

#pragma once

#include "CriWareAssetDefinitionDefault.h"

#include "AtomAssetDefinition_Base.generated.h"

struct FToolMenuSection;

UCLASS(Abstract, MinimalAPI)
class UAtomAssetDefinition_Base : public UCriWareAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinitionDefault Begin
	CRIWARECOREEDITOR_API virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	// UAssetDefinitionDefault End

	CRIWARECOREEDITOR_API virtual const FSlateBrush* GetIconOverlayBrush(const FAssetData& AssetData) const override;

	CRIWARECOREEDITOR_API virtual void GetAssetActions(FToolMenuSection& Section) const {}
};
