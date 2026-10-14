/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackGraph.h
 *
 ****************************************************************************/

#pragma once

#include "EdGraph/EdGraph.h"

#include "AtomRackGraph.generated.h"

// Forward Declarations
class UAtomRackBase;
class UAtomRackGraphNode;

UCLASS(MinimalAPI)
class UAtomRackGraph : public UEdGraph
{
	GENERATED_BODY()

public:

	UAtomRackGraph(const FObjectInitializer& ObjectInitializer);

	/**
	 * Set the AtomRack which forms the root of this graph
	 */
	CRIWARECOREEDITOR_API void SetRootAtomRack(UAtomRackBase* InAtomRack);

	/**
	 * Get the AtomRack which forms the root of this graph
	 */
	UAtomRackBase* GetRootAtomRack() const;

	/**
	 * Completely rebuild the graph from the root, removing all old nodes
	 */
	CRIWARECOREEDITOR_API void RebuildGraph();

	/**
	 * Display AtomRacks (and all of their children) that have been dragged onto the editor
	 *
	 * @param	AtomRacks	AtomRacks not already represented on the graph
	 * @param	NodePosX	X coordinate rack(s) were dropped at
	 * @param	NodePosY	Y coordinate rack(s) were dropped at
	 */
	void AddDroppedAtomRacks(const TSet<UAtomRackBase*>& AtomRacks, int32 NodePosX, int32 NodePosY);

	/**
	 * Display a new AtomRack that has just been created using the editor
	 *
	 * @param	FromPin			The Pin that was dragged from to create the AtomRack (may be null)
	 * @param	AtomRack		The newly created AtomRack
	 * @param	NodePosX		X coordinate new rack was created at
	 * @param	NodePosY		Y coordinate new rack was created at
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 */
	CRIWARECOREEDITOR_API void AddNewAtomRack(UEdGraphPin* FromPin, UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);

	/**
	 * Checks whether a AtomRack is already represented on this graph
	 */
	bool IsRackDisplayed(UAtomRackBase* AtomRack) const;

	/**
	 * Use this graph to re-link all of the AtomRacks it represents after a change in linkage
	 */
	void LinkAtomRacks();

	/**
	 * Re-link all of the nodes in this graph after a change to AtomRack linkage
	 */
	CRIWARECOREEDITOR_API void RefreshGraphLinks();

	/**
	 * Recursively remove a set of nodes from this graph and re-link AtomRacks afterwards
	 */
	CRIWARECOREEDITOR_API void RecursivelyRemoveNodes(const TSet<UObject*> NodesToRemove);

	/**
	 * Find an existing node that represents a given AtomRack
	 */
	CRIWARECOREEDITOR_API UAtomRackGraphNode* FindExistingNode(UAtomRackBase* AtomRack) const;

private:

	/**
	 * Construct Nodes to represent a AtomRack and all of its children
	 *
	 * @param	AtomRack		The AtomRack to represent
	 * @param	NodePosX		X coordinate to place first node at
	 * @param	NodePosY		Y coordinate to place first node at
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 * @return	Total height of all constructed nodes (used to arrange multiple new nodes)
	 */
	int32 ConstructNodes(UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);
	/**
	 * Recursively build a map of child counts for each AtomRack to arrange them correctly
	 *
	 * @param	ParentRack		The AtomRack we are getting child counts for
	 * @param	OutChildCounts	Map of child counts
	 * @return	Total child count for ParentClass
	 */
	int32 RecursivelyGatherChildCounts(UAtomRackBase* ParentRack, TMap<UAtomRackBase*, int32>& OutChildCounts);
	/**
	 * Recursively Construct Nodes to represent the children of a AtomRack
	 *
	 * @param	ParentNode		The Node we are constructing children for
	 * @param	InChildCounts	Map of child counts
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 * @return	Total height of constructed nodes (used to arrange next new node)
	 */
	int32 RecursivelyConstructChildNodes(UAtomRackGraphNode* ParentNode, const TMap<UAtomRackBase*, int32>& InChildCounts, bool bSelectNewNode = true);
	/**
	 * Recursively remove a node and its children from the graph
	 */
	void RecursivelyRemoveNode(UAtomRackGraphNode* ParentNode);
	/**
	 * Remove all Nodes from the graph
	 */
	void RemoveAllNodes();
	/**
	 * Create a new node to represent a AtomRack
	 *
	 * @param	AtomRack		The AtomRack to represent
	 * @param	NodePosX		X coordinate to place node at
	 * @param	NodePosY		Y coordinate to place node at
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 * @return	Either a new node or an existing node representing the class
	 */
	UAtomRackGraphNode* CreateNode(UAtomRackBase* AtomRack, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);

private:

	/** AtomRack which forms the root of this graph */
	UPROPERTY(Transient)
	TObjectPtr<UAtomRackBase> RootAtomRack = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAtomRackBase>> StaleRoots;
};
