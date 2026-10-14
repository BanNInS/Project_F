
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomWaveBank.h"
#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomWaveBankAssetDefinition.generated.h"

UCLASS()
class UAtomWaveBankAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomWaveBank", "Atom Wave Bank"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultWaveBankColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomWaveBank::StaticClass(); }
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	virtual bool CanImport() const { return true; }
	// UAssetDefinition End

	void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const;
};
