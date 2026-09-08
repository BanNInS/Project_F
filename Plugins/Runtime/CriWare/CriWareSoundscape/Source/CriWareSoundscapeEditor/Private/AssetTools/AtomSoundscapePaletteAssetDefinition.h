
#pragma once

#include "CoreMinimal.h"

#include "AtomSoundscapePalette.h"
#include "AssetTools/AtomSoundscapeAssetDefinition_Base.h"

#include "AtomSoundscapePaletteAssetDefinition.generated.h"

UCLASS()
class UAtomSoundscapePaletteAssetDefinition : public UAtomSoundscapeAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SoundscapePalette", "Soundscape Palette"); }
	virtual FLinearColor GetAssetColor() const override { return FColor(0, 125, 255); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundscapePalette::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundscapeSubMenu", "Soundscape") };
		return Categories;
	}
	// UAssetDefinition End
};
