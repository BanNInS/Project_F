
#include "AtomSoundClassGraphNode.h"

#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "HAL/PlatformMath.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AssertionMacros.h"
#include "Templates/Casts.h"
#include "UObject/UnrealNames.h"

#include "Atom/AtomSoundClass.h"

#include "AtomSoundClassGraph.h"
#include "AtomSoundClassGraphSchema.h"

#define LOCTEXT_NAMESPACE "AtomSoundClassGraphNode"

/////////////////////////////////////////////////////
// UAtomSoundClassGraphNode

UAtomSoundClassGraphNode::UAtomSoundClassGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ChildPin(NULL)
	, ParentPin(NULL)
{
}

bool UAtomSoundClassGraphNode::CheckRepresentsSoundClass()
{
	if (!SoundClass)
	{
		return false;
	}

	for (int32 ChildIndex = 0; ChildIndex < ChildPin->LinkedTo.Num(); ChildIndex++)
	{
		UAtomSoundClassGraphNode* ChildNode = CastChecked<UAtomSoundClassGraphNode>(ChildPin->LinkedTo[ChildIndex]->GetOwningNode());
		if (!SoundClass->ChildClasses.Contains(ChildNode->SoundClass))
		{
			return false;
		}
	}

	for (int32 ChildIndex = 0; ChildIndex < SoundClass->ChildClasses.Num(); ChildIndex++)
	{
		bool bFoundChild = false;
		for (int32 NodeChildIndex = 0; NodeChildIndex < ChildPin->LinkedTo.Num(); NodeChildIndex++)
		{
			UAtomSoundClassGraphNode* ChildNode = CastChecked<UAtomSoundClassGraphNode>(ChildPin->LinkedTo[NodeChildIndex]->GetOwningNode());
			if (ChildNode->SoundClass == SoundClass->ChildClasses[ChildIndex])
			{
				bFoundChild = true;
				break;
			}
		}

		if (!bFoundChild)
		{
			return false;
		}
	}

	return true;
}

FLinearColor UAtomSoundClassGraphNode::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

void UAtomSoundClassGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	ChildPin = CreatePin(EGPD_Output, TEXT("AtomSoundClass"), *LOCTEXT("AtomSoundClassChildren", "Children").ToString());
	ParentPin = CreatePin(EGPD_Input, TEXT("AtomSoundClass"), NAME_None);
}

void UAtomSoundClassGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin)
	{
		const UAtomSoundClassGraphSchema* Schema = CastChecked<UAtomSoundClassGraphSchema>(GetSchema());

		if (FromPin->Direction == EGPD_Input)
		{
			Schema->TryCreateConnection(FromPin, ChildPin);
		}
		else
		{
			Schema->TryCreateConnection(FromPin, ParentPin);
		}
	}
}

bool UAtomSoundClassGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UAtomSoundClassGraphSchema::StaticClass());
}

FText UAtomSoundClassGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (SoundClass)
	{
		return FText::FromString(SoundClass->GetName());
	}
	else
	{
		return Super::GetNodeTitle(TitleType);
	}
}

bool UAtomSoundClassGraphNode::CanUserDeleteNode() const
{
	UAtomSoundClassGraph* SoundClassGraph = CastChecked<UAtomSoundClassGraph>(GetGraph());

	// Cannot remove the root node from the graph
	return SoundClass != SoundClassGraph->GetRootSoundClass();
}

#undef LOCTEXT_NAMESPACE
