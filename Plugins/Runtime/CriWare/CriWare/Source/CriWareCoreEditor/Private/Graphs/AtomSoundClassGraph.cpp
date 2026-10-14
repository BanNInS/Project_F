
#include "Graphs/AtomSoundClassGraph.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "GraphEditor.h"
#include "HAL/PlatformCrt.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Package.h"

#include "Atom/AtomSoundClass.h"

#include "AtomSoundClassGraphNode.h"

class FAtomSoundClassEditor
	: public IAtomSoundClassEditor
{
public:
	void RefreshGraphLinks(UEdGraph* SoundClassGraph) override
	{
		CastChecked<UAtomSoundClassGraph>(SoundClassGraph)->RefreshGraphLinks();
	}
};

UAtomSoundClassGraph::UAtomSoundClassGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RootSoundClass(NULL)
{
	if (!UAtomSoundClass::GetSoundClassEditor().IsValid())
	{
		UAtomSoundClass::SetSoundClassEditor(TSharedPtr<IAtomSoundClassEditor>(new FAtomSoundClassEditor()));
	}
}

void UAtomSoundClassGraph::SetRootSoundClass(UAtomSoundClass* InSoundClass)
{
	RootSoundClass = InSoundClass;
}

UAtomSoundClass* UAtomSoundClassGraph::GetRootSoundClass() const
{
	return RootSoundClass;
}

void UAtomSoundClassGraph::RebuildGraph()
{
	check(RootSoundClass);

	// Don't allow initial graph rebuild to affect package dirty state; remember current state...
	UPackage* Package = GetOutermost();
	const bool bIsDirty = Package->IsDirty();

	Modify();

	RemoveAllNodes();

	ConstructNodes(RootSoundClass, 0, 0);

	NotifyGraphChanged();

	// ...and restore it
	Package->SetDirtyFlag(bIsDirty);
}

void UAtomSoundClassGraph::AddDroppedSoundClasses(const TArray<UAtomSoundClass*>& SoundClasses, int32 NodePosX, int32 NodePosY)
{
	Modify();

	for (int32 ClassIndex = 0; ClassIndex < SoundClasses.Num(); ClassIndex++)
	{
		NodePosY += ConstructNodes(SoundClasses[ClassIndex], NodePosX, NodePosY);
	}

	NotifyGraphChanged();
}

void UAtomSoundClassGraph::AddNewSoundClass(UEdGraphPin* FromPin, class UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode/* = true*/)
{
	Modify();

	UAtomSoundClassGraphNode* GraphNode = CreateNode(SoundClass, NodePosX, NodePosY, bSelectNewNode);
	GraphNode->AutowireNewNode(FromPin);

	NotifyGraphChanged();
}

bool UAtomSoundClassGraph::IsClassDisplayed(UAtomSoundClass* SoundClass) const
{
	return FindExistingNode(SoundClass) != NULL;
}

void UAtomSoundClassGraph::LinkSoundClasses()
{
	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++)
	{
		UAtomSoundClassGraphNode* Node = CastChecked<UAtomSoundClassGraphNode>(Nodes[NodeIndex]);

		if (!Node->CheckRepresentsSoundClass())
		{
			Node->SoundClass->Modify();

			// remove parents of existing children
			for (int32 ChildIndex = 0; ChildIndex < Node->SoundClass->ChildClasses.Num(); ChildIndex++)
			{
				UAtomSoundClass* ChildClass = Node->SoundClass->ChildClasses[ChildIndex];

				if (ChildClass)
				{
					ChildClass->Modify();
					ChildClass->ParentClass = NULL;
				}
			}

			Node->SoundClass->ChildClasses.Empty();

			UEdGraphPin* ChildPin = Node->GetChildPin();

			for (int32 ChildIndex = 0; ChildIndex < ChildPin->LinkedTo.Num(); ChildIndex++)
			{
				UAtomSoundClassGraphNode* ChildNode = CastChecked<UAtomSoundClassGraphNode>(ChildPin->LinkedTo[ChildIndex]->GetOwningNode());
				Node->SoundClass->ChildClasses.Add(ChildNode->SoundClass);
				ChildNode->SoundClass->SetParentClass(Node->SoundClass);
			}

			Node->SoundClass->PostEditChange();
			Node->SoundClass->MarkPackageDirty();
		}
	}

	RootSoundClass->RefreshAllGraphs(true);
}

void UAtomSoundClassGraph::RefreshGraphLinks()
{
	Modify();

	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++)
	{
		UAtomSoundClassGraphNode* Node = CastChecked<UAtomSoundClassGraphNode>(Nodes[NodeIndex]);

		if (!Node->CheckRepresentsSoundClass())
		{
			UEdGraphPin* ChildPin = Node->GetChildPin();

			Node->Modify();

			ChildPin->BreakAllPinLinks();

			if (Node->SoundClass)
			{
				for (int32 ChildIndex = 0; ChildIndex < Node->SoundClass->ChildClasses.Num(); ChildIndex++)
				{
					UAtomSoundClass* ChildClass = Node->SoundClass->ChildClasses[ChildIndex];

					if (ChildClass)
					{
						UAtomSoundClassGraphNode* ChildNode = FindExistingNode(ChildClass);

						if (!ChildNode)
						{
							// New Child not yet represented on graph
							ConstructNodes(ChildClass, Node->NodePosX + 400, Node->NodePosY);
							ChildNode = FindExistingNode(ChildClass);
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

void UAtomSoundClassGraph::RecursivelyRemoveNodes(const TSet<class UObject*> NodesToRemove)
{
	Modify();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(NodesToRemove); NodeIt; ++NodeIt)
	{
		UAtomSoundClassGraphNode* Node = Cast<UAtomSoundClassGraphNode>(*NodeIt);

		if (Node && Node->CanUserDeleteNode())
		{
			RecursivelyRemoveNode(Node);
		}
	}

	LinkSoundClasses();
}

int32 UAtomSoundClassGraph::ConstructNodes(class UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode/* = true*/)
{
	check(SoundClass);

	TMap<UAtomSoundClass*, int32> ChildCounts;

	RecursivelyGatherChildCounts(SoundClass, ChildCounts);

	UAtomSoundClassGraphNode* GraphNode = CreateNode(SoundClass, NodePosX, NodePosY, bSelectNewNode);

	return RecursivelyConstructChildNodes(GraphNode, ChildCounts);
}

int32 UAtomSoundClassGraph::RecursivelyGatherChildCounts(UAtomSoundClass* ParentClass, TMap<UAtomSoundClass*, int32>& OutChildCounts)
{
	int32 ChildSize = 0;

	for (int32 ChildIndex = 0; ChildIndex < ParentClass->ChildClasses.Num(); ChildIndex++)
	{
		if (ParentClass->ChildClasses[ChildIndex])
		{
			ChildSize += RecursivelyGatherChildCounts(ParentClass->ChildClasses[ChildIndex], OutChildCounts);
		}
	}

	if (ChildSize == 0)
	{
		ChildSize = 1;
	}

	OutChildCounts.Add(ParentClass, ChildSize);
	return ChildSize;
}

int32 UAtomSoundClassGraph::RecursivelyConstructChildNodes(UAtomSoundClassGraphNode* ParentNode, const TMap<UAtomSoundClass*, int32>& InChildCounts, bool bSelectNewNode/* = true*/)
{
	const int32 HorizontalSpacing = 400;
	const int32 VerticalSpacing = 100;

	UAtomSoundClass* ParentClass = ParentNode->SoundClass;
	int32 TotalChildSizeY = InChildCounts.FindChecked(ParentClass) * VerticalSpacing;
	int32 NodeStartY = ParentNode->NodePosY - (TotalChildSizeY * 0.5f) + (VerticalSpacing * 0.5f);
	int32 NodePosX = ParentNode->NodePosX + HorizontalSpacing;

	for (int32 ChildIndex = 0; ChildIndex < ParentClass->ChildClasses.Num(); ChildIndex++)
	{
		if (ParentClass->ChildClasses[ChildIndex])
		{
			const int32 ChildCount = InChildCounts.FindChecked(ParentClass->ChildClasses[ChildIndex]);
			int32 NodePosY = NodeStartY + (ChildCount * VerticalSpacing * 0.5f) - (VerticalSpacing * 0.5f);
			UAtomSoundClassGraphNode* ChildNode = CreateNode(ParentClass->ChildClasses[ChildIndex], NodePosX, NodePosY, bSelectNewNode);
			ParentNode->GetChildPin()->MakeLinkTo(ChildNode->GetParentPin());
			RecursivelyConstructChildNodes(ChildNode, InChildCounts);
			NodeStartY += ChildCount * VerticalSpacing;
		}
	}

	return TotalChildSizeY;
}

void UAtomSoundClassGraph::RecursivelyRemoveNode(class UAtomSoundClassGraphNode* ParentNode)
{
	UEdGraphPin* ChildPin = ParentNode->GetChildPin();

	for (int32 ChildIndex = ChildPin->LinkedTo.Num() - 1; ChildIndex >= 0; ChildIndex--)
	{
		UAtomSoundClassGraphNode* ChildNode = CastChecked<UAtomSoundClassGraphNode>(ChildPin->LinkedTo[ChildIndex]->GetOwningNode());
		RecursivelyRemoveNode(ChildNode);
	}

	ParentNode->Modify();
	RemoveNode(ParentNode);
}

void UAtomSoundClassGraph::RemoveAllNodes()
{
	TArray<UEdGraphNode*> NodesToRemove = Nodes;
	for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
	{
		NodesToRemove[NodeIndex]->Modify();
		RemoveNode(NodesToRemove[NodeIndex]);
	}
}

UAtomSoundClassGraphNode* UAtomSoundClassGraph::CreateNode(UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode/* = true*/)
{
	UAtomSoundClassGraphNode* GraphNode = FindExistingNode(SoundClass);

	if (!GraphNode)
	{
		FGraphNodeCreator<UAtomSoundClassGraphNode> NodeCreator(*this);
		GraphNode = NodeCreator.CreateNode(bSelectNewNode);
		GraphNode->SoundClass = SoundClass;
		GraphNode->NodePosX = NodePosX;
		GraphNode->NodePosY = NodePosY;
		NodeCreator.Finalize();
	}
	return GraphNode;
}

UAtomSoundClassGraphNode* UAtomSoundClassGraph::FindExistingNode(UAtomSoundClass* SoundClass) const
{
	UAtomSoundClassGraphNode* ExistingNode = NULL;

	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		UAtomSoundClassGraphNode* Node = CastChecked<UAtomSoundClassGraphNode>(Nodes[NodeIndex]);
		if (Node->SoundClass == SoundClass)
		{
			ExistingNode = Node;
			break;
		}
	}

	return ExistingNode;
}
