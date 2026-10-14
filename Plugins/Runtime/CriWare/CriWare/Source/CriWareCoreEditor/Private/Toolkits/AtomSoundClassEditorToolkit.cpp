
#include "AtomSoundClassEditorToolkit.h"

#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "EdGraph/EdGraph.h"
#include "Editor.h"
#include "EngineAnalytics.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WorkspaceMenuStructureModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Atom/AtomDebug.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomSoundClass.h"

#include "CriWareCoreEditorModule.h"
#include "Factories/AtomSoundClassFactory.h"
#include "Graphs/AtomSoundClassGraph.h"
#include "Graphs/AtomSoundClassGraphNode.h"
#include "Graphs/AtomSoundClassGraphSchema.h"
#include "Widgets/SAtomSoundClassActionMenu.h"
#include "Models/AtomSoundClassEditorCommands.h"

#define LOCTEXT_NAMESPACE "AtomSoundClassEditorToolkit"
DEFINE_LOG_CATEGORY_STATIC( LogAtomSoundClassEditor, Log, All );

const FName FAtomSoundClassEditorToolkit::GraphCanvasTabId( TEXT( "AtomSoundClassEditorToolkit_GraphCanvas" ) );
const FName FAtomSoundClassEditorToolkit::PropertiesTabId( TEXT( "AtomSoundClassEditorToolkit_Properties" ) );


void FAtomSoundClassEditorToolkit::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("Audition");
			{
				ToolbarBuilder.AddToolBarButton(FAtomSoundClassEditorCommands::Get().ToggleMute);
				ToolbarBuilder.AddToolBarButton(FAtomSoundClassEditorCommands::Get().ToggleSolo);
			}
			ToolbarBuilder.EndSection();
		})
	);

	AddToolbarExtender(ToolbarExtender);

	//IAudioEditorModule* AudioEditorModule = &FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
	//AddToolbarExtender(AudioEditorModule->GetSoundClassToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
	
	//ICriWareCoreEditorModule* CoreEditorModule = &FModuleManager::LoadModuleChecked<ICriWareCoreEditorModule>("CriWareCoreEditor");
	//AddToolbarExtender(CoreEditorModule->GetAtomSoundClassToolBarExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));
}

//////////////////////////////////////////////////////////////////////////
// FAtomSoundClassEditorToolkit

void FAtomSoundClassEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_AtomSoundClassEditorToolkit", "Atom Sound Class Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner( GraphCanvasTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundClassEditorToolkit::SpawnTab_GraphCanvas) )
		.SetDisplayName( LOCTEXT( "GraphCanvasTab", "Graph" ) )
		.SetGroup( WorkspaceMenuCategoryRef )
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner( PropertiesTabId, FOnSpawnTab::CreateSP(this, &FAtomSoundClassEditorToolkit::SpawnTab_Properties) )
		.SetDisplayName( LOCTEXT( "PropertiesTab", "Details" ) )
		.SetGroup( WorkspaceMenuCategoryRef )
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FAtomSoundClassEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner( GraphCanvasTabId );
	InTabManager->UnregisterTabSpawner( PropertiesTabId );
}

void FAtomSoundClassEditorToolkit::Initialize(UObject* ObjectToEdit, const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
	SoundClass = CastChecked<UAtomSoundClass>(ObjectToEdit);

	while (SoundClass->ParentClass)
	{
		SoundClass = SoundClass->ParentClass;
	}

	// Support undo/redo
	SoundClass->SetFlags(RF_Transactional);

	GEditor->RegisterForUndo(this);
	
	FAtomSoundClassEditorCommands::Register();
	BindCommands();
	
	if( !SoundClass->SoundClassGraph )
	{
		UAtomSoundClassGraph* SoundClassGraph = CastChecked<UAtomSoundClassGraph>(FBlueprintEditorUtils::CreateNewGraph(SoundClass, NAME_None, UAtomSoundClassGraph::StaticClass(), UAtomSoundClassGraphSchema::StaticClass()));
		SoundClassGraph->SetRootSoundClass(SoundClass);

		SoundClass->SoundClassGraph = SoundClassGraph;
	}

	CastChecked<UAtomSoundClassGraph>(SoundClass->SoundClassGraph)->RebuildGraph();

	CreateInternalWidgets();

	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout( "Standalone_AtomSoundClassEditorToolkit_Layout_v3" )
	->AddArea
	(
		FTabManager::NewPrimaryArea() ->SetOrientation(Orient_Vertical)
		->Split
		(
			FTabManager::NewSplitter()
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.2f)
				->AddTab(PropertiesTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.8f)
				->AddTab(GraphCanvasTabId, ETabState::OpenedTab)
			)
		)
	);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor( Mode, InitToolkitHost, TEXT("AtomSoundClassEditorToolkitApp"), StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, SoundClass);

	//IAudioEditorModule* AudioEditorModule = &FModuleManager::LoadModuleChecked<IAudioEditorModule>( "AudioEditor" );
	//AddMenuExtender(AudioEditorModule->GetSoundClassMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

	ExtendToolbar();
	RegenerateMenusAndToolbars();
			
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
	{
		Debugger = &RuntimeManager->GetDebugger();
	}	
#endif

	GraphEditor->SelectAllNodes();
	for (UObject* SelectedNode : GraphEditor->GetSelectedNodes())
	{
		UAtomSoundClassGraphNode* GraphNode = CastChecked<UAtomSoundClassGraphNode>(SelectedNode);
		if (GraphNode->SoundClass == ObjectToEdit)
		{
			GraphEditor->ClearSelectionSet();
			GraphEditor->SetNodeSelection(GraphNode, true);
			DetailsView->SetObject(ObjectToEdit);
			break;
		}
	}
}

void FAtomSoundClassEditorToolkit::BindCommands()
{
	ToolkitCommands->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::UndoGraphAction));

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::RedoGraphAction));

	const FAtomSoundClassEditorCommands& Commands = FAtomSoundClassEditorCommands::Get();
	
	ToolkitCommands->MapAction(
		Commands.ToggleSolo,
		FExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::ToggleSolo),
		FCanExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::CanExcuteToggleSolo),
		FIsActionChecked::CreateSP(this, &FAtomSoundClassEditorToolkit::IsSoloToggled));

	ToolkitCommands->MapAction(
		Commands.ToggleMute,
		FExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::ToggleMute),
		FCanExecuteAction::CreateSP(this, &FAtomSoundClassEditorToolkit::CanExcuteToggleMute),
		FIsActionChecked::CreateSP(this, &FAtomSoundClassEditorToolkit::IsMuteToggled));
}

FAtomSoundClassEditorToolkit::FAtomSoundClassEditorToolkit()
	: SoundClass(nullptr)
#if ENABLE_ATOM_DEBUG
	, Debugger(nullptr)
#endif
{
}

FAtomSoundClassEditorToolkit::~FAtomSoundClassEditorToolkit()
{
	GEditor->UnregisterForUndo( this );
	DetailsView.Reset();
}

void FAtomSoundClassEditorToolkit::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject(SoundClass);
}

TSharedRef<SDockTab> FAtomSoundClassEditorToolkit::SpawnTab_GraphCanvas(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == GraphCanvasTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("GraphCanvasTitle", "Graph"))
		[
			GraphEditor.ToSharedRef()
		];

	return SpawnedTab;
}

TSharedRef<SDockTab> FAtomSoundClassEditorToolkit::SpawnTab_Properties(const FSpawnTabArgs& Args)
{
	check( Args.GetTabId() == PropertiesTabId );

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("AtomSoundClassPropertiesTitle", "Details" ) )
		[
			DetailsView.ToSharedRef()
		];
	
	return SpawnedTab;
}

FName FAtomSoundClassEditorToolkit::GetToolkitFName() const
{
	return FName("AtomSoundClassEditorToolkit");
}

FText FAtomSoundClassEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Atom Sound Class Editor");
}

FString FAtomSoundClassEditorToolkit::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Atom Sound Class ").ToString();
}

FLinearColor FAtomSoundClassEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor( 0.3f, 0.2f, 0.5f, 0.5f );
}

void FAtomSoundClassEditorToolkit::CreateInternalWidgets()
{
	GraphEditor = CreateGraphEditorWidget();

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );
	DetailsView->SetObject( SoundClass );
}

TSharedRef<SGraphEditor> FAtomSoundClassEditorToolkit::CreateGraphEditorWidget()
{
	if ( !GraphEditorCommands.IsValid() )
	{
		GraphEditorCommands = MakeShareable( new FUICommandList );

		// Editing commands
		GraphEditorCommands->MapAction( FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP( this, &FAtomSoundClassEditorToolkit::SelectAllNodes ),
			FCanExecuteAction::CreateSP( this, &FAtomSoundClassEditorToolkit::CanSelectAllNodes )
			);

		GraphEditorCommands->MapAction( FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP( this, &FAtomSoundClassEditorToolkit::RemoveSelectedNodes ),
			FCanExecuteAction::CreateSP( this, &FAtomSoundClassEditorToolkit::CanRemoveNodes )
			);
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_AtomSoundClass", "ATOM SOUND CLASS");

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAtomSoundClassEditorToolkit::OnSelectedNodesChanged);
	InEvents.OnCreateActionMenu = SGraphEditor::FOnCreateActionMenu::CreateSP(this, &FAtomSoundClassEditorToolkit::OnCreateGraphActionMenu);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(SoundClass->SoundClassGraph)
		.GraphEvents(InEvents)
		.ShowGraphStateOverlay(false);
}

void FAtomSoundClassEditorToolkit::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	if(NewSelection.Num())
	{
		for(TSet<class UObject*>::TConstIterator SetIt(NewSelection);SetIt;++SetIt)
		{
			UAtomSoundClassGraphNode* GraphNode = CastChecked<UAtomSoundClassGraphNode>(*SetIt);
			Selection.Add(GraphNode->SoundClass);
		}
		DetailsView->SetObjects(Selection);
	}
	else
	{
		DetailsView->SetObject(SoundClass);
	}
}

FActionMenuContent FAtomSoundClassEditorToolkit::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed)
{
	TSharedRef<SAtomSoundClassActionMenu> ActionMenu = 
		SNew(SAtomSoundClassActionMenu)
		.GraphObj(InGraph)
		.NewNodePosition(InNodePosition)
		.DraggedFromPins(InDraggedPins)
		.AutoExpandActionMenu(bAutoExpand)
		.OnClosedCallback(InOnMenuClosed);

	return FActionMenuContent( ActionMenu, ActionMenu );
}

void FAtomSoundClassEditorToolkit::SelectAllNodes()
{
	GraphEditor->SelectAllNodes();
}

bool FAtomSoundClassEditorToolkit::CanSelectAllNodes() const
{
	return true;
}

void FAtomSoundClassEditorToolkit::RemoveSelectedNodes()
{
	const FScopedTransaction Transaction( LOCTEXT("AtomSoundClassEditorToolkitRemoveSelectedNode", "Atom Sound Class Editor: Remove Selected AtomSoundClasses from editor") );

	CastChecked<UAtomSoundClassGraph>(SoundClass->SoundClassGraph)->RecursivelyRemoveNodes(GraphEditor->GetSelectedNodes());

	GraphEditor->ClearSelectionSet();
}

bool FAtomSoundClassEditorToolkit::CanRemoveNodes() const
{
	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UAtomSoundClassGraphNode* Node = Cast<UAtomSoundClassGraphNode>(*NodeIt);

		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FAtomSoundClassEditorToolkit::UndoGraphAction()
{
	GEditor->UndoTransaction();
}

void FAtomSoundClassEditorToolkit::RedoGraphAction()
{
	// Clear selection, to avoid holding refs to nodes that go away
	GraphEditor->ClearSelectionSet();

	GEditor->RedoTransaction();
}

void FAtomSoundClassEditorToolkit::ToggleSolo()
{
#if ENABLE_AUDIO_DEBUG
	if (Debugger)
	{
		Debugger->ToggleSoloSoundClass(SoundClass->GetFName());
	}
#endif
}

bool FAtomSoundClassEditorToolkit::CanExcuteToggleSolo() const
{
#if ENABLE_AUDIO_DEBUG
	// Enable solo if we are in PIE and not Muted 
	if (Debugger)
	{
		bool bIsInPIE = (GEditor != nullptr && GEditor->PlayWorld != nullptr) || GIsPlayInEditorWorld;
		if (bIsInPIE) 
		{
			return !Debugger->IsMuteSoundClass(SoundClass->GetFName());
		}
	}
#endif
	return false;
}

bool FAtomSoundClassEditorToolkit::IsSoloToggled() const
{
#if ENABLE_AUDIO_DEBUG
	if (Debugger)
	{
		return Debugger->IsSoloSoundClass(SoundClass->GetFName());
	}
#endif	
	return false;
}

void FAtomSoundClassEditorToolkit::ToggleMute()
{
#if ENABLE_AUDIO_DEBUG
	if (Debugger)
	{
		Debugger->ToggleMuteSoundClass(SoundClass->GetFName());
	}
#endif
}

bool FAtomSoundClassEditorToolkit::CanExcuteToggleMute() const
{
#if ENABLE_AUDIO_DEBUG
	// Enable mute if we are in PIE and not Soloed
	if (Debugger)
	{
		bool bIsInPIE = (GEditor != nullptr && GEditor->PlayWorld != nullptr) || GIsPlayInEditorWorld;
		if (bIsInPIE)
		{
			return !Debugger->IsSoloSoundClass(SoundClass->GetFName());
		}
	}
#endif
	return false;
}

bool FAtomSoundClassEditorToolkit::IsMuteToggled() const
{	
#if ENABLE_AUDIO_DEBUG
	if (Debugger)
	{
		return Debugger->IsMuteSoundClass(SoundClass->GetFName());
	}
#endif
	return false;
}

void FAtomSoundClassEditorToolkit::CreateSoundClass(UEdGraphPin* FromPin, const FVector2D& Location, const FString& Name)
{
	// If we have a valid name
	if (!Name.IsEmpty() && Name != SoundClass->GetName())
	{
		// Derive new package path from existing asset's path
		FString PackagePath = SoundClass->GetPathName();
		FString AssetName = FString::Printf(TEXT("/%s.%s"), *SoundClass->GetName(), *SoundClass->GetName());
		PackagePath.RemoveFromEnd(AssetName);

		// Create a sound class factory to create a new sound class
		UAtomSoundClassFactory* SoundClassFactory = NewObject<UAtomSoundClassFactory>();

		// Load asset tools to create the asset properly
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		UAtomSoundClass* NewSoundClass = Cast<UAtomSoundClass>(AssetToolsModule.Get().CreateAsset(Name, PackagePath, UAtomSoundClass::StaticClass(), SoundClassFactory, FName("AtomSoundClassEditorToolkitNewAsset")));

		// This may fail if the asset has same name as existing asset, etc.
		if (NewSoundClass)
		{
			CastChecked<UAtomSoundClassGraph>(SoundClass->SoundClassGraph)->AddNewSoundClass(FromPin, NewSoundClass, Location.X, Location.Y);

			NewSoundClass->PostEditChange();
			NewSoundClass->MarkPackageDirty();
		}
	}
}

void FAtomSoundClassEditorToolkit::PostUndo(bool bSuccess)
{
	GraphEditor->ClearSelectionSet();
	GraphEditor->NotifyGraphChanged();

	FSlateApplication::Get().DismissAllMenus();
}

#undef LOCTEXT_NAMESPACE
