
#pragma once

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "AtomSoundClassGraph.generated.h"

class UEdGraphPin;
class UObject;

UCLASS(MinimalAPI)
class UAtomSoundClassGraph
	: public UEdGraph
{
	GENERATED_BODY()

public:

	/** Constructor */
	UAtomSoundClassGraph(const FObjectInitializer& ObjectInitializer);

	/**
	 * Set the AtomSoundClass which forms the root of this graph
	 */
	CRIWARECOREEDITOR_API void SetRootSoundClass(class UAtomSoundClass* InSoundClass);
	
	/**
	 * Get the AtomSoundClass which forms the root of this graph
	 */
	class UAtomSoundClass* GetRootSoundClass() const;
	
	/**
	 * Completely rebuild the graph from the root, removing all old nodes
	 */
	CRIWARECOREEDITOR_API void RebuildGraph();
	
	/**
	 * Display AtomSoundClasses (and all of their children) that have been dragged onto the editor
	 *
	 * @param	SoundClasses	AtomSoundClasses not already represented on the graph
	 * @param	NodePosX		X coordinate classes were dropped at
	 * @param	NodePosY		Y coordinate classes were dropped at
	 */
	void AddDroppedSoundClasses(const TArray<class UAtomSoundClass*>& SoundClasses, int32 NodePosX, int32 NodePosY);

	/**
	 * Display a new AtomSoundClass that has just been created using the editor
	 *
	 * @param	FromPin			The Pin that was dragged from to create the AtomSoundClass (may be NULL)
	 * @param	SoundClass		The newly created AtomSoundClass
	 * @param	NodePosX		X coordinate new class was created at
	 * @param	NodePosY		Y coordinate new class was created at
	 * @param bSelectNewNode	Whether or not to select the new node being created
	 */
	CRIWARECOREEDITOR_API void AddNewSoundClass(UEdGraphPin* FromPin, class UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);
	
	/**
	 * Checks whether a AtomSoundClass is already represented on this graph
	 */
	bool IsClassDisplayed(class UAtomSoundClass* SoundClass) const;

	/**
	 * Use this graph to re-link all of the AtomSoundClasses it represents after a change in linkage
	 */
	void LinkSoundClasses();

	/**
	 * Re-link all of the nodes in this graph after a change to AtomSoundClass linkage
	 */
	CRIWARECOREEDITOR_API void RefreshGraphLinks();

	/**
	 * Recursively remove a set of nodes from this graph and re-link AtomSoundClasses afterwards
	 */
	CRIWARECOREEDITOR_API void RecursivelyRemoveNodes(const TSet<class UObject*> NodesToRemove);

private:

	/**
	 * Construct Nodes to represent a AtomSoundClass and all of its children
	 *
	 * @param	SoundClass		The AtomSoundClass to represent
	 * @param	NodePosX		X coordinate to place first node at
	 * @param	NodePosY		Y coordinate to place first node at
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 * @return	Total height of all constructed nodes (used to arrange multiple new nodes)
	 */
	int32 ConstructNodes(class UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);
	
	/**
	 * Recursively build a map of child counts for each AtomSoundClass to arrange them correctly
	 *
	 * @param	ParentClass		The class we are getting child counts for
	 * @param	OutChildCounts	Map of child counts
	 * @return	Total child count for ParentClass
	 */
	int32 RecursivelyGatherChildCounts(class UAtomSoundClass* ParentClass, TMap<class UAtomSoundClass*, int32>& OutChildCounts);
	
	/**
	 * Recursively Construct Nodes to represent the children of a AtomSoundClass
	 *
	 * @param	ParentNode		The Node we are constructing children for
	 * @param	InChildCounts	Map of child counts
	 * @param	bSelectNewNode	Whether or not to select the new node being created
	 * @return	Total height of constructed nodes (used to arrange next new node)
	 */
	int32 RecursivelyConstructChildNodes(class UAtomSoundClassGraphNode* ParentNode, const TMap<class UAtomSoundClass*, int32>& InChildCounts, bool bSelectNewNode = true);
	
	/**
	 * Recursively remove a node and its children from the graph
	 */
	void RecursivelyRemoveNode(class UAtomSoundClassGraphNode* ParentNode);
	
	/**
	 * Remove all Nodes from the graph
	 */
	void RemoveAllNodes();
	
	/**
	 * Create a new node to represent a AtomSoundClass
	 *
	 * @param	SoundClass	The AtomSoundClass to represent
	 * @param	NodePosX	X coordinate to place node at
	 * @param	NodePosY	Y coordinate to place node at
	 * @param bSelectNewNode	Whether or not to select the new node being created
	 * @return	Either a new node or an existing node representing the class
	 */
	class UAtomSoundClassGraphNode* CreateNode(class UAtomSoundClass* SoundClass, int32 NodePosX, int32 NodePosY, bool bSelectNewNode = true);
	
	/**
	 * Find an existing node that represents a given AtomSoundClass
	 */
	class UAtomSoundClassGraphNode* FindExistingNode(class UAtomSoundClass* SoundClass) const;

private:

	/** SoundClass which forms the root of this graph */
	class UAtomSoundClass*	RootSoundClass;
};

