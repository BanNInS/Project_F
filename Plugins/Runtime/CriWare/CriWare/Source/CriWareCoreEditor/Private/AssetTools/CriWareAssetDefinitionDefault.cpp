/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2025 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : CriWareAssetDefinitionDefault.cpp
 *
 ****************************************************************************/

#include "AssetTools/CriWareAssetDefinitionDefault.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "ClassIconFinder.h"

#include "CriWareCoreEditorAtomSettings.h"

TSharedPtr<SWidget> UCriWareAssetDefinitionDefault::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	if (GetIconOverlayVisibility(AssetData) == EVisibility::Visible)
	{
		const FSlateBrush* Icon = GetIconOverlayBrush(AssetData);
		if (Icon)
		{
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

TSharedPtr<SWidget> UCriWareAssetDefinitionDefault::GetThumbnailWidget(const FAssetData& InAssetData, const FName InClassName) const
{
	const FName DefaultThumbnail = FName(*FString::Printf(TEXT("ClassThumbnail.%s"), *InClassName.ToString()));
	const FSlateBrush* ThumbnailClassBrush = FClassIconFinder::FindThumbnailForClass(InAssetData.GetClass(), DefaultThumbnail);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetNoBrush())
			.Visibility(EVisibility::HitTestInvisible)
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(ThumbnailClassBrush)
			]
		]
		+ SOverlay::Slot()
		[
			GetThumbnailOverlay(InAssetData).ToSharedRef()
		];
}

const FSlateBrush* UCriWareAssetDefinitionDefault::GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const
{
	const bool bHideIconOverlay = GetDefault<UCriWareCoreEditorAtomSettings>()->bHideAssetThumbnailIconOverlay;
	if (!bHideIconOverlay && GetIconOverlayVisibility(InAssetData) == EVisibility::Visible)
	{
		if (!TextureRenderTarget.IsValid())
		{
			const_cast<UCriWareAssetDefinitionDefault*>(this)->CreateThumbnailBrushRenderingResources();
		}

		check(TextureRenderTarget.IsValid());

		const TSharedPtr<SWidget> ThumbnailWidget = GetThumbnailWidget(InAssetData, InClassName);	
		
		WidgetRenderer->DrawWidget(TextureRenderTarget.Get(), ThumbnailWidget.ToSharedRef(), FVector2D(256, 256), 0, false);

		return &ThumbnailBrush;
	}

	return nullptr;
}

void UCriWareAssetDefinitionDefault::CreateThumbnailBrushRenderingResources()
{
	const auto DrawSize = FVector2D(256, 256);
	const bool bIsLinearSpace = true;
	const EPixelFormat requestedFormat = FSlateApplication::Get().GetRenderer()->GetSlateRecommendedColorFormat();
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->Filter = TF_Bilinear;
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->SRGB = bIsLinearSpace;
	RenderTarget->TargetGamma = 1;
	RenderTarget->InitCustomFormat(FMath::FloorToInt32(DrawSize.X), FMath::FloorToInt32(DrawSize.Y), requestedFormat, bIsLinearSpace);
	RenderTarget->UpdateResourceImmediate(true);

	WidgetRenderer = MakeShared<FWidgetRenderer>(false);
	TextureRenderTarget = TStrongObjectPtr<UTextureRenderTarget2D>(RenderTarget);

	ThumbnailBrush.SetImageSize(DrawSize);
	ThumbnailBrush.SetResourceObject(TextureRenderTarget.Get());
}
