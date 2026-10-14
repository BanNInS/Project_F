#pragma once

#include "CoreMinimal.h"

#include "Extensions/IAtomEndpoint.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomEndpointSettingsAssetDefinition.generated.h"

UCLASS()
class UAtomEndpointSettingsAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomEndpointSettings", "Atom Endpoint Settings"); }
	virtual FLinearColor GetAssetColor() const override { return FColor(100, 100, 100); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomEndpointSettingsBase::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundRendererSubMenu", "Sound Renderer") };
		return Categories;
	}
	// UAssetDefinition End
};
