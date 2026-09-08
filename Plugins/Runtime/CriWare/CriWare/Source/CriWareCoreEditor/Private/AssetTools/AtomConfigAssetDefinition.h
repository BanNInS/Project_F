
#pragma once

#include "CoreMinimal.h"
#include "EditorFramework/AssetImportData.h"

#include "Atom/AtomConfig.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomConfigAssetDefinition.generated.h"

UCLASS()
class UAtomConfigAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomConfig", "Atom Config"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultConfigColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomConfig::StaticClass(); }
	virtual bool CanImport() const { return true; }
	// UAssetDefinition End

	void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
	{
		for (auto& Asset : TypeAssets)
		{
			const auto AtomConfig = CastChecked<UAtomConfig>(Asset);
			if (AtomConfig->AssetImportData)
			{
				AtomConfig->AssetImportData->ExtractFilenames(OutSourceFilePaths);
			}
		}
	}
};

/**
 * Implements asset definiton for UAtomDspBusSetting preset assets held by UAtomConfig assets.
 */
UCLASS()
class UAtomDspBusSettingAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomDspBusSetting", "Atom Dsp Bus Setting"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultDspBusSettingColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomDspBusSetting::StaticClass(); }
	virtual FAssetSupportResponse CanRename(const FAssetData& InAsset) const override { return FAssetSupportResponse::NotSupported(); }
	virtual FAssetSupportResponse CanDuplicate(const FAssetData& InAsset) const override { return FAssetSupportResponse::NotSupported(); }
	// UAssetDefinition End
};
