/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEffectPresetEditorToolkit.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "EditorUndoClient.h"
#include "Math/Color.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/GCObject.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SWidget.h"
#include "Runtime/Launch/Resources/Version.h"

// Forward Declarations
class UClass;
class UAtomEffectPreset;
class UWidgetBlueprint;

class FAtomEffectPresetEditorToolkit
	: public FAssetEditorToolkit
	, public FNotifyHook
	, public FEditorUndoClient
	, public FGCObject 
{
public:

	FAtomEffectPresetEditorToolkit();
	virtual ~FAtomEffectPresetEditorToolkit() = default;

	void Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>&InitToolkitHost, UAtomEffectPreset * PresetToEdit, const TArray<UUserWidget*>&InWidgetBlueprints);

	//~ Begin FAssetEditorToolkit interface
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual bool CloseWindow(EAssetEditorCloseReason InCloseReason) override;
#else
	virtual bool CloseWindow() override;
#endif
	virtual FName GetEditorName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	//~ End FAssetEditorToolkit interface

	//~ Begin FNotifyHook interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged) override;
	//~ End FNotifyHook interface

protected:

	//~ Begin FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient interface

private:

	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("SAtomEffectPresetWidget");
	}
	//~ Begin FGCObject Interface

	/** Initializes all preset user widgets. */
	void InitPresetWidgets(const TArray<UUserWidget*>& InWidgets);

	/**	Spawns the tab allowing for editing/viewing the blueprint widget for the associated AtomEffectPreset */
	TSharedRef<SDockTab> SpawnTab_UserWidgetEditor(const FSpawnTabArgs& Args, int32 WidgetIndex);

	/**	Spawns the tab allowing for editing/viewing details panel */
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

	/** Get the orientation for the snap value controls. */
	EOrientation GetSnapLabelOrientation() const;

	/** Properties tab */
	TSharedPtr<IDetailsView> PropertiesView;

	/** Settings Editor App Identifier */
	static const FName AppIdentifier;

	TStrongObjectPtr<UAtomEffectPreset> AtomEffectPreset;
	TArray<TObjectPtr<UUserWidget>> UserWidgets;

	/** Tab Ids */
	static const FName PropertiesTabId;
	static const FName UserWidgetTabId;
};
