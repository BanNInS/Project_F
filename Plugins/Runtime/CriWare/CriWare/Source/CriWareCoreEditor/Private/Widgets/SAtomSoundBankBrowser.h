/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : CriWareCoreEditor
* File     : SAtomSoundBankBrowser.h
*
****************************************************************************/

#pragma once

#include "CoreTypes.h"
#include "ContentBrowserDelegates.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
#include "AssetRegistry/AssetData.h"
#else
#include "AssetData.h"
#endif

// Forward Definitions
class SWidget;
class UAtomSoundBank;
class ISlateStyle;
class UToolMenu;
class SAtomSoundBrowser;

/**
 * Implements the sound library of the SoundBank asset editor.
 */
class SAtomSoundBankBrowser
	: public SCompoundWidget
{
public:

	/** Delegate that is executed when the check box state changes */
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, const TArray<UObject*>&);


	SLATE_BEGIN_ARGS(SAtomSoundBankBrowser)
	{ }

		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)

	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs The declaration data for this widget.
	 * @param InSoundBank The SoundBank asset to show the details for.
	 * @param InStyleSet The style set to use.
	 */
	void Construct(const FArguments& InArgs, UAtomSoundBank& InSoundBank);

	/* Register menus associated with this widget */
	//static void RegisterMenus();

private:

	/** Callback for double-clicking an asset in the asset picker. */
	void OnAssetDoubleClicked(const struct FAssetData& AssetData);

	/** Callback for pressing Enter on a selected asset in the asset picker. */
	void OnAssetEnterPressed(const TArray<FAssetData>& SelectedAssets);

	/** Callback for selecting asset in the asset picker. */
	void OnAssetSelected(const FAssetData& SelectedAsset);

	/** Callback for getting the context menu of an asset in the asset picker. */
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);

	/** */
	FString GetStringValueForCustomColumn(FAssetData& AssetData, FName ColumnName) const;

	/** */
	FText GetDisplayTextForCustomColumn(FAssetData& AssetData, FName ColumnName) const;

	/** */
	void AddAssetFileContextMenuOptions(UToolMenu* Menu);
	void AddAssetTypeMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects, const UClass* CommonClass);
	void AddAssetActionMenuOptions(UToolMenu* Menu, const UClass* CommonClass);
	void AddCommonMenuOptions(UToolMenu* Menu, const TArray<UObject*>& SelectedObjects);
	void AddReferenceMenuOptions(UToolMenu* Menu);

	/** */
	void ExecuteEditItems(TArray<UObject*> ObjectsToEdit);
	void ExecutePropertyMatrix(TArray<UObject*> ObjectsForPropertiesMenu);
	FText GetExecutePropertyMatrixTooltip() const;
	void ExecuteCopyReference();
	void ExecuteReferenceViewer();
	void ExecuteSizeMap();
	bool IsAnythingSelected() const;

private:

	/** Pointer to the SoundBank asset that is being viewed. */
	UAtomSoundBank* SoundBank;

	/** Commands handled by this widget */
	TSharedPtr<FUICommandList> Commands;

	/** Delegate called when selected assets have changed */
	FOnSelectionChanged OnSelectionChanged;

	/** Name of the asset picker asset context menu */
	static const FName AssetPickerAssetContextMenuName;

	/** Custom columns of the asset picker */
	static const FName ColumnID_WaveBank;
	static const FName ColumnID_Loading;

	/** Delegates to interact with asset view */
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	FSetARFilterDelegate SetFilterDelegate;
};
