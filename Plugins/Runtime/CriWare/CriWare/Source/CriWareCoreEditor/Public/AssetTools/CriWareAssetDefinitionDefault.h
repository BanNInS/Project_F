/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2025 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : CriWareAssetDefinitionDefault.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "Styling/SlateBrush.h"
#include "Templates/SharedPointer.h"
#include "UObject/StrongObjectPtr.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"

#include "CriWareAssetDefinitionDefault.generated.h"

UCLASS(Abstract, MinimalAPI)
class UCriWareAssetDefinitionDefault : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:

	// UAssetDefinitionDefault Begin
	CRIWARECOREEDITOR_API virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override;
	CRIWARECOREEDITOR_API virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& AssetData) const override;
	// UAssetDefinitionDefault End

	CRIWARECOREEDITOR_API virtual const FSlateBrush* GetIconOverlayBrush(const FAssetData& AssetData) const { return nullptr; }
	CRIWARECOREEDITOR_API virtual EVisibility GetIconOverlayVisibility(const FAssetData& AssetData) const { return EVisibility::Visible; }

private:
	TSharedPtr<SWidget> GetThumbnailWidget(const FAssetData& InAssetData, const FName InClassName) const;
	void CreateThumbnailBrushRenderingResources();

	TSharedPtr<FWidgetRenderer> WidgetRenderer;
	TStrongObjectPtr<UTextureRenderTarget2D> TextureRenderTarget;
	FSlateBrush ThumbnailBrush;
};
