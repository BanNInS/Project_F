
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundClass.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomSoundClassAssetDefinition.generated.h"

UCLASS()
class UAtomSoundClassAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundClass", "Atom Sound Class"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundClassColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundClass::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto PinnedCategories = { Super::GetAssetCategories().Last() };
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundClassSubMenu", "Classes") };

		if (GetDefault<UCriWareCoreEditorAtomSettings>()->bPinAtomSoundClassInAssetMenu)
		{
			return PinnedCategories;
		}
		return Categories;
	}
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	// UAssetDefinition End
};
