
#pragma once

#include "CoreMinimal.h"

#include "AtomSoundscapeColor.h"
#include "AssetTools/AtomSoundscapeAssetDefinition_Base.h"

#include "AtomSoundscapeColorAssetDefinition.generated.h"

UCLASS()
class UAtomSoundscapeColorAssetDefinition : public UAtomSoundscapeAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundscapeColor", "Soundscape Color"); }
	virtual FLinearColor GetAssetColor() const override { return FColor(0, 175, 175); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundscapeColor::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundscapeSubMenu", "Soundscape") };
		return Categories;
	}
	// UAssetDefinition End
};
