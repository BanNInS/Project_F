/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAssetView.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"

class CRIWARECOREEDITOR_API SAssetView
	: public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SAssetView, SCompoundWidget)

	/** Called when a valid asset is dropped */
	DECLARE_DELEGATE_TwoParams(FOnAssetDropped, const FDragDropEvent&, const FAssetData&);

	/** Called when we need to check if an asset type is valid for dropping and also will have a reason if it is not */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FIsAssetAcceptableForDropWithReason, const FAssetData&, FText&);

public:

	SLATE_BEGIN_ARGS(SAssetView)
		: _AssetData()
		, _DisplayThumbnail(true)
		, _ThumbnailSize(64.0f)
		, _OpenAssetOnDoubleClick(false)
		, _AllowAssetDragging(false)
		, _AllowAssetDropping(false)
	{}

		/** The object displayed in this asset view */
		SLATE_ATTRIBUTE(FAssetData, AssetData)

		/** Whether a Thmbnail is displayed if object is an asset. */
		SLATE_ARGUMENT(bool, DisplayThumbnail)

		/** The size in pixel of the thumbnail to display. */
		SLATE_ARGUMENT(float, ThumbnailSize)

		/** Open asset editor when mouse is double clicked on the widget. */
		SLATE_ARGUMENT(bool, OpenAssetOnDoubleClick)

		/** Whether possible to drag the asset outside this view. */
		SLATE_ARGUMENT(bool, AllowAssetDragging)
		
		/** Whether possible to drop an asset to this view. */
		SLATE_ARGUMENT(bool, AllowAssetDropping)

		/** Called when an asset was dropped to this view. */
		SLATE_EVENT(FOnAssetDropped, OnAssetDropped)

		/** Called when we need to check if an asset type is valid for dropping and also will have a reason if it is not */
		SLATE_EVENT(FIsAssetAcceptableForDropWithReason, OnIsAssetAcceptableForDropWithReason)

		/** Called when this text is double clicked */
		SLATE_EVENT(FPointerEventHandler, OnDoubleClicked)

	SLATE_END_ARGS()

	SAssetView();
	~SAssetView();

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * Gets the obejct assigned to this asset view
	 *
	 * @return	This asset view's object
	 */
	void GetObject(FAssetData& OutAssetData) const
	{
		OutAssetData = AssetData.Get();
	}

	//~ Begin SWidget Interface
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& PointerEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& Geometry, const FPointerEvent& PointerEvent) override;
	//~ End SWidget Interface
public:

	/** Sets the object for this asset view. */
	void SetObject(const FAssetData& InAssetData);

private:

	TAttribute<FAssetData> AssetData;

	bool bDisplayThumbnail;

	float ThumbnailSize;

	bool bOpenAssetOnDoubleClick;

	bool bAllowAssetDragging;

	bool bAllowAssetDropping;

	FOnAssetDropped OnAssetDropped;

	FIsAssetAcceptableForDropWithReason OnIsAssetAcceptableForDropWithReason;

	TSharedPtr<SOverlay> ThumbnailOverlay;

	TSharedRef<SWidget> CreateAssetView(bool bWithThumbnail, float ThumbnailSize);
};
