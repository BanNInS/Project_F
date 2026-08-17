
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundSourceBus.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomSoundBaseAssetDefinition.h"

#include "AtomSoundSourceBusAssetDefinition.generated.h"

UCLASS()
class UAtomSoundSourceBusAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundSourceBus", "Atom Source Bus"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundSourceBusColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundSourceBus::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };
		return Categories;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override { return UAtomAssetDefinition_Base::GetThumbnailOverlay(InAssetData); }
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override { return false; }
#endif
	// UAssetDefinition End
};
