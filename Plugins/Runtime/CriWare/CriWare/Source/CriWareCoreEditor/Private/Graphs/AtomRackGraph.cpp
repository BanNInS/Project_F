/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraph.cpp
 *
 ****************************************************************************/

#include "AtomRackGraph.h"

#include "GraphEditor.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CriWareLocalization.h"
#include "Atom/AtomRack.h"

#include "AtomRackGraphNode.h"

#define LOCTEXT_NAMESPACE "AtomRackGraph"

UAtomRackGraph::UAtomRackGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomRackGraph::SetRootAtomRack(UAtomRackBase* InAtomRack)
{
	if (RootAtomRack)
	{
		// Defer request to close stale editor(s) to avoid property assignments being
		// added to undo/redo transaction stack from the process of reopening the new editor.
		// This can occur if client code calling this function is performed within a scoped
		// transaction (as it most likely is).
		const int32 Size = StaleRoots.Num();
		const int32 Index = StaleRoots.AddUnique(RootAtomRack);
		if (Size == Index && GEditor)
		{
			GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
			{
				if (StaleRoots.Num() > 0)
				{
					check(GEditor);
					if (UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
					{
						for (UAtomRackBase* Rack : StaleRoots)
						{
							TArray<IAssetEditorInstance*> AtomRackEditors = EditorSubsystem->FindEditorsForAsset(Rack);
							for (IAssetEditorInstance* Editor : AtomRackEditors)
							{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
								Editor->CloseWindow(EAssetEditorCloseReason::EditorRefreshRequested);
#else
								Editor->CloseWindow();
#endif
							}
						}

						if (RootAtomRack)
						{
							if (EditorSubsystem->FindEditorsForAsset(RootAtomRack).Num() == 0)
							{
								EditorSubsystem->OpenEditorForAsset(RootAtomRack);
							}
						}
					}

					StaleRoots.Reset();
				}
			}));
		}
	}

	RootAtomRack = InAtomRack;
}

UAtomRackBase* UAtomRackGraph::GetRootAtomRack() const
{
	return RootAtomRack;
}

void UAtomRackGraph::RebuildGraph()
{
	check(RootAtomRack);

	// Don't allow initial graph rebuild to affect package dirty state; remember current state...
	UPackage* Package = GetOutermost();
	const bool bIsDirty = Package->IsDirty();

	Modify();

	RemoveAllNodes();

	ConstructNodes(RootAtomRack, 0, 0);

	NotifyGraphChanged();

	// ...and restore it
	Package->SetDirtyFlag(bIsDirty);
}

void UAtomRackGraph::AddDroppedAtomRacks(const TSet<UAtomRackBase*>& AtomRacks, int32 NodePosX, int32 NodePosY)
{
	Modify();

	for (UAtomRackBase* AtomRack : AtomRacks)
	{
		NodePosY += ConstructNodes(AtomRack, NodePosX, NodePosY);
	}

	NotifyGraphChanged();
}

void UAtomRackGraph::AddNewAtomRack(UEdGraphPin* FromPin, UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode /* = true */)
{
	check(AtomRack->ChildRacks.Num() == 0);

	Modify();

	UAtomRackGraphNode* GraphNode = CreateNode(AtomRack, NodePosX, NodePosY, bSelectNewNode);
	GraphNode->AutowireNewNode(FromPin);

	NotifyGraphChanged();
}

bool UAtomRackGraph::IsRackDisplayed(UAtomRackBase* AtomRack) const
{
	return FindExistingNode(AtomRack) != nullptr;
}

void UAtomRackGraph::LinkAtomRacks()
{
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++)
	{
		UAtomRackGraphNode* Node = CastChecked<UAtomRackGraphNode>(Nodes[NodeIndex]);

		if (!Node->CheckRepresentsAtomRack())
		{
			Node->AtomRack->Modify();

			// remove parents of existing children
			for (UAtomRackBase* ChildRack : Node->AtomRack->ChildRacks)
			{
				if (ChildRack)
				{
					ChildRack->Modify();
					if (UAtomRackWithParentBase* RackWithParent = CastChecked<UAtomRackWithParentBase>(ChildRack))
					{
						RackWithParent->ParentRack = nullptr;
					}
				}
			}

			Node->AtomRack->ChildRacks.Empty();

			if (UEdGraphPin* ChildPin = Node->GetChildPin())
			{
				for (UEdGraphPin* GraphPin : ChildPin->LinkedTo)
				{


					if (!GraphPin)
					{
						continue;
					}

					UAtomRackGraphNode* ChildNode = CastChecked<UAtomRackGraphNode>(GraphPin->GetOwningNode());

					// If the child rack we're connecting to isn't the type of rack that has an output, continue.
					UAtomRackWithParentBase* ChildRackWithParent = Cast<UAtomRackWithParentBase>(ChildNode->AtomRack);

					if (ChildRackWithParent)
					{
						Node->AtomRack->ChildRacks.Add(ChildNode->AtomRack);
						ChildRackWithParent->SetParentRack(Node->AtomRack);
					}
				}
			}

			Node->AtomRack->PostEditChange();
			Node->AtomRack->MarkPackageDirty();
		}
	}
}

void UAtomRackGraph::RefreshGraphLinks()
{
	Modify();

	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++)
	{
		UAtomRackGraphNode* Node = CastChecked<UAtomRackGraphNode>(Nodes[NodeIndex]);

		if (!Node->CheckRepresentsAtomRack())
		{
			UEdGraphPin* ChildPin = Node->GetChildPin();

			Node->Modify();

			ChildPin->BreakAllPinLinks();

			if (Node->AtomRack)
			{
				for (int32 ChildIndex = 0; ChildIndex < Node->AtomRack->ChildRacks.Num(); ChildIndex++)
				{
					UAtomRackBase* ChildRack = Node->AtomRack->ChildRacks[ChildIndex];

					if (ChildRack)
					{
						UAtomRackGraphNode* ChildNode = FindExistingNode(ChildRack);

						if (!ChildNode)
						{
							// New Child not yet represented on graph
							ConstructNodes(ChildRack, Node->NodePosX + 400, Node->NodePosY);
							ChildNode = FindExistingNode(ChildRack);
						}

						ChildPin->MakeLinkTo(ChildNode->GetParentPin());
					}
				}
			}

			Node->PostEditChange();
		}
	}

	NotifyGraphChanged();
}

void UAtomRackGraph::RecursivelyRemoveNodes(const TSet<UObject*> NodesToRemove)
{
	Modify();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(NodesToRemove); NodeIt; ++NodeIt)
	{
		UAtomRackGraphNode* Node = Cast<UAtomRackGraphNode>(*NodeIt);

		if (Node && Node->CanUserDeleteNode())
		{
			RecursivelyRemoveNode(Node);
		}
	}

	LinkAtomRacks();
}

int32 UAtomRackGraph::ConstructNodes(UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode /* = true */)
{
	check(AtomRack);

	TMap<UAtomRackBase*, int32> ChildCounts;

	RecursivelyGatherChildCounts(AtomRack, ChildCounts);

	UAtomRackGraphNode* GraphNode = CreateNode(AtomRack, NodePosX, NodePosY, bSelectNewNode);

	return RecursivelyConstructChildNodes(GraphNode, ChildCounts);
}

int32 UAtomRackGraph::RecursivelyGatherChildCounts(UAtomRackBase* ParentRack, TMap<UAtomRackBase*, int32>& OutChildCounts)
{
	int32 ChildSize = 0;

	for (int32 ChildIndex = 0; ChildIndex < ParentRack->ChildRacks.Num(); ChildIndex++)
	{
		if (ParentRack->ChildRacks[ChildIndex])
		{
			ChildSize += RecursivelyGatherChildCounts(ParentRack->ChildRacks[ChildIndex], OutChildCounts);
		}
	}

	if (ChildSize == 0)
	{
		ChildSize = 1;
	}

	OutChildCounts.Add(ParentRack, ChildSize);
	return ChildSize;
}

int32 UAtomRackGraph::RecursivelyConstructChildNodes(UAtomRackGraphNode* ParentNode, const TMap<UAtomRackBase*, int32>& InChildCounts, bool bSelectNewNode /* = true*/)
{
	static const int32 HorizontalSpacing = -400;
	static const int32 VerticalSpacing = 100;

	UAtomRackBase* ParentRack = ParentNode->AtomRack;
	int32 TotalChildSizeY = InChildCounts.FindChecked(ParentRack) * VerticalSpacing;
	int32 NodeStartY = ParentNode->NodePosY - (TotalChildSizeY * 0.5f) + (VerticalSpacing * 0.5f);
	int32 NodePosX = ParentNode->NodePosX + HorizontalSpacing;

	for (int32 ChildIndex = 0; ChildIndex < ParentRack->ChildRacks.Num(); ChildIndex++)
	{
		if (ParentRack->ChildRacks[ChildIndex])
		{
			const int32 ChildCount = InChildCounts.FindChecked(ParentRack->ChildRacks[ChildIndex]);
			int32 NodePosY = NodeStartY + (ChildCount * VerticalSpacing * 0.5f) - (VerticalSpacing * 0.5f);
			UAtomRackGraphNode* ChildNode = CreateNode(ParentRack->ChildRacks[ChildIndex], NodePosX, NodePosY, bSelectNewNode);
			ParentNode->GetChildPin()->MakeLinkTo(ChildNode->GetParentPin());
			RecursivelyConstructChildNodes(ChildNode, InChildCounts);
			NodeStartY += ChildCount * VerticalSpacing;
		}
	}

	return TotalChildSizeY;
}

void UAtomRackGraph::RecursivelyRemoveNode(UAtomRackGraphNode* ParentNode)
{
	UEdGraphPin* ChildPin = ParentNode->GetChildPin();

	for (int32 ChildIndex = ChildPin->LinkedTo.Num() - 1; ChildIndex >= 0; ChildIndex--)
	{
		UAtomRackGraphNode* ChildNode = CastChecked<UAtomRackGraphNode>(ChildPin->LinkedTo[ChildIndex]->GetOwningNode());
		RecursivelyRemoveNode(ChildNode);
	}

	ParentNode->Modify();
	RemoveNode(ParentNode);
}

void UAtomRackGraph::RemoveAllNodes()
{
	TArray<UEdGraphNode*> NodesToRemove = Nodes;
	for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
	{
		NodesToRemove[NodeIndex]->Modify();
		RemoveNode(NodesToRemove[NodeIndex]);
	}
}

UAtomRackGraphNode* UAtomRackGraph::CreateNode(UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode /* = true */)
{
	UAtomRackGraphNode* GraphNode = FindExistingNode(AtomRack);

	if (!GraphNode)
	{
		FGraphNodeCreator<UAtomRackGraphNode> NodeCreator(*this);
		GraphNode = NodeCreator.CreateNode(bSelectNewNode);
		GraphNode->AtomRack = AtomRack;
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
		NodeCreator.Finalize();
	}
	return GraphNode;
}

UAtomRackGraphNode* UAtomRackGraph::FindExistingNode(UAtomRackBase* AtomRack) const
{
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		UAtomRackGraphNode* Node = CastChecked<UAtomRackGraphNode>(Nodes[NodeIndex]);
		if (Node->AtomRack == AtomRack)
		{
			return Node;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
