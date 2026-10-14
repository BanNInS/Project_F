
#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AtomGameplayRequirements.generated.h"

/**
 * UAtomRequirementPreset - A reusable GameplayTagQuery for Atom.
 */
UCLASS(BlueprintType)
class CRIWARECORE_API UAtomRequirementPreset
	: public UDataAsset
{
	GENERATED_BODY()

public:

	UAtomRequirementPreset(const FObjectInitializer& ObjectInitializer);
	~UAtomRequirementPreset() = default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Tag Query")
	FGameplayTagQuery Query;

	/** Returns true if the query is not empty and matches the provided tags */
	bool Matches(const FGameplayTagContainer& Tags) const;
};

/**
 * AtomGameplayRequirements - A set of requirements for Atom gameplay features.
 */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomGameplayRequirements
{
	GENERATED_BODY()

public:

	FAtomGameplayRequirements();

	/** An optional requirement preset tested against a collection of tags to determine a match.  Ignored if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	TObjectPtr<UAtomRequirementPreset> Preset = nullptr;

	/** An optional custom query tested against a collection of tags to determine a match.  Ignored if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	FGameplayTagQuery Custom;

	/** Returns true if the preset and the custom query match the provided tags; ignores preset or custom query if they are empty. */
	bool Matches(const FGameplayTagContainer& Tags) const;
};
