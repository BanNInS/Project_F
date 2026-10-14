/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraphNode.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNode.h"

#include "AtomRackGraphNode.generated.h"

class UAtomRackBase;

UCLASS(MinimalAPI)
class UAtomRackGraphNode
	: public UEdGraphNode
{
	GENERATED_BODY()

public:

	UAtomRackGraphNode(const FObjectInitializer& ObjectInitializer);

	/** The AtomRack this represents */
	UPROPERTY(VisibleAnywhere, instanced, Category = Atom)
	TObjectPtr<UAtomRackBase> AtomRack;

	/** Get the Pin that connects to all children */
	UEdGraphPin* GetChildPin() const { return ChildPin; }

	/** Get the Pin that connects to its parent */
	UEdGraphPin* GetParentPin() const { return ParentPin; }

	/** Check whether the children of this node match the AtomRack it is representing */
	bool CheckRepresentsAtomRack();

	//~ Begin UEdGraphNode Interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void AllocateDefaultPins() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual bool CanUserDeleteNode() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//virtual TSharedPtr<SGraphNode> CreateVisualWidget();
	//~ End UEdGraphNode Interface.

private:

	/** Pin that connects to all children */
	UEdGraphPin* ChildPin;

	/** Pin that connects to its parent */
	UEdGraphPin* ParentPin;
};
