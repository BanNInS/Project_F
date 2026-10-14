
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomConcurrency.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomConcurrencyAssetDefinition.generated.h"

UCLASS()
class UAtomConcurrencyAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomConcurrency", "Atom Concurrency"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultConcurrencyColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomConcurrency::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto PinnedCategories = { Super::GetAssetCategories().Last() };
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomConcurrencySubMenu", "Mix") };

		if (GetDefault<UCriWareCoreEditorAtomSettings>()->bPinAtomSoundConcurrencyInAssetMenu)
		{
			return PinnedCategories;
		}
		return Categories;
	}
	// UAssetDefinition End
};
