#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "UObject/Object.h"
#include "K2Node.h"

#include "KismetNodes/SGraphNodeK2Base.h"

#include "EdGraphUtilities.h"

#include "AtomEnvelopeGraphNode.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;

UCLASS(meta = (Keywords = "Atom Envelope"))
class CRIWARECOREBLUEPRINTGRAPH_API UK2Node_MakeAtomEnvelope : public UK2Node
{
    GENERATED_BODY()

public:

    // Overrided from UObject
    virtual void Serialize(FArchive& Ar) override;

    // Overrided from UEdGraphNode.
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;

    // Overrided from UK2Node.
    virtual bool IsNodePure() const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

    EVisibility GetShapeVisibility() const { return bIsShapeVisible ? EVisibility::Visible : EVisibility::Collapsed; }
    FLinearColor GetShapeColor() const { return ShapeColor; }

private:

    mutable bool bIsShapeVisible = true;
    mutable FLinearColor ShapeColor = FLinearColor::Yellow;
};
