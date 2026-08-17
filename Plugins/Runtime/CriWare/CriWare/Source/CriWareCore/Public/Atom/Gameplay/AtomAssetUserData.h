
#pragma once

#include "Engine/AssetUserData.h"
#include "GameplayTagContainer.h"

#include "AtomAssetUserData.generated.h"

/**
 * UAtomAssetUserData - Base class for user data being attached to Atom assets
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomAssetUserData : public UAssetUserData
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags", meta = (Categories = "Atom.Metadata"))
	FGameplayTagContainer MetadataTags;
};
