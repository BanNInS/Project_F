
#pragma once

#include "CoreMinimal.h"

#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"
#include "Atom/Modulation/AtomModulationGenerator.h"
#include "Atom/Modulation/AtomModulationPatch.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomModulationAssetDefinition.generated.h"

UCLASS()
class UAtomModulationControlBusAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomModulationControlBus", "Atom Control Bus"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultModulationControlBusColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomModulationControlBus::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomModulationControlBusSubMenu", "Modulations") };
		return Categories;
	}
	// UAssetDefinition End
};

UCLASS()
class UAtomModulationControlBusMixAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomModulationControlBusMix", "Atom Control Bus Mix"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultModulationControlBusMixColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomModulationControlBusMix::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomModulationControlMixSubMenu", "Modulations") };
		return Categories;
	}
	// UAssetDefinition End
};

UCLASS()
class UAtomModulationGeneratorAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomModulationGenerator", "Atom Modulation Generator"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultModulationGeneratorColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomModulationGenerator::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomModulationGeneratorSubMenu", "Modulations") };
		return Categories;
	}
	// UAssetDefinition End
};

UCLASS()
class UAtomModulationParameterAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomModulationParameter", "Atom Modulation Parameter"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultModulationParameterColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomModulationParameter::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomModulationParameterSubMenu", "Modulations") };
		return Categories;
	}
	// UAssetDefinition End
};

UCLASS()
class UAtomModulationPatchAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomModulationPatch", "Atom Modulation Parameter Patch"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultModulationPatchColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomModulationPatch::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomModulationPatchSubMenu", "Modulations") };
		return Categories;
	}
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	// UAssetDefinition End
};
