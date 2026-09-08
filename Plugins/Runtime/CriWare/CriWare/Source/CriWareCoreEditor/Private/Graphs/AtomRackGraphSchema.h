/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraphSchema.h
 *
 ****************************************************************************/

#pragma once

#include "AssetRegistry/AssetData.h"
#include "ConnectionDrawingPolicy.h"
#include "EdGraphUtilities.h"
#include "Misc/EngineVersionComparison.h"

#include "AtomRackGraphSchema.generated.h"


// Forward Declarations
class UEdGraph;
class UEdGraphSchema;
class FMenuBuilder;

struct FAtomRackGraphConnectionDrawingPolicyFactory
	: public FGraphPanelPinConnectionFactory
{
public:

	virtual ~FAtomRackGraphConnectionDrawingPolicyFactory() {}

	//~ FGraphPanelPinConnectionFactory Interface
	virtual class FConnectionDrawingPolicy* CreateConnectionPolicy(
		const UEdGraphSchema* Schema,
		int32 InBackLayerID,
		int32 InFrontLayerID,
		float ZoomFactor,
		const FSlateRect& InClippingRect,
		FSlateWindowElementList& InDrawElements,
		UEdGraph* InGraphObj) const override;
};

// This class draws the connections for an UEdGraph using a AtomRack schema
class FAtomRackGraphConnectionDrawingPolicy
	: public FConnectionDrawingPolicy
{
protected:

	// Times for one execution pair within the current graph
	struct FTimePair
	{
		double PredExecTime;
		double ThisExecTime;

		FTimePair()
			: PredExecTime(0.0)
			, ThisExecTime(0.0)
		{}
	};

	// Map of pairings
	typedef TMap<UEdGraphNode*, FTimePair> FExecPairingMap;

	// Map of nodes that preceded before a given node in the execution sequence (one entry for each pairing)
	TMap<UEdGraphNode*, FExecPairingMap> PredecessorNodes;

	UEdGraph* GraphObj;

	float ActiveWireThickness;
	float InactiveWireThickness;

public:
	FAtomRackGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	//~ Begin FConnectionDrawingPolicy Interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	//~ End FConnectionDrawingPolicy Interface
};

/** Action to add a node to the graph */
USTRUCT()
struct CRIWARECOREEDITOR_API FAtomRackGraphSchemaAction_NewNode
	: public FEdGraphSchemaAction
{
	GENERATED_BODY();

	// Simple type info
	static FName StaticGetTypeId() { static FName Type("FAtomRackGraphSchemaAction_NewNode"); return Type; }

	FAtomRackGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
		, NewAtomRackName(TEXT("RackName"))
	{}

	FAtomRackGraphSchemaAction_NewNode(FText InNodeCategory, FText InMenuDesc, FText InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		, NewAtomRackName(TEXT("RackName"))
	{}

	//~ Begin FEdGraphSchemaAction Interface
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
    virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;
#else
    virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
#endif
	//~ End FEdGraphSchemaAction Interface

	/** Name for the new AtomRack */
	FString NewAtomRackName;
};

UCLASS(MinimalAPI)
class UAtomRackGraphSchema
	: public UEdGraphSchema
{
	GENERATED_BODY()

public:

	UAtomRackGraphSchema(const FObjectInitializer& ObjectInitializer);

	/** Check whether connecting these pins would cause a loop */
	bool ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const;

	//~ Begin EdGraphSchema Interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual FName GetParentContextMenuName() const override { return NAME_None; }
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
	virtual bool TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const override;
	virtual bool ShouldHidePinDefaultValue(UEdGraphPin* Pin) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void BreakNodeLinks(UEdGraphNode& TargetNode) const override;
	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const override;
	virtual void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const override;
	virtual void DroppedAssetsOnGraph(const TArray<struct FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* Graph) const override;
	//~ End EdGraphSchema Interface
};
