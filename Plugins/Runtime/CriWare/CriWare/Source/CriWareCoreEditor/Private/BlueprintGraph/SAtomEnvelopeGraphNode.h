#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraphUtilities.h"
#include "KismetNodes/SGraphNodeK2Base.h"

#include "AtomEnvelopeGraphNode.h"

// Forward Definitions
class UEdGraphPin;
enum class EAtomEnvelopeCurve : uint8;

class SGraphNodeMakeAtomEnvelope : public SGraphNodeK2Base
{
public:

    SLATE_BEGIN_ARGS(SGraphNodeMakeAtomEnvelope)
    {
    }
    SLATE_END_ARGS()
    
    void Construct(const FArguments& InArgs, UK2Node_MakeAtomEnvelope* InNode);
    
    //~ Begin SGraphNodeK2Base Interface
    virtual TSharedRef<SWidget> CreateNodeContentArea() override;
    //~ End SGraphNodeK2Base Interface

private:

    float GetParameterValue(const UEdGraphPin* Pin) const;
    EAtomEnvelopeCurve GetParameterCurve(const UEdGraphPin* Pin) const;
};
