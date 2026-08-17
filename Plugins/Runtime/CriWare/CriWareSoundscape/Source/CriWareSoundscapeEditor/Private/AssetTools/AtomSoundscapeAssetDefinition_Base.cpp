/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareSoundscape
 * File     : AtomSoundscapeAssetDefinition_Base.cpp
 *
 ****************************************************************************/

#include "AssetTools/AtomSoundscapeAssetDefinition_Base.h"

#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"

TConstArrayView<FAssetCategoryPath> UAtomSoundscapeAssetDefinition_Base::GetAssetCategories() const
{
	static const auto Categories = { FAssetCategoryPath(NSLOCTEXT("CriWareCoreEditor", "AdxAssetCategory", "ADX Atom")) };
	return Categories;
}

TSharedPtr<SWidget> UAtomSoundscapeAssetDefinition_Base::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	if (GetIconOverlayVisibility(AssetData) == EVisibility::Visible)
	{
		const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle("AtomEditorStyle");
		if (ensure(AtomEditorStyle))
		{
			const FSlateBrush* Icon = AtomEditorStyle->GetBrush("CriWare.ADXThumbnail"); // should be better with svg
			const float Ratio = 0.3f;

			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f - Ratio)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(Ratio)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f - Ratio)
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Fill)
					.FillHeight(Ratio)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetNoBrush())
						.Visibility(EVisibility::HitTestInvisible)
						.Padding(FMargin(0.0f, 0.0f, 6.0f, 6.0f))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SImage)
								.Image(Icon)
						]
					]
				];
		}
	}

	return SNullWidget::NullWidget;
}