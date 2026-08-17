/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAssetDefinition_Base.cpp
 *
 ****************************************************************************/

#include "AssetTools/AtomAssetDefinition_Base.h"

#include "Styling/SlateStyleRegistry.h"

TConstArrayView<FAssetCategoryPath> UAtomAssetDefinition_Base::GetAssetCategories() const
{
	static const auto Categories = { FAssetCategoryPath(NSLOCTEXT("CriWareCoreEditor", "AdxAssetCategory", "ADX Atom"))};
	return Categories;
}

const FSlateBrush* UAtomAssetDefinition_Base::GetIconOverlayBrush(const FAssetData& AssetData) const
{
	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle("AtomEditorStyle");
	if (ensure(AtomEditorStyle))
	{
		return AtomEditorStyle->GetBrush("CriWare.ADXThumbnail"); // should be better with svg
	}

	return nullptr;
}
