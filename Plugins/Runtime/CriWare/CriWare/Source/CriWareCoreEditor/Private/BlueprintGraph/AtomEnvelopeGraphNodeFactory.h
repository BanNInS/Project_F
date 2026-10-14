#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraphUtilities.h"
#include "K2Node.h"

#include "Atom/AtomAisacPatch.h" 
#include "SAtomEnvelopeGraphNode.h"

class FAtomEnvelopeGraphNodeFactory : public FGraphPanelNodeFactory
{
public:

    virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override
    {
        if (UK2Node_MakeAtomEnvelope* MakeAtomEnvelopeNode = Cast<UK2Node_MakeAtomEnvelope>(Node))
        {
            return SNew(SGraphNodeMakeAtomEnvelope, MakeAtomEnvelopeNode);
        }
        return nullptr;
    }
};
