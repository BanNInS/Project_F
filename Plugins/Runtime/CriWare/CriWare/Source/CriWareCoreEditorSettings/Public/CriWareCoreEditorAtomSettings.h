/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditorSettings
 * File     : CriWareCoreEditorAtomSettings.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareCoreEditorAtomSettings.generated.h"

/**
 * Configure Atom settings for the Editor.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "ADX Atom"))
class CRIWARECOREEDITORSETTINGS_API UCriWareCoreEditorAtomSettings
	: public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UCriWareCoreEditorAtomSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("CriWare"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
	//~ End UDeveloperSettings interface

	/** Whether to pin the Atom Attenuation asset type when creating new assets. Requires editor restart to take effect. */
	UPROPERTY(EditAnywhere, config, Category = AssetMenu, meta = (DisplayName = "Pin Atom Attenuation In Asset Menu"))
	bool bPinAtomSoundAttenuationInAssetMenu = true;

	/** Whether to pin the Atom Concurrency asset type when creating new assets. Requires editor restart to take effect. */
	UPROPERTY(EditAnywhere, config, Category = AssetMenu, meta = (DisplayName = "Pin Atom Concurrency In Asset Menu"))
	bool bPinAtomSoundConcurrencyInAssetMenu = true;

	/** Whether to pin the Atom Sound Class asset type when creating new assets. Requires editor restart to take effect. */
	UPROPERTY(EditAnywhere, config, Category = AssetMenu)
	bool bPinAtomSoundClassInAssetMenu = true;

	/** Whether to pin the Atom Sound Simple asset type when creating new assets. Requires editor restart to take effect. */
	UPROPERTY(EditAnywhere, config, Category = AssetMenu)
	bool bPinAtomSoundSimpleInAssetMenu = false;

	/** Whether to pin the Atom Sound Proxy asset type when creating new assets. Requires editor restart to take effect. */
	UPROPERTY(EditAnywhere, config, Category = AssetMenu)
	bool bPinAtomSoundProxyInAssetMenu = true;

	/** If checked, will create a new Atom runtime instance for the play-in-editor window. Otherwise, will re-use the Atom runtime used in content browser. */
	UPROPERTY(EditAnywhere, config, Category = Atom, DisplayName = "Create New Atom Runtime For Play in Editor (Experimental)")
	uint32 bCreateNewAtomRuntimeForPlayInEditor:1;

	/** The number of additional handles to use when from Atom runtime used in content browser. Apply also to PIE if 'Create New Atom Runtime For Play In Editor' is not checked. */
	UPROPERTY(EditAnywhere, config, Category = Atom, DisplayName = "Number of Additional Player Handles")
	int32 NumOfAdditionalPlayerHandles;

	/** Whether to hide CriWare module icons over assets thumbanil in Content Browser. */
	UPROPERTY(EditAnywhere, config, Category = Appearance, DisplayName = "Hide CriWare Asset Thumbnail Icons")
	bool bHideAssetThumbnailIconOverlay = false;
};
