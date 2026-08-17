
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundSimple.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomSoundBaseAssetDefinition.h"
#include "CriWareCoreEditorAtomSettings.h" 

#include "AtomSoundSimpleAssetDefinition.generated.h"

UCLASS()
class UAtomSoundSimpleAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundSimple", "Atom Sound Simple"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundSimpleColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundSimple::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto PinnedCategories = { Super::GetAssetCategories().Last() };
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };
		
		if (GetDefault<UCriWareCoreEditorAtomSettings>()->bPinAtomSoundSimpleInAssetMenu)
		{
			return PinnedCategories;
		}
		return Categories;
	}
	// UAssetDefinition End
};
