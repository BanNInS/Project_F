


#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomRackAssetDefinition.generated.h"

UCLASS()
class UAtomRackAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomRack", "Atom Rack"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultRackColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomRack::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSoundRendererSubMenu", "Sound Renderer") };
		return Categories;
	}
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	virtual EAssetCommandResult ActivateAssets(const FAssetActivateArgs& ActivateArgs) const override;
	// UAssetDefinition End
};

UCLASS()
class UAtomSoundfieldRackAssetDefinition : public UAtomRackAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundfieldRack", "Atom Soundfield Rack"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::SoundfieldDefaultRackColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundfieldRack::StaticClass(); }
	// UAssetDefinition End
};

UCLASS()
class UAtomEndpointRackAssetDefinition : public UAtomRackAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomEndpointRack", "Atom Endpoint Rack"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::EndpointDefaultRackColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomEndpointRack::StaticClass(); }
	// UAssetDefinition End
};

UCLASS()
class UAtomSoundfieldEndpointRackAssetDefinition : public UAtomRackAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundfieldEndpointRack", "Atom Soundfield Endpoint Rack"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::SoundfieldEndpointDefaultRackColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundfieldEndpointRack::StaticClass(); }
	// UAssetDefinition End
};

UCLASS()
class UAtomBusAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomBus", "Atom Bus"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultBusColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomBus::StaticClass(); }
	// UAssetDefinition End
};
