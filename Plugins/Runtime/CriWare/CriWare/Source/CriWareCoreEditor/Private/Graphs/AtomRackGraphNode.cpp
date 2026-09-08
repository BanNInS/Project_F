/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraphNode.cpp
 *
 ****************************************************************************/

#include "AtomRackGraphNode.h"

//#if Unreal 4
//#include "Toolkits/AssetEditorManager.h"

#include "CriWareLocalization.h"
#include "Atom/AtomRack.h"

#include "CriWareCoreEditorPrivate.h"
#include "AtomRackGraphSchema.h"
#include "AtomRackGraph.h"
#include "AtomDefaults.h"
#include "Toolkits/AtomRackEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomRackGraphNode"

UAtomRackGraphNode::UAtomRackGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ChildPin(nullptr)
	, ParentPin(nullptr)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

bool UAtomRackGraphNode::CheckRepresentsAtomRack()
{
	if (!AtomRack)
	{
		return false;
	}

	for (int32 ChildIndex = 0; ChildIndex < ChildPin->LinkedTo.Num(); ChildIndex++)
	{
		UAtomRackGraphNode* ChildNode = CastChecked<UAtomRackGraphNode>(ChildPin->LinkedTo[ChildIndex]->GetOwningNode());
		if (!AtomRack->ChildRacks.Contains(ChildNode->AtomRack))
		{
			return false;
		}
	}

	for (int32 ChildIndex = 0; ChildIndex < AtomRack->ChildRacks.Num(); ChildIndex++)
	{
		bool bFoundChild = false;
		for (int32 NodeChildIndex = 0; NodeChildIndex < ChildPin->LinkedTo.Num(); NodeChildIndex++)
		{
			UAtomRackGraphNode* ChildNode = CastChecked<UAtomRackGraphNode>(ChildPin->LinkedTo[NodeChildIndex]->GetOwningNode());
			if (ChildNode->AtomRack == AtomRack->ChildRacks[ChildIndex])
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

FLinearColor UAtomRackGraphNode::GetNodeTitleColor() const
{
	return Atom::GetColorForRackType(AtomRack);
}

void UAtomRackGraphNode::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	ChildPin = CreatePin(EGPD_Input, Atom::GetNameForRackType(AtomRack), *LOCTEXT("AtomRackGraphNode_Input", "Input").ToString());

	if (UAtomRackWithParentBase* NonNativeRack = Cast<UAtomRackWithParentBase>(AtomRack))
	{
		ParentPin = CreatePin(EGPD_Output, Atom::GetNameForRackType(AtomRack), *LOCTEXT("AtomRackGraphNode_Output", "Output").ToString());
	}
}

void UAtomRackGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin)
	{
		const UAtomRackGraphSchema* Schema = CastChecked<UAtomRackGraphSchema>(GetSchema());

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

bool UAtomRackGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UAtomRackGraphSchema::StaticClass());
}

bool UAtomRackGraphNode::CanUserDeleteNode() const
{
	check(GEditor);
	UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	TArray<IAssetEditorInstance*> AtomRackEditors = EditorSubsystem->FindEditorsForAsset(AtomRack);
	for (IAssetEditorInstance* Editor : AtomRackEditors)
	{
		if (!Editor)
		{
			continue;
		}

		FAtomRackEditorToolkit* AtomRackEditor = static_cast<FAtomRackEditorToolkit*>(Editor);
		if (UEdGraph* Graph = AtomRackEditor->GetGraph())
		{
			if (AtomRack->AtomRackGraph == Graph)
			{
				UAtomRackBase* RootRack = CastChecked<UAtomRackGraph>(Graph)->GetRootAtomRack();
				if (RootRack == AtomRack)
				{
					return false;
				}
			}
		}
	}

	return UEdGraphNode::CanUserDeleteNode();
}

FText UAtomRackGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (AtomRack)
	{
		return FText::FromString(AtomRack->GetName());
	}
	else
	{
		return Super::GetNodeTitle(TitleType);
	}
}

/*TSharedPtr<SGraphNode> UAtomRackGraphNode::CreateVisualWidget()
{
	if (UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
	{
		if (UAtomRackBase* SubmixBase = Cast<UAtomRackBase>(SoundSubmix))
		{
			if (UAudioWidgetSubsystem* AudioWidgetSubsystem = GEngine ? GEngine->GetEngineSubsystem<UAudioWidgetSubsystem>() : nullptr)
			{
				TArray<UUserWidget*> UserWidgets = AudioWidgetSubsystem->CreateUserWidgets(*World, UAtomRackWidgetInterface::StaticClass());
				if (!UserWidgets.IsEmpty())
				{
					// For now, only supports single widget. Gallery system to be implemented to support
					// showing multiple widgets and/or cycling node widgets.
					SubmixNodeUserWidget = UserWidgets[0];
				}
			}

			const UAudioSubmixEditorSettings* Settings = GetDefault<UAudioSubmixEditorSettings>();
			check(Settings);

			// Pass the owning submix and the user widgets to the graph node
			return SNew(SSubmixGraphNode, this)
				.SubmixBase(SubmixBase)
				.SubmixNodeUserWidget(SubmixNodeUserWidget);
		}
	}

	return nullptr;
}*/

#undef LOCTEXT_NAMESPACE
