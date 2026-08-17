#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomEffectPreset.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomEffectPresetAssetDefinition.generated.h"

UCLASS()
class UAtomBusEffectPresetAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomBusEffectPreset", "Bus Effect Preset"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultEffectPresetColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomBusEffectPreset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomEffectSubMenu", "Effects") };
		return Categories;
	}
	// UAssetDefinition End

	// UObject Begin
	virtual void PostCDOContruct() override;
	// UObject End
};

UCLASS()
class UAtomEffectPresetAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override { return EffectPreset->GetPresetColor(); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomEffectSubMenu", "Effects") };
		return Categories;
	}
	EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const;
protected:
	virtual bool CanRegisterStatically() const override;
	// UAssetDefinition End

public:
	void Initialize(UAtomEffectPreset* InEffectPreset);

private:
	TStrongObjectPtr<UAtomEffectPreset> EffectPreset;
};

/*UCLASS()
class UAtomEffectSourcePresetAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomEffectSourcePreset", "Effect Source Preset"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultEffectSourcePresetColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomEffectSourcePreset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomEffectSubMenu", "Effects") };
		return Categories;
	}
};*/

/*UCLASS()
class UAtomEffectSourcePresetChainAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomEffectSourcePresetChain", "Effect Source Preset Chain"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultEffectSourcePresetChainColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomEffectSourcePresetChain::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomEffectSubMenu", "Effects") };
		return Categories;
	}
};*/
