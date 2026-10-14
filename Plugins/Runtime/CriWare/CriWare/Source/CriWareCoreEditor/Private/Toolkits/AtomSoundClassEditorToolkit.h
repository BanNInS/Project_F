
#pragma once

#include "AudioDefines.h"
#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Toolkits/IToolkitHost.h"
#include "EditorUndoClient.h"
#include "GraphEditor.h"

#include "CriWareDefines.h"

// Forward Declarations
class IDetailsView;
class UEdGraph;
class UAtomSoundClass;

namespace Atom
{
	class FAtomDebugger;
}

class IAtomSoundClassEditorToolkit
	: public FAssetEditorToolkit
{
public:

	/**
	* Creates a new Atom sound class
	*
	* @param	FromPin		Pin that was dragged to create sound class
	* @param	Location	Location for new sound class
	* @param	Name		Name of the new sound class
	*/
	virtual void CreateSoundClass(class UEdGraphPin* FromPin, const FVector2D& Location, const FString& Name) = 0;
};

class FAtomSoundClassEditorToolkit
	: public IAtomSoundClassEditorToolkit
	, public FGCObject
	, public FEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(FAtomSoundClassEditorToolkit)
	{
	}
	SLATE_END_ARGS()

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/**
	 * Edits the specified Atom sound class object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The sound class to edit
	 */
	void Initialize(UObject* ObjectToEdit, const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost);

	FAtomSoundClassEditorToolkit();
	virtual ~FAtomSoundClassEditorToolkit();

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("FAtomSoundClassEditorToolkit");
	}

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;

	/** @return Returns the color and opacity to use for the color that appears behind the tab text for this toolkit's tab in world-centric mode. */
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** IAtomSoundClassEditorToolkit interface */
	void CreateSoundClass(class UEdGraphPin* FromPin, const FVector2D& Location, const FString& Name) override;

	/** FEditorUndoClient Interface */
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

private:
	TSharedRef<SDockTab> SpawnTab_GraphCanvas(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Properties(const FSpawnTabArgs& Args);

private:
	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Create new graph editor widget */
	TSharedRef<SGraphEditor> CreateGraphEditorWidget();

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	/** Called to create context menu when right-clicking on graph */
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed);

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

	/** Toggle solo (Soloing means we just hear this) */
	void ToggleSolo();
	bool CanExcuteToggleSolo() const;
	bool IsSoloToggled() const;

	/** Toggle mute (Muting means this will be explicitly silienced)*/
	void ToggleMute();
	bool CanExcuteToggleMute() const;
	bool IsMuteToggled() const;

private:
	/** The AtomSoundClass asset being inspected */
	TObjectPtr<UAtomSoundClass> SoundClass;

	/** Graph Editor */
	TSharedPtr<SGraphEditor> GraphEditor;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;

	/** Command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

#if ENABLE_ATOM_DEBUG
	/** Cache the audio debugger instance */
	Atom::FAtomDebugger* Debugger;
#endif

	/**	The tab ids for all the tabs used */
	static const FName GraphCanvasTabId;
	static const FName PropertiesTabId;

	/** Bind to commands. */
	void BindCommands();

	/** Helper function to grow the toolbar and add custom buttons */
	void ExtendToolbar();
};
