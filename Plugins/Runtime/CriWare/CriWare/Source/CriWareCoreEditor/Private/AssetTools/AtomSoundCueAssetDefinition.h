
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundCue.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomSoundBaseAssetDefinition.h"

#include "AtomSoundCueAssetDefinition.generated.h"

UCLASS()
class UAtomSoundCueAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundCue", "Atom Sound Cue"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundCueColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundCue::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };
		return Categories;
	}
	virtual void GetAssetActions(FToolMenuSection& Section) const override;
	virtual FAssetSupportResponse CanRename(const FAssetData& InAsset) const override { return FAssetSupportResponse::NotSupported(); }
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
	EVisibility GetThumbnailSound3DStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
	EVisibility GetThumbnailAisacStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
	EVisibility GetThumbnailLoopStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
	EVisibility GetThumbnailLockStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
	EVisibility GetThumbnailPrivateStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const;
#else
	static TSharedPtr<SWidget> GetSoundCueThumbnailOverlay(const FAssetData& AssetData);
#endif
};
