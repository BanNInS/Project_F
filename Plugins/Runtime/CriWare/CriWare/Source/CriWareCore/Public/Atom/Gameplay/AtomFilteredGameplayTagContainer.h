
#include "Delegates/Delegate.h" 
#include "GameplayTagContainer.h"

#define CRI_API CRIWARECORE_API

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAtomFilteredGameplayTagAdded, const FGameplayTag)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAtomFilteredGameplayTagRemoved, const FGameplayTag)

namespace AtomFilteredGPTContainer
{
	typedef TPair<FGameplayTag, FGameplayTagQuery> FGPTagQueryPair;
}

/*
*	A container of FGameplayTag associated to a FGameplayTagQuery. 
*	Tags will only be added if the accompanying query is satisfied. 
*	Whenever a tag is added or removed, the tags in the container are "Filtered":
*	i.e. their query is checked to see if they should still remain in the array. 
* 
*	Note:
*	Tags are unique
*	Filtering is evaluated from last to first element in the array
*/
class FAtomFilteredGameplayTagContainer
{
public:

	/**
	* Adds a tag to the container if the query matches. 
	* If the tag is added, proceeds to evaluate the conditions on the contained tags to check 
	* if they should remain or not.
	*
	* @param InPair			The tag and query to try and add to this container
	*
	* @return True if tag was added
	*/
	CRI_API bool AddTagFiltered(const AtomFilteredGPTContainer::FGPTagQueryPair& InPair);
	
	/**
	* Adds a tag to the container if the query matches.
	* If the tag is added, proceeds to evaluate the conditions on the contained tags to check
	* if they should be kept or not.
	*
	* @param InTag			The tag to try add to this container
	* @param InQuery		The query to determine if this tag should be added to the container
	*
	* @return True if tag was added
	*/
	CRI_API bool AddTagFiltered(const FGameplayTag& InTag, const FGameplayTagQuery& InQuery = FGameplayTagQuery());

	/**
	* Removes a tag from the container, then proceeds to evaluate the conditions on the contained tags to check
	* if they should be kept or not.
	*
	* @return True if tag was removed
	*/
	CRI_API bool RemoveTagFiltered(const FGameplayTag& InTag);

	FORCEINLINE int32 Find(const AtomFilteredGPTContainer::FGPTagQueryPair& EventToFind) const
	{
		return GPTagQueryPairContainer.Find(EventToFind);
	}

	FORCEINLINE int32 Num() const
	{
		return GPTagQueryPairContainer.Num();
	}

	FORCEINLINE bool IsEmpty() const
	{
		return GPTagQueryPairContainer.Num() == 0;
	}

	FOnAtomFilteredGameplayTagAdded OnGameplayTagAdded;
	FOnAtomFilteredGameplayTagRemoved OnGameplayTagRemoved;

private:

	CRI_API bool AddTagToContainers(const AtomFilteredGPTContainer::FGPTagQueryPair& InPair);
	CRI_API bool RemoveTagFromContainers(const FGameplayTag& Tag);
	CRI_API void FilterTags();
	CRI_API void CheckContainersConsistency();

	TArray<AtomFilteredGPTContainer::FGPTagQueryPair> GPTagQueryPairContainer;
	FGameplayTagContainer TagContainer;
};

#undef CRI_API
