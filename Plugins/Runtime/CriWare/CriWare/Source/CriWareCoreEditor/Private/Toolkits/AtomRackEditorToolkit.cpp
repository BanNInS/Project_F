/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackEditorToolkit.cpp
 *
 ****************************************************************************/

#include "AtomRackEditorToolkit.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "GraphEditor.h"
#include "GraphEditAction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Docking/TabManager.h"
#include "IAssetTools.h"
#include "ScopedTransaction.h"
#include "Toolkits/IToolkitHost.h"
#include "UObject/Package.h"
#include "Widgets/Docking/SDockTab.h"

#include "Atom/AtomRack.h"

#include "CriWareCoreEditorPrivate.h"
#include "Factories/AtomRackFactory.h"
#include "Graphs/AtomRackGraph.h"
#include "Graphs/AtomRackGraphNode.h"
#include "Graphs/AtomRackGraphSchema.h"
#include "Widgets/SAtomRackActionMenu.h"

#define LOCTEXT_NAMESPACE "AtomRackEditorToolkit"

DEFINE_LOG_CATEGORY_STATIC(LogAtomRackEditor, Log, All);

namespace AtomRackEditorToolkit
{
	static const FName AppIdentifier("AtomRackEditorApp");
	static const FName DetailsTabId("Details");
	static const FName GraphCanvasTabId("GraphCanvas");
	static const FName EffectEditorTabId("EffectEditor");
}

class SAtomRackGraphEditor
	: public SGraphEditor
{
private:

	TWeakPtr<FAtomRackEditorToolkit> RackEditor;

public:

	SAtomRackGraphEditor()
		: SGraphEditor()
		, RackEditor(nullptr)
	{}

	void Construct(const FArguments& InArgs, TSharedPtr<FAtomRackEditorToolkit> InEditor)
	{
		RackEditor = InEditor;
		SGraphEditor::Construct(InArgs);
	}

	void OnGraphChanged(const FEdGraphEditAction& InAction) override
	{
		if (RackEditor.IsValid())
		{
			TSharedPtr<FAtomRackEditorToolkit> PinnedEditor = RackEditor.Pin();
			if (InAction.Graph && InAction.Graph == PinnedEditor->GetGraph())
			{
				PinnedEditor->AddMissingEditableRacks();
			}
		}
		SGraphEditor::OnGraphChanged(InAction);
	}
};

FAtomRackEditorToolkit::~FAtomRackEditorToolkit()
{
	GEditor->UnregisterForUndo(this);
	DetailsView.Reset();
}

void FAtomRackEditorToolkit::Initialize(UObject* ObjectToEdit, const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost >& InitToolkitHost)
{
	UAtomRackBase* AtomRack = CastChecked<UAtomRackBase>(ObjectToEdit);

	UAtomRackWithParentBase* RackWithParent = Cast<UAtomRackWithParentBase>(AtomRack);
	while (RackWithParent && RackWithParent->ParentRack)
	{
		AtomRack = RackWithParent->ParentRack;
		RackWithParent = Cast<UAtomRackWithParentBase>(AtomRack);
	}

	GEditor->RegisterForUndo(this);

	ToolkitCommands->MapAction
	(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::UndoGraphAction)
	);

	ToolkitCommands->MapAction
	(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::RedoGraphAction)
	);

	UAtomRackGraph* AtomRackGraph = CastChecked<UAtomRackGraph>(FBlueprintEditorUtils::CreateNewGraph(AtomRack, NAME_None, UAtomRackGraph::StaticClass(), UAtomRackGraphSchema::StaticClass()));
	AtomRackGraph->SetRootAtomRack(AtomRack);

	AtomRack->AtomRackGraph = AtomRackGraph;
	AtomRackGraph->RebuildGraph();

	CreateInternalWidgets(AtomRack);

	TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_AtomRackEditor")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			/*->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split*/
			->SetOrientation(Orient_Horizontal)
			//(
			//	FTabManager::NewSplitter()
				->Split
				(
					FTabManager::NewStack()
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.2f)
					->AddTab(AtomRackEditorToolkit::DetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.8f)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->SetSizeCoefficient(0.2f)
						->AddTab(AtomRackEditorToolkit::EffectEditorTabId, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetHideTabWell(true)
						->SetSizeCoefficient(0.8f)
						->AddTab(AtomRackEditorToolkit::GraphCanvasTabId, ETabState::OpenedTab)
					)
				)
			//)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, AtomRackEditorToolkit::AppIdentifier, Layout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, AtomRack);

	// Must be called after super class initialization
	AddEditableRackChildren(AtomRack);

	//IAudioEditorModule* AudioEditorModule = &FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	//AddMenuExtender(AudioEditorModule->GetSoundSubmixMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	//AddToolbarExtender(AudioEditorModule->GetSoundSubmixToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	GraphEditor->SelectAllNodes();
	for (UObject* SelectedNode : GraphEditor->GetSelectedNodes())
	{
		UAtomRackGraphNode* GraphNode = CastChecked<UAtomRackGraphNode>(SelectedNode);
		if (GraphNode->AtomRack == ObjectToEdit)
		{
			GraphEditor->ClearSelectionSet();
			GraphEditor->SetNodeSelection(GraphNode, true);
			DetailsView->SetObject(ObjectToEdit);
			break;
		}
	}
}

void FAtomRackEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SoundSubmixEditor", "Sound Submix Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(AtomRackEditorToolkit::GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FAtomRackEditorToolkit::SpawnTab_GraphCanvas))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Graph"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(AtomRackEditorToolkit::DetailsTabId, FOnSpawnTab::CreateSP(this, &FAtomRackEditorToolkit::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("PropertiesTab", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(AtomRackEditorToolkit::EffectEditorTabId, FOnSpawnTab::CreateSP(this, &FAtomRackEditorToolkit::SpawnTab_EffectEditor))
		.SetDisplayName(LOCTEXT("EffectEditorTab", "Effect Graph"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GraphEditor.EventGraph_16x"));
}

void FAtomRackEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(AtomRackEditorToolkit::GraphCanvasTabId);
	InTabManager->UnregisterTabSpawner(AtomRackEditorToolkit::DetailsTabId);
	InTabManager->UnregisterTabSpawner(AtomRackEditorToolkit::EffectEditorTabId);
}

FText FAtomRackEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Atom Rack Editor");
}

FName FAtomRackEditorToolkit::GetToolkitFName() const
{
	return FName("AtomRackEditor");
}

FText FAtomRackEditorToolkit::GetToolkitName() const
{
	UObject* EditObject = GetEditingObjects()[0];
	return GetLabelForObject(EditObject);
}

FText FAtomRackEditorToolkit::GetToolkitToolTipText() const
{
	return GetToolTipTextForObject(GetEditingObjects()[0]);
}

FLinearColor FAtomRackEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.2f, 0.4f, 0.8f, 0.5f);
}

FString FAtomRackEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Atom Rack ").ToString();
}

void FAtomRackEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
    Collector.AddReferencedObjects(GetEditingObjectPtrs());
#else
	for (UObject* Obj : GetEditingObjects())
	{
		Collector.AddReferencedObject(Obj);
	}
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
void FAtomRackEditorToolkit::CreateAtomRack(UEdGraphPin* FromPin, const FVector2f Location, const FString& Name)
#else
void FAtomRackEditorToolkit::CreateAtomRack(UEdGraphPin* FromPin, const FVector2D Location, const FString& Name)
#endif
{
	if (Name.IsEmpty())
	{
		return;
	}

	for (UObject* Obj : GetEditingObjects())
	{
		if (Obj->GetName() == Name)
			return;
	}

	// Derive new package path from existing asset's path
	UAtomRackBase* AtomRack = CastChecked<UAtomRackBase>(GetEditingObjects()[0]);
	FString PackagePath = AtomRack->GetPathName();
	FString AssetName = FString::Printf(TEXT("/%s.%s"), *AtomRack->GetName(), *AtomRack->GetName());
	PackagePath.RemoveFromEnd(AssetName);

	// Create an Atom rack factory to create a new sound class
	UAtomRackFactory* AtomRackFactory = NewObject<UAtomRackFactory>();

	// Load asset tools to create the asset properly
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UAtomRack* NewAtomRack = Cast<UAtomRack>(AssetToolsModule.Get().CreateAsset(Name, PackagePath, UAtomRack::StaticClass(), AtomRackFactory, FName("AtomRackEditorNewAsset")));

	if (NewAtomRack)
	{
		CastChecked<UAtomRackGraph>(AtomRack->AtomRackGraph)->AddNewAtomRack(FromPin, NewAtomRack, Location.X, Location.Y);

		AddEditingObject(NewAtomRack);
		NewAtomRack->PostEditChange();
		NewAtomRack->MarkPackageDirty();
	}
}

void FAtomRackEditorToolkit::AddMissingEditableRacks()
{
	if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
	{
		bool bChanged = false;
		if (Graph->Nodes.Num() > GetEditingObjects().Num())
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				UAtomRackGraphNode* GraphNode = CastChecked<UAtomRackGraphNode>(Node);
				UAtomRackBase* UntrackedRack = GraphNode->AtomRack;
				if (UntrackedRack && !GetEditingObjects().Contains(UntrackedRack))
				{
					bChanged = true;
					AddEditingObject(UntrackedRack);
				}
			}
		}

		if (bChanged)
		{
			GraphEditor->NotifyGraphChanged();
		}
	}
}

void FAtomRackEditorToolkit::SelectRacks(TSet<UAtomRackBase*>& InRacks)
{
	TArray<UObject*> ObjectsToSelect;

	GraphEditor->SelectAllNodes();
	const TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();
	GraphEditor->ClearSelectionSet();

	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode)
		{
			UAtomRackGraphNode* GraphNode = CastChecked<UAtomRackGraphNode>(SelectedNode);
			if (UAtomRackBase* Rack = GraphNode->AtomRack)
			{
				if (InRacks.Contains(Rack))
				{
					ObjectsToSelect.Add(Rack);
					GraphEditor->SetNodeSelection(GraphNode, true /* bSelect */);
				}
			}
		}
	}

	DetailsView->SetObjects(ObjectsToSelect);
}

UEdGraph* FAtomRackEditorToolkit::GetGraph()
{
	return GraphEditor->GetCurrentGraph();
}

void FAtomRackEditorToolkit::PostUndo(bool bSuccess)
{
	GraphEditor->ClearSelectionSet();
	GraphEditor->NotifyGraphChanged();
}

void FAtomRackEditorToolkit::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

TSharedRef<SDockTab> FAtomRackEditorToolkit::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == AtomRackEditorToolkit::GraphCanvasTabId);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("GraphTabName", "Graph"))
		[
			GraphEditor.ToSharedRef()
		];

	return SpawnedTab;
}

TSharedRef<SDockTab> FAtomRackEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == AtomRackEditorToolkit::DetailsTabId);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabName", "Details"))
		[
			DetailsView.ToSharedRef()
		];

	return SpawnedTab;
}

TSharedRef<SDockTab> FAtomRackEditorToolkit::SpawnTab_EffectEditor(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == AtomRackEditorToolkit::EffectEditorTabId);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("EffectEditorTabName", "Effect Editor"))
		[
			EffectEditor.ToSharedRef()
		];

	return SpawnedTab;
}

void FAtomRackEditorToolkit::CreateInternalWidgets(UAtomRackBase* InAtomRack)
{
	GraphEditor = CreateGraphEditorWidget(InAtomRack);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(InAtomRack);

	EffectEditor = CreateEffectEditorWidget(InAtomRack);
}

TSharedRef<SAtomEffectEditor> FAtomRackEditorToolkit::CreateEffectEditorWidget(UAtomRackBase* InAtomRack)
{
	// map actions UI command
	/*if (!EffectEditorCommands.IsValid())
	{
		EffectEditorCommands = MakeShareable(new FUICommandList);
	}*/

	FAtomEffectEditorAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_AtomEffect", "ATOM EFFECT");

	//SAtomEffectEditor::FAtomEffectEditorEvents InEvents;
	//InEvents.OnSelectionChanged = SAtomEffectEditor::FOnSelectionChanged::CreateSP(this, &FAtomRackEditorToolkit::OnSelectedEffectChanged);
	//InEvents.OnCreateActionMenu = SAtomEffectEditor::FOnCreateActionMenu::CreateSP(this, &FAtomRackEditorToolkit::OnCreateGraphActionMenu);

	return SNew(SAtomEffectEditor)//, SharedThis(this))
		//.AdditionalCommands(EffectEditorCommands) // In
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.RackToEdit(InAtomRack)
		//.EffectEvents(InEvents) // out
		.ShowGraphStateOverlay(false);
}

TSharedRef<SGraphEditor> FAtomRackEditorToolkit::CreateGraphEditorWidget(UAtomRackBase* InAtomRack)
{
	if (!GraphEditorCommands.IsValid())
	{
		GraphEditorCommands = MakeShareable(new FUICommandList);

		// Editing commands
		GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::SelectAllNodes),
			FCanExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::CanSelectAllNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::RemoveSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FAtomRackEditorToolkit::CanRemoveNodes)
		);
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_AtomRack", "ATOM RACK");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAtomRackEditorToolkit::OnSelectedNodesChanged);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	InEvents.OnCreateActionMenuAtLocation = SGraphEditor::FOnCreateActionMenuAtLocation::CreateSP(this, &FAtomRackEditorToolkit::OnCreateGraphActionMenu); 
#else
	InEvents.OnCreateActionMenu = SGraphEditor::FOnCreateActionMenu::CreateSP(this, &FAtomRackEditorToolkit::OnCreateGraphActionMenu);
#endif

	return SNew(SAtomRackGraphEditor, SharedThis(this))
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(InAtomRack->AtomRackGraph)
		.GraphEvents(InEvents)
		.ShowGraphStateOverlay(false);
}

void FAtomRackEditorToolkit::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	if (NewSelection.Num() > 0)
	{
		TArray<UObject*> Selection;
		for (UObject* Obj : NewSelection)
		{
			UAtomRackGraphNode* GraphNode = CastChecked<UAtomRackGraphNode>(Obj);
			Selection.Add(GraphNode->AtomRack);
		}
		DetailsView->SetObjects(Selection);
	}
	else
	{
		DetailsView->SetObject(GetEditingObjects()[0]);
	}
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
FActionMenuContent FAtomRackEditorToolkit::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2f& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
#else
FActionMenuContent FAtomRackEditorToolkit::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
#endif
{
	TSharedRef<SAtomRackActionMenu> ActionMenu =
		SNew(SAtomRackActionMenu)
		.GraphObj(InGraph)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		.NewNodePosition(InNodePosition)
#else
		.NewNodePosition(FVector2f(InNodePosition))
#endif
		.DraggedFromPins(InDraggedPins)
		.AutoExpandActionMenu(bAutoExpand)
		.OnClosedCallback(InOnMenuClosed);

	return FActionMenuContent(ActionMenu, ActionMenu);
}

void FAtomRackEditorToolkit::AddEditableRackChildren(UAtomRackBase* RootRack)
{
	if (!RootRack)
	{
		return;
	}

	RootRack->SetFlags(RF_Transactional);
	for (UAtomRackBase* Child : RootRack->ChildRacks)
	{
		if (Child)
		{
			Child->AtomRackGraph = RootRack->AtomRackGraph;
			AddEditingObject(Child);
			AddEditableRackChildren(Child);
		}
	}
}

void FAtomRackEditorToolkit::SelectAllNodes()
{
	GraphEditor->SelectAllNodes();
}

bool FAtomRackEditorToolkit::CanSelectAllNodes() const
{
	return true;
}

void FAtomRackEditorToolkit::RemoveSelectedNodes()
{
	const FScopedTransaction Transaction(LOCTEXT("AtomRackEditorRemoveSelectedNode", "Atom Rack Editor: Remove Selected Atom racks from editor"));

	int32 NumObjectsRemoved = 0;

	const TSet<UObject*> SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		UAtomRackGraphNode* Node = Cast<UAtomRackGraphNode>(SelectedNode);
		if (Node && Node->AtomRack && Node->CanUserDeleteNode())
		{
			NumObjectsRemoved++;
			RemoveEditingObject(Node->AtomRack);
		}
	}

	if (NumObjectsRemoved > 0)
	{
		UAtomRackGraph* Graph = CastChecked<UAtomRackGraph>(GraphEditor->GetCurrentGraph());
		Graph->RecursivelyRemoveNodes(SelectedNodes);
		GraphEditor->ClearSelectionSet();
	}
}

bool FAtomRackEditorToolkit::CanRemoveNodes() const
{
	return GraphEditor->GetSelectedNodes().Num() > 0;
}

void FAtomRackEditorToolkit::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FAtomRackEditorToolkit::RedoGraphAction()
{
	// Clear selection, to avoid holding refs to nodes that go away
	GraphEditor->ClearSelectionSet();

	GEditor->RedoTransaction();
}

#undef LOCTEXT_NAMESPACE
