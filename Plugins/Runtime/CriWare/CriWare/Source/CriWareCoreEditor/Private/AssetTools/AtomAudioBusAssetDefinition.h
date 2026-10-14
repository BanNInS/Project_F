
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomAudioBus.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomAudioBusAssetDefinition.generated.h"

UCLASS()
class UAtomAudioBusAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomAudioBus", "Atom Audio Bus"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultAudioBusColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomAudioBus::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundRendererSubMenu", "Sound Renderer") };
		return Categories;
	}
	// UAssetDefinition End
};
