#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomCueSheet.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomCueSheetAssetDefinition.generated.h"

UCLASS()
class UAtomCueSheetAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomCueSheet", "Atom Cue Sheet"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultCueSheetColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomCueSheet::StaticClass(); }
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	virtual bool CanImport() const { return true; }
	// UAssetDefinition End

	void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const;
};
