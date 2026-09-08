/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackEditorToolkit.h
 *
 ****************************************************************************/

#pragma once

#include "EditorUndoClient.h"
#include "Framework/Docking/TabManager.h"
#include "GraphEditor.h"
#include "UObject/Object.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CriWareCoreEditorPrivate.h"
#include "Widgets/SAtomEffectEditor.h"

// Forward Declarations
class UEdGraphPin;
class UAtomRackBase;
class UEdGraph;

class IAtomRackEditorToolkit : public FAssetEditorToolkit
{
public:

	/**
	* Creates a new Atom rack
	*
	* @param	FromPin		Pin that was dragged to create sound class
	* @param	Location	Location for new Atom rack
	* @param	Name		Name of the new Atom rack
	*/
	
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	virtual void CreateAtomRack(UEdGraphPin* FromPin, FVector2f Location, const FString& Name) = 0;
#else
	virtual void CreateAtomRack(UEdGraphPin* FromPin, const FVector2D Location, const FString& Name) = 0;
#endif
};

class FAtomRackEditorToolkit 
	: public IAtomRackEditorToolkit
	, public FGCObject
	, public FEditorUndoClient
{
public:

	/** Virtual destructor. */
	virtual ~FAtomRackEditorToolkit();

	/**
	 * Initializes the editor tool kit.
	 *
	 * @param InObjectToEdit The UObject asset to edit.
	 * @param InMode The mode to create the toolkit in.
	 * @param InToolkitHost The toolkit host.
	 */
	void Initialize(UObject* ObjectToEdit, const EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost);

public:

	//~ FAssetEditorToolkit interface

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;

public:

	//~ IToolkit interface

	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;

public:

	//~ FGCObject interface

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FAtomRackEditor");
	}

public:

	//~ Begin IAtomRackEditorToolkit interface
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	void CreateAtomRack(UEdGraphPin* FromPin, FVector2f Location, const FString& Name) override;
#else
	void CreateAtomRack(UEdGraphPin* FromPin, const FVector2D Location, const FString& Name) override;
#endif
	//~ End IAtomRackEditorToolkit interface

	void AddMissingEditableRacks();

	/** Select node associated with the provided rack */
	void SelectRacks(TSet<UAtomRackBase*>& InRacks);

	/** Returns current graph handled by editor */
	UEdGraph* GetGraph();

protected:

	//~ FEditorUndoClient interface

	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:

	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EffectEditor(const FSpawnTabArgs& Args);

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets(UAtomRackBase* InAtomRack);

	/** Create new Atom effect editor widget */
	TSharedRef<SAtomEffectEditor> CreateEffectEditorWidget(UAtomRackBase* InAtomRack);

	/** Create new graph editor widget */
	TSharedRef<SGraphEditor> CreateGraphEditorWidget(UAtomRackBase* InAtomRack);

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection);

	/** Called to create context menu when right-clicking on graph */
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);
#else
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);
#endif

	/** Adds all children of provided root rack as editable */
	void AddEditableRackChildren(UAtomRackBase* RootRack);

	/** Select every node in the graph */
	void SelectAllNodes();

	/** Whether we can select every node */
	bool CanSelectAllNodes() const;

	/** Remove the currently selected nodes from editor view*/
	void RemoveSelectedNodes();

	/** Whether we are able to remove the currently selected nodes */
	bool CanRemoveNodes() const;

	/** Called to undo the last action */
	void UndoGraphAction();

	/** Called to redo the last undone action */
	void RedoGraphAction();

private:
	/** Graph Editor */
	TSharedPtr<SGraphEditor> GraphEditor;

	/** Property View */
	TSharedPtr<IDetailsView> DetailsView;

	/** Atom Effect Editor */
	TSharedPtr<SAtomEffectEditor> EffectEditor;

	/** Command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;
};
