
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomAisacPatch.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomAisacPatchAssetDefinition.generated.h"

UCLASS()
class UAtomAisacPatchAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomAisacPatch", "Atom AISAC Patch"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultAisacPatchColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomAisacPatch::StaticClass(); }
	// UAssetDefinition End
};
