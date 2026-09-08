
#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomSoundProxy.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomSoundBaseAssetDefinition.h"
#include "CriWareCoreEditorAtomSettings.h"

#include "AtomSoundProxyAssetDefinition.generated.h"

UCLASS()
class UAtomSoundProxyAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundProxy", "Atom Sound Proxy"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundProxyColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundProxy::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto PinnedCategories = { Super::GetAssetCategories().Last() };
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };

		if (GetDefault<UCriWareCoreEditorAtomSettings>()->bPinAtomSoundProxyInAssetMenu)
		{
			return PinnedCategories;
		}
		return Categories;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	virtual void GetAssetStatusInfo(const TSharedPtr<IAssetStatusInfoProvider>& InAssetStatusInfoProvider, TArray<FAssetDisplayInfo>& OutStatusInfo) const;
#else
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
#endif
	// UAssetDefinition End

	// UAtomAssetDefinition_Base Begin
	virtual EVisibility GetIconOverlayVisibility(const FAssetData& AssetData) const override;
	// UAtomAssetDefinition_Base End
};


UCLASS()
class UMySoundAssetDefinition : public UAtomSoundBaseAssetDefinition
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundWave", "My Sound"); }
	virtual FLinearColor GetAssetColor() const override { return FColor::Cyan; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UMySound::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { Super::GetAssetCategories().Last() / NSLOCTEXT("AssetTypeActions", "AssetAtomSourceSubMenu", "Source") };
		return Categories;
	}
	// UAssetDefinition End
};