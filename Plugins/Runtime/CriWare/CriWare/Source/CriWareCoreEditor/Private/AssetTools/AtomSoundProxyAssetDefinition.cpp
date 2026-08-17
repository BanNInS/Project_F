/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundProxyAssetDefintions.cpp
 *
 ****************************************************************************/

#include "AtomSoundProxyAssetDefinition.h"

#include "AssetDefinitionRegistry.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundProxy.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
#include "IAssetStatusInfoProvider.h"
#include "AssetStatusAssetDataInfoProvider.h"
#endif

#define LOCTEXT_NAMESPACE "AtomSoundProxyDefinition"

 /*
  * Atom Sound Proxy Asset Definition
  *****************************************************************************/

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
void UAtomSoundProxyAssetDefinition::GetAssetStatusInfo(const TSharedPtr<IAssetStatusInfoProvider>& InAssetStatusInfoProvider, TArray<FAssetDisplayInfo>& OutStatusInfo) const
{
	if (FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData(); AssetData.IsValid())
	{
		if (UAtomSoundProxy* SoundProxy = Cast<UAtomSoundProxy>(AssetData.GetAsset()))
		{
			if (UAtomSoundBase* Sound = SoundProxy->GetLeafSound())
			{
				if (const UAssetDefinition* AD = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(Sound->GetClass()))
				{
					const TSharedPtr<IAssetStatusInfoProvider> StatusInfoProvider = MakeShared<FAssetStatusAssetDataInfoProvider>(FAssetData(Sound));
					AD->GetAssetStatusInfo(StatusInfoProvider, OutStatusInfo); // derived sound base
				}
			}
		}
	}

	Super::GetAssetStatusInfo(InAssetStatusInfoProvider, OutStatusInfo); // sound base;
}
#else
TSharedPtr<SWidget> UAtomSoundProxyAssetDefinition::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	if (UAtomSoundProxy* SoundProxy = Cast<UAtomSoundProxy>(AssetData.GetAsset()))
	{
		if (UAtomSoundBase* Sound = SoundProxy->GetLeafSound())
		{
			if (const UAssetDefinition* AD = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(Sound->GetClass()))
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				return AD->GetThumbnailOverlay(FAssetData(Sound)); // derived sound base
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}
		}
	}

	return Super::GetThumbnailOverlay(AssetData); // sound base;
}
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
bool UAtomSoundProxyAssetDefinition::GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const
{
	Super::GetThumbnailActionOverlay(InAssetData, OutActionOverlayInfo); // sound base

	if (UAtomSoundProxy* SoundProxy = Cast<UAtomSoundProxy>(InAssetData.GetAsset()))
	{
		if (UAtomSoundBase* Sound = SoundProxy->GetLeafSound())
		{
			if (const UAssetDefinition* AD = UAssetDefinitionRegistry::Get()->GetAssetDefinitionForClass(Sound->GetClass()))
			{
				FAssetActionThumbnailOverlayInfo OutSoundOverlayInfo;
				AD->GetThumbnailActionOverlay(FAssetData(Sound), OutSoundOverlayInfo); // derived sound base

				OutActionOverlayInfo = OutSoundOverlayInfo;
			}
		}
	}

	return true;
}
#endif

EVisibility UAtomSoundProxyAssetDefinition::GetIconOverlayVisibility(const FAssetData& InAssetData) const
{
	if (UAtomSoundProxy* SoundProxy = Cast<UAtomSoundProxy>(InAssetData.GetAsset()))
	{
		if (UAtomSoundBase* Sound = SoundProxy->GetLeafSound())
		{
			return Cast<UAtomSoundCue>(Sound) || Cast<UAtomSoundWave>(Sound) ? EVisibility::Hidden : EVisibility::Visible;
		}
	}

	return EVisibility::Visible;
}

#undef LOCTEXT_NAMESPACE