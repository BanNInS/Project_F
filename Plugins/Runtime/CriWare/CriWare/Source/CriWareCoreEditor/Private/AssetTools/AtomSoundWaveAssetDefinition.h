
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundWave.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomSoundBaseAssetDefinition.h"

#include "AtomSoundWaveAssetDefinition.generated.h"

UCLASS()
class UAtomSoundWaveAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundWave", "Atom Sound Wave"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundWaveColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundWave::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };
		return Categories;
	}
	virtual void GetAssetActions(FToolMenuSection& Section) const override;
	virtual FAssetSupportResponse CanRename(const FAssetData& InAsset) const override { return FAssetSupportResponse::Supported(); }
	virtual FAssetSupportResponse CanDuplicate(const FAssetData& InAsset) const override { return FAssetSupportResponse::NotSupported(); }
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	virtual void GetAssetStatusInfo(const TSharedPtr<IAssetStatusInfoProvider>& InAssetStatusInfoProvider, TArray<FAssetDisplayInfo>& OutStatusInfo) const;
#else
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
#endif
	// UAssetDefinition End

	// UAtomAssetDefinition_Base Begin
	virtual EVisibility GetIconOverlayVisibility(const FAssetData& AssetData) const override { return EVisibility::Hidden; }
	// UAtomAssetDefinition_Base End

private:
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	EVisibility GetThumbnailLoopStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
#else
	static TSharedPtr<SWidget> GetSoundWaveThumbnailOverlay(const FAssetData& AssetData);
#endif
};
