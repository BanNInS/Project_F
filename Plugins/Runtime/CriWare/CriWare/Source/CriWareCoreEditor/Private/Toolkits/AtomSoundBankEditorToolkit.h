/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankEditorToolkit.h
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

// Forward Definitions
class UAtomSoundBank;
class SAtomSoundBrowser;

class FAtomSoundBankEditorToolkit
	: public FAssetEditorToolkit
	, public FNotifyHook
	, public FGCObject
	, public FEditorUndoClient
{
public:

	FAtomSoundBankEditorToolkit();
	virtual ~FAtomSoundBankEditorToolkit() = default;

	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAtomSoundBank* SoundBankToEdit);

	//~ Begin FAssetEditorToolkit interface
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual bool CloseWindow(EAssetEditorCloseReason InCloseReason) override;
#else
	virtual bool CloseWindow() override;
#endif
	virtual FName GetEditorName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	//~ End FAssetEditorToolkit interface

	//~ Begin FNotifyHook interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FEditPropertyChain* PropertyThatChanged) override;
	//~ End FNotifyHook interface

	//~ Begin FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FAtomSoundBankEditor");
	}
	//~ End FGCObject interface

protected:

	//~ Begin FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FEditorUndoClient interface

private:

	/**	Spawns the tab allowing for editing/viewing details panel */
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

	/**	Spawns the tab to list the sounds in the bank */
	TSharedRef<SDockTab> SpawnTab_BankBrowser(const FSpawnTabArgs& Args);

	/**	Spawns the tab to list the sounds in the bank */
	TSharedRef<SDockTab> SpawnTab_SoundBrowser(const FSpawnTabArgs& Args);

	/**	Spawns the tab allowing for editing/viewing the sound selected in the sound browser */
	TSharedRef<SDockTab> SpawnTab_SoundProperties(const FSpawnTabArgs& Args);

	/** Called when bank selection is modified in bank browser */
	void OnBankSelectionChanged(const TArray<UObject*>& NewSelection);
	
	/** Called when bank selection is modified in sound browser */
	void OnSoundSelectionChanged(const TArray<UObject*>& NewSelection);

	/** Get the orientation for the snap value controls. */
	EOrientation GetSnapLabelOrientation() const;

	/** Properties tab */
	TSharedPtr<IDetailsView> PropertiesView;

	/** Selected sounds Properties tab */
	TSharedPtr<IDetailsView> SoundPropertiesView;

	/** Sound browser tab */
	TSharedPtr<SAtomSoundBrowser> SoundBrowser;

	/** Settings Editor App Identifier */
	static const FName AppIdentifier;

	TStrongObjectPtr<UAtomSoundBank> SoundBank;

	/** Tab Ids */
	static const FName PropertiesTabId;
	static const FName BankBrowserTabId;
	static const FName SoundBrowserTabId;
	static const FName SoundPropertiesTabId;
};

