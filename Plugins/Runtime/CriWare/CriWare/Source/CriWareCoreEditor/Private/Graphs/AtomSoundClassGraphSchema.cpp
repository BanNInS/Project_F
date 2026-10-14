
#include "AtomSoundClassGraphSchema.h"

#include "UObject/ObjectPtr.h"
#include "AssetRegistry/AssetData.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformCrt.h"
#include "Internationalization/Internationalization.h"
#include "ScopedTransaction.h"
#include "Templates/Casts.h"
#include "Templates/SharedPointer.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "Toolkits/ToolkitManager.h"

#include "Atom/AtomSoundClass.h"

#include "AtomSoundClassGraph.h"
#include "AtomSoundClassGraphNode.h"
#include "Toolkits/AtomSoundClassEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomSoundClassSchema"

UEdGraphNode* FAtomSoundClassGraphSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	check(ParentGraph);

	// Cast outer to AtomSoundClass
	UAtomSoundClass* SoundClass = CastChecked<UAtomSoundClass>(ParentGraph->GetOuter());

	if (SoundClass)
	{
		TSharedPtr<IAtomSoundClassEditorToolkit> SoundClassEditor;
		TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(SoundClass);
		if (FoundAssetEditor.IsValid())
		{
			SoundClassEditor = StaticCastSharedPtr<IAtomSoundClassEditorToolkit>(FoundAssetEditor);
			SoundClassEditor->CreateSoundClass(FromPin, Location, NewSoundClassName);
		}
	}

	//FAtomSoundClassEditorUtilities::CreateSoundClass(ParentGraph, FromPin, Location, NewSoundClassName);
	return nullptr;
}

UAtomSoundClassGraphSchema::UAtomSoundClassGraphSchema(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UAtomSoundClassGraphSchema::ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	UAtomSoundClassGraphNode* InputNode = CastChecked<UAtomSoundClassGraphNode>(InputPin->GetOwningNode());
	UAtomSoundClassGraphNode* OutputNode = CastChecked<UAtomSoundClassGraphNode>(OutputPin->GetOwningNode());

	return InputNode->SoundClass->RecurseCheckChild( OutputNode->SoundClass );
}

void UAtomSoundClassGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const FText Name = LOCTEXT("NewAtomSoundClass", "New Atom Sound Class");
	const FText ToolTip = LOCTEXT("NewAtomSoundClassTooltip", "Create a new Atom sound class.");
	
	TSharedPtr<FAtomSoundClassGraphSchemaAction_NewNode> NewAction(new FAtomSoundClassGraphSchemaAction_NewNode(FText::GetEmpty(), Name, ToolTip, 0));

	ContextMenuBuilder.AddAction( NewAction );
}

void UAtomSoundClassGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context->Node)
	{
		const UAtomSoundClassGraphNode* SoundGraphNode = Cast<const UAtomSoundClassGraphNode>(Context->Node);

		{
			FToolMenuSection& Section = Menu->AddSection("AtomSoundClassGraphSchemaNodeActions", LOCTEXT("AtomClassActionsMenuHeader", "AtomSoundClass Actions"));
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
		}
	}

	// No Super call so Node comments option is not shown
}

const FPinConnectionResponse UAtomSoundClassGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both are on the same node"));
	}

	// Compare the directions
	const UEdGraphPin* InputPin = NULL;
	const UEdGraphPin* OutputPin = NULL;

	if (!CategorizePinsByDirection(PinA, PinB, /*out*/ InputPin, /*out*/ OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionIncompatible", "Directions are not compatible"));
	}

	if (ConnectionCausesLoop(InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionLoop", "Connection would cause loop"));
	}

	// Break existing connections on inputs only - multiple output connections are acceptable
	if (InputPin->LinkedTo.Num() > 0)
	{
		ECanCreateConnectionResponse ReplyBreakOutputs;
		if (InputPin == PinA)
		{
			ReplyBreakOutputs = CONNECT_RESPONSE_BREAK_OTHERS_A;
		}
		else
		{
			ReplyBreakOutputs = CONNECT_RESPONSE_BREAK_OTHERS_B;
		}
		return FPinConnectionResponse(ReplyBreakOutputs, LOCTEXT("ConnectionReplace", "Replace existing connections"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UAtomSoundClassGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	bool bModified = UEdGraphSchema::TryCreateConnection(PinA, PinB);

	if (bModified)
	{
		CastChecked<UAtomSoundClassGraph>(PinA->GetOwningNode()->GetGraph())->LinkSoundClasses();
	}

	return bModified;
}

bool UAtomSoundClassGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}

FLinearColor UAtomSoundClassGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FLinearColor::White;
}

void UAtomSoundClassGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	CastChecked<UAtomSoundClassGraph>(TargetNode.GetGraph())->LinkSoundClasses();
}

void UAtomSoundClassGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links") );

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
	
	// if this would notify the node then we need to re-link sound classes
	if (bSendsNodeNotifcation)
	{
		CastChecked<UAtomSoundClassGraph>(TargetPin.GetOwningNode()->GetGraph())->LinkSoundClasses();
	}
}

void UAtomSoundClassGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link") );
	Super::BreakSinglePinLink(SourcePin, TargetPin);

	CastChecked<UAtomSoundClassGraph>(SourcePin->GetOwningNode()->GetGraph())->LinkSoundClasses();
}

void UAtomSoundClassGraphSchema::DroppedAssetsOnGraph(const TArray<struct FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const
{
	UAtomSoundClassGraph* SoundClassGraph = CastChecked<UAtomSoundClassGraph>(Graph);

	TArray<UAtomSoundClass*> UndisplayedClasses;
	for (int32 AssetIdx = 0; AssetIdx < Assets.Num(); ++AssetIdx)
	{
		UAtomSoundClass* SoundClass = Cast<UAtomSoundClass>(Assets[AssetIdx].GetAsset());
		if (SoundClass && !SoundClassGraph->IsClassDisplayed(SoundClass))
		{
			UndisplayedClasses.Add(SoundClass);
		}
	}

	if (UndisplayedClasses.Num() > 0)
	{
		const FScopedTransaction Transaction( LOCTEXT("SoundClassEditorDropClasses", "Atom Sound Class Editor: Drag and Drop Atom Sound Class") );

		SoundClassGraph->AddDroppedSoundClasses(UndisplayedClasses, GraphPosition.X, GraphPosition.Y);
	}
}

#undef LOCTEXT_NAMESPACE
