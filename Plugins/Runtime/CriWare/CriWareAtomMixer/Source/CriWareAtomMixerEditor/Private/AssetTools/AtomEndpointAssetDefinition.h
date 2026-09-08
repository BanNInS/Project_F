


#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomBus.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomEndpointAssetDefinition.generated.h"

UCLASS()
class UAtomEndpointAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomEndpoint", "Atom Endpoint"); }
	virtual FLinearColor GetAssetColor() const override { return FColor(249, 242, 231); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomEndpoint::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundRendererSubMenu", "Mixer") };
		return Categories;
	}
	// UAssetDefinition End
};

UCLASS()
class UAtomSoundfieldEndpointAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundfieldEndpoint", "Atom Soundfield Endpoint"); }
	virtual FLinearColor GetAssetColor() const override { return FColor(64, 192, 203); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundfieldEndpoint::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundRendererSubMenu", "Mixer") };
		return Categories;
	}
	// UAssetDefinition End
};
