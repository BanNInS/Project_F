/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraphSchema.cpp
 *
 ****************************************************************************/

#include "AtomRackGraphSchema.h"

#include "AssetRegistry/AssetData.h"
#include "GraphEditorActions.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Toolkits/ToolkitManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScopedTransaction.h"

#include "CriWareLocalization.h"
#include "CriWareCoreSettings.h"

#include "AtomDefaults.h"
#include "AtomRackGraphNode.h"
#include "AtomRackGraph.h"
#include "Toolkits/AtomRackEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomRackGraphSchema"

/*
 * FAtomRackGraphConnectionDrawingPolicyFactory struct
 *****************************************************************************/

FConnectionDrawingPolicy* FAtomRackGraphConnectionDrawingPolicyFactory::CreateConnectionPolicy(
	const UEdGraphSchema* Schema,
	int32 InBackLayerID,
	int32 InFrontLayerID,
	float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UAtomRackGraphSchema::StaticClass()))
	{
		return new FAtomRackGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
	return nullptr;
}

/*
 * FAtomRackGraphConnectionDrawingPolicy class
 *****************************************************************************/

FAtomRackGraphConnectionDrawingPolicy::FAtomRackGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
	ActiveWireThickness = Settings->TraceAttackWireThickness;
	InactiveWireThickness = Settings->TraceReleaseWireThickness;
}

// Give specific editor modes a chance to highlight this connection or darken non-interesting connections
void FAtomRackGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& OutParams)
{
	if (!(OutputPin && InputPin && GraphObj))
	{
		return;
	}

	OutParams.AssociatedPin1 = InputPin;
	OutParams.AssociatedPin2 = OutputPin;

	// Get the schema and grab the default color from it
	const UEdGraphSchema* Schema = GraphObj->GetSchema();

	OutParams.WireColor = Schema->GetPinTypeColor(OutputPin->PinType);

	bool bExecuted = false;

	UAtomRackBase* InputRack = OutputPin ? CastChecked<UAtomRackGraphNode>(OutputPin->GetOwningNode())->AtomRack : nullptr;
	UAtomRackBase* OutputRack = InputPin ? CastChecked<UAtomRackGraphNode>(InputPin->GetOwningNode())->AtomRack : nullptr;

	// Run through the predecessors, and on
	if (FExecPairingMap* PredecessorMap = PredecessorNodes.Find(OutputPin->GetOwningNode()))
	{
		if (FTimePair* Times = PredecessorMap->Find(InputPin->GetOwningNode()))
		{
			bExecuted = true;

			OutParams.WireThickness = ActiveWireThickness;
			OutParams.WireColor = Atom::GetColorForRackType(OutputRack);
			OutParams.bDrawBubbles = Atom::IsConnectionPerformingSoundfieldConversion(InputRack, OutputRack);
		}
	}

	if (!bExecuted)
	{
		OutParams.WireColor = Atom::GetColorForRackType(InputRack);
		OutParams.WireThickness = InactiveWireThickness;
	}
}

/*
 * FAtomRackGraphSchemaAction_NewNode class
 *****************************************************************************/

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
UEdGraphNode* FAtomRackGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode)
#else
UEdGraphNode* FAtomRackGraphSchemaAction_NewNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
#endif
{
	check(ParentGraph);

	// Cast outer to AtomRack
	UAtomRackBase* AtomRack = CastChecked<UAtomRackBase>(ParentGraph->GetOuter());

	if (AtomRack)
	{
		TSharedPtr<IAtomRackEditorToolkit> AtomRackEditor;
		TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(AtomRack);
		if (FoundAssetEditor.IsValid())
		{
			AtomRackEditor = StaticCastSharedPtr<IAtomRackEditorToolkit>(FoundAssetEditor);
			AtomRackEditor->CreateAtomRack(FromPin, Location, NewAtomRackName);
		}
	}

	//FAtomRackEditorUtilities::CreateRack(ParentGraph, FromPin, Location, NewAtomRackName);
	return nullptr;
}

/*
 * UAtomRackGraphSchema class
 *****************************************************************************/

UAtomRackGraphSchema::UAtomRackGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

bool UAtomRackGraphSchema::ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	UAtomRackGraphNode* InputNode = CastChecked<UAtomRackGraphNode>(InputPin->GetOwningNode());
	UAtomRackGraphNode* OutputNode = CastChecked<UAtomRackGraphNode>(OutputPin->GetOwningNode());

	// Master Rack cannot be an input as it would create an inferred loop for Racks without an explicit parent
	if (const UCriWareCoreSettings* Settings = GetDefault<UCriWareCoreSettings>())
	{
		if (UAtomRack* MasterRack = Cast<UAtomRack>(Settings->MasterRack.TryLoad()))
		{
			if (OutputNode->AtomRack == MasterRack)
			{
				return true;
			}

			if (OutputNode->AtomRack->RecurseCheckChild(MasterRack))
			{
				return true;
			}
		}
	}

	return OutputNode->AtomRack->RecurseCheckChild(InputNode->AtomRack);
}

void UAtomRackGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const FText Name = LOCTEXT("NewAtomRack", "New Sound Rack");
	const FText ToolTip = LOCTEXT("NewAtomRackTooltip", "Create a new sound Rack");

	TSharedPtr<FAtomRackGraphSchemaAction_NewNode> NewAction(new FAtomRackGraphSchemaAction_NewNode(FText::GetEmpty(), Name, ToolTip, 0));

	ContextMenuBuilder.AddAction(NewAction);
}

void UAtomRackGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context->Node)
	{
		const UAtomRackGraphNode* SoundGraphNode = Cast<const UAtomRackGraphNode>(Context->Node);
		{
			FToolMenuSection& Section = Menu->AddSection("AtomRackGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "AtomRack Actions"));
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
		}
	}

	// No Super call so Node comments option is not shown
}

const FPinConnectionResponse UAtomRackGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both are on the same node"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionIncompatible", "Directions are not compatible"));
	}

	// Note- are input pin and output pin swapped here? Am I losing it?
	UAtomRackBase* InputRack = CastChecked<UAtomRackGraphNode>(OutputPin->GetOwningNode())->AtomRack;
	UAtomRackBase* OutputRack = CastChecked<UAtomRackGraphNode>(InputPin->GetOwningNode())->AtomRack;

	// Check to see if this is an endpoint Rack.
	if (!InputRack->IsA<UAtomRackWithParentBase>())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("RackIsEndpoint", "Rack you are trying to connect from is an endpoint."));
	}

	// If we're trying to make a connection between two soundfield Racks, ensure that we can transcode between the two.
	if (InputRack->IsA<UAtomSoundfieldRack>() && (OutputRack->IsA<UAtomSoundfieldRack>() || OutputRack->IsA<UAtomSoundfieldEndpointRack>()))
	{
		UAtomSoundfieldRack* InputSoundfieldRack = Cast<UAtomSoundfieldRack>(InputRack);
		UAtomSoundfieldRack* OutputSoundfieldRack = Cast<UAtomSoundfieldRack>(OutputRack);

		// atom spatial types
		EAtomSoundfieldRendererType InputType = InputSoundfieldRack->GetRackType();
		EAtomSoundfieldRendererType OutputType = EAtomSoundfieldRendererType::Default;

		if (!OutputSoundfieldRack)
		{
			UAtomSoundfieldEndpointRack* SoundfieldEndpointRackB = CastChecked<UAtomSoundfieldEndpointRack>(OutputRack);
			OutputType = SoundfieldEndpointRackB->SoundfieldRendererType; // internal
		}
		else
		{
			OutputType = InputSoundfieldRack->GetRackType(); // inherit from parent if Default
		}

		// default to "any" only
		if (InputType != EAtomSoundfieldRendererType::Default)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SoundfieldRacksAreIncompatible", "These two Racks have incompatible types."));
		}

		/*
		ISoundfieldFactory* InputFactory = InputSoundfieldRack->GetSoundfieldFactoryForRack();
		ISoundfieldFactory* OutputFactory = nullptr;

		const USoundfieldEncodingSettingsBase* InputEncodingSettings = InputSoundfieldRack->GetEncodingSettings();
		const USoundfieldEncodingSettingsBase* OutputEncodingSettings = nullptr;

		if (!OutputSoundfieldRack)
		{
			UAtomSoundfieldEndpointRack* SoundfieldEndpointRackB = CastChecked<UAtomSoundfieldEndpointRack>(OutputRack);
			OutputFactory = SoundfieldEndpointRackB->GetSoundfieldEndpointForRack();
			OutputEncodingSettings = SoundfieldEndpointRackB->GetEncodingSettings();
		}
		else
		{
			OutputFactory = OutputSoundfieldRack->GetSoundfieldFactoryForRack();
			OutputEncodingSettings = OutputSoundfieldRack->GetEncodingSettings();
		}

		if (InputFactory && OutputFactory)
		{
			if (!InputEncodingSettings)
			{
				InputEncodingSettings = InputFactory->GetDefaultEncodingSettings();
			}

			if (!InputEncodingSettings)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SoundfieldRackSourceIsInvalid", "Rack you are trying to connect from does not specify default settings. Please implement ISoundfieldFactory::GetDefaultEncodingSettings."));
			}

			TUniquePtr<ISoundfieldEncodingSettingsProxy> InputEncodingSettingsProxy = InputEncodingSettings->GetProxy();

			if (!InputEncodingSettingsProxy)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SourceSoundfieldEncodingSettingsAreInvalid", "Rack you are trying to connect from failed to generate a proxy of it's settings. Please check USoundfieldEncodingSettingsBase::GetProxy()."));
			}


			if (!OutputEncodingSettings)
			{
				OutputEncodingSettings = OutputFactory->GetDefaultEncodingSettings();
			}

			if (!OutputEncodingSettings)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SoundfieldRackDestIsInvalid", "Rack you are trying to connect to does not specify default settings. Please implement ISoundfieldFactory::GetDefaultEncodingSettings."));
			}

			TUniquePtr<ISoundfieldEncodingSettingsProxy> OutputEncodingSettingsProxy = OutputEncodingSettings->GetProxy();

			if (!OutputEncodingSettingsProxy)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("DestSoundfieldEncodingSettingsAreInvalid", "Rack you are trying to connect to failed to generate a proxy of it's settings. Please check USoundfieldEncodingSettingsBase::GetProxy()."));
			}

			const bool bAreSoundfieldsCompatible = InputFactory->CanTranscodeToSoundfieldFormat(OutputFactory->GetSoundfieldFormatName(), *OutputEncodingSettingsProxy) || OutputFactory->CanTranscodeFromSoundfieldFormat(InputFactory->GetSoundfieldFormatName(), *InputEncodingSettingsProxy);
			if (!bAreSoundfieldsCompatible)
			{
				return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SoundfieldRacksAreIncompatible", "These two Racks have incompatible types."));
			}
		}*/
	}

	if (ConnectionCausesLoop(InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionLoop", "Connection would cause loop"));
	}

	// Break existing connections on outputs only - multiple input connections are acceptable
	if (OutputPin->LinkedTo.Num() > 0)
	{
		ECanCreateConnectionResponse ReplyBreakInputs;
		if (OutputPin == PinA)
		{
			ReplyBreakInputs = CONNECT_RESPONSE_BREAK_OTHERS_A;
		}
		else
		{
			ReplyBreakInputs = CONNECT_RESPONSE_BREAK_OTHERS_B;
		}
		return FPinConnectionResponse(ReplyBreakInputs, LOCTEXT("ConnectionReplace", "Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UAtomRackGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	check(PinA);
	check(PinB);

	bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		UAtomRackGraph* Graph = CastChecked<UAtomRackGraph>(PinA->GetOwningNode()->GetGraph());
		Graph->LinkAtomRacks();

		UAtomRackBase* RackA = CastChecked<UAtomRackGraphNode>(PinA->GetOwningNode())->AtomRack;
		UAtomRackBase* RackB = CastChecked<UAtomRackGraphNode>(PinB->GetOwningNode())->AtomRack;

		UAtomRackWithParentBase* RackWithParentA = Cast<UAtomRackWithParentBase>(RackA);
		UAtomRackWithParentBase* RackWithParentB = Cast<UAtomRackWithParentBase>(RackA);

		// If re-basing root, re-open editor.  This will force the root to be the primary edited node
		if (Graph->GetRootAtomRack() == RackA && RackWithParentA && RackWithParentA->ParentRack != nullptr)
		{
			Graph->SetRootAtomRack(RackWithParentA->ParentRack);
		}
		else if (Graph->GetRootAtomRack() == RackB && RackWithParentB && RackWithParentB->ParentRack != nullptr)
		{
			Graph->SetRootAtomRack(RackWithParentB->ParentRack);
		}
	}

	return bModified;
}

bool UAtomRackGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UAtomRackGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return Atom::GetColorForRackType(PinType.PinCategory);
}

void UAtomRackGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	CastChecked<UAtomRackGraph>(TargetNode.GetGraph())->LinkAtomRacks();
}

void UAtomRackGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);

	// if this would notify the node then we need to re-link sound classes
	if (bSendsNodeNotifcation)
	{
		if (UAtomRackGraphNode* GraphNode = Cast<UAtomRackGraphNode>(TargetPin.GetOwningNode()))
		{
			// If TargetPin is an input, We should break links to all child Racks of the Rack that owns this pin.
			if (TargetPin.Direction == EEdGraphPinDirection::EGPD_Input)
			{
				// Iterate through all child Racks
				UAtomRackBase* OutputRack = GraphNode->AtomRack;

				// Note: If we ever support multiple parents for Racks, this will need to be modified.
				for (UAtomRackBase* InputRack : OutputRack->ChildRacks)
				{
					if (UAtomRackWithParentBase* RackWithParent = Cast<UAtomRackWithParentBase>(InputRack))
					{
						RackWithParent->ParentRack = nullptr;
						RackWithParent->PostEditChange();
					}
				}

				OutputRack->ChildRacks.Reset();
				OutputRack->PostEditChange();
			}
			else if (TargetPin.Direction == EEdGraphPinDirection::EGPD_Output)
			{
				// If this is an output pin, break the connection between this Rack and it's parent.
				UAtomRackWithParentBase* InputRack = CastChecked<UAtomRackWithParentBase>(GraphNode->AtomRack);
				UAtomRackBase* OutputRack = InputRack->ParentRack;
				check(OutputRack);

				OutputRack->ChildRacks.Remove(InputRack);
				InputRack->ParentRack = nullptr;

				OutputRack->PostEditChange();
				InputRack->PostEditChange();
			}
		}

		CastChecked<UAtomRackGraph>(TargetPin.GetOwningNode()->GetGraph())->LinkAtomRacks();
	}
}

void UAtomRackGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));
	Super::BreakSinglePinLink(SourcePin, TargetPin);

	// Compare the directions
	UEdGraphPin* InputPin = nullptr;
	UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(SourcePin, TargetPin, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return;
	}

	// Note- are input pin and output pin swapped here? Am I losing it?
	UAtomRackBase* InputRack = CastChecked<UAtomRackGraphNode>(OutputPin->GetOwningNode())->AtomRack;
	UAtomRackBase* OutputRack = CastChecked<UAtomRackGraphNode>(InputPin->GetOwningNode())->AtomRack;

	if (UAtomRackWithParentBase* RackWithParent = CastChecked<UAtomRackWithParentBase>(InputRack))
	{
		RackWithParent->ParentRack = nullptr;
		RackWithParent->PostEditChange();
	}

	CastChecked<UAtomRackGraph>(SourcePin->GetOwningNode()->GetGraph())->LinkAtomRacks();
}

void UAtomRackGraphSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	check(GEditor);
	check(Graph);

	UAtomRackGraph* AtomRackGraph = CastChecked<UAtomRackGraph>(Graph);
	TSet<IAssetEditorInstance*> Editors;
	TSet<UAtomRackBase*> UndisplayedRacks;
	for (const FAssetData& Asset : Assets)
	{
		if (UAtomRackBase* AtomRack = Cast<UAtomRackBase>(Asset.GetAsset()))
		{
			// Walk to the root Rack
			UAtomRackWithParentBase* RackWithParent = Cast<UAtomRackWithParentBase>(AtomRack);
			while (RackWithParent && RackWithParent->ParentRack != nullptr)
			{
				AtomRack = RackWithParent->ParentRack;
				RackWithParent = Cast<UAtomRackWithParentBase>(AtomRack);
			}

			if (!AtomRackGraph->IsRackDisplayed(AtomRack))
			{
				UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				TArray<IAssetEditorInstance*> RackEditors = EditorSubsystem->FindEditorsForAsset(AtomRack);
				for (IAssetEditorInstance* Editor : RackEditors)
				{
					if (Editor)
					{
						Editors.Add(Editor);
					}
				}
				UndisplayedRacks.Add(AtomRack);
			}
		}
	}

	if (UndisplayedRacks.Num() > 0)
	{
		const FScopedTransaction Transaction(LOCTEXT("AtomRackEditorDropRacks", "Sound Rack Editor: Drag and Drop Sound Rack"));

		for (IAssetEditorInstance* Editor : Editors)
		{
			check(Editor);
			FAtomRackEditorToolkit* AtomRackEditor = static_cast<FAtomRackEditorToolkit*>(Editor);

			// Close editors with dropped (and undisplayed) Rack branches as they are now displayed locally in this graph
			// (to avoid modification of multiple graph editors representing the same branch of Racks)
			if (AtomRackEditor->GetGraph() != Graph)
			{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				Editor->CloseWindow(EAssetEditorCloseReason::AssetUnloadingOrInvalid);
#else
				Editor->CloseWindow();
#endif
			}
		}

		// If editor is this graph's editor, update editable objects and select dropped Racks.
		if (UAtomRackBase* RootRack = AtomRackGraph->GetRootAtomRack())
		{
			UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
			if (IAssetEditorInstance* EditorInstance = EditorSubsystem->FindEditorForAsset(RootRack, false /* bFocusIfOpen */))
			{
				FAtomRackEditorToolkit* AtomRackEditor = static_cast<FAtomRackEditorToolkit*>(EditorInstance);
				AtomRackGraph->AddDroppedAtomRacks(UndisplayedRacks, GraphPosition.X, GraphPosition.Y);
				AtomRackEditor->AddMissingEditableRacks();
				AtomRackEditor->SelectRacks(UndisplayedRacks);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
