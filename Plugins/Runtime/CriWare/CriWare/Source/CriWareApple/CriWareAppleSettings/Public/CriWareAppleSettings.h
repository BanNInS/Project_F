/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Module Settings for CriWareApple
* File     : CriWareAppleSettings.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareCoreSettings.h"

#include "CriWareAppleSettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAppleSettings, Log, All);

/**
 * Implements the settings for CriWare on Apple platform.
 */
UCLASS(config = CriWare, DefaultConfig, Abstract)
class CRIWAREAPPLESETTINGS_API UCriWareAppleSettings 
	: public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UCriWareAppleSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UObject interface
	virtual void PostReloadConfig(class FProperty* PropertyThatWasLoaded) override;

#if WITH_EDITOR
protected:

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
	//~ End UObject interface

public:
	/** Called whenever a registered Apple property changes. */
	//DECLARE_MULTICAST_DELEGATE_OneParam(FPropertyChanged, struct FPropertyChangedEvent&);
	//FPropertyChanged OnPropertyChanged;

#endif

public:
	//~ Apple

	/** */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Voices", meta = (
		DisplayName = "Global Voices Overrides"))
	FAtomVoicesSettings VoicesOverrides;

#if WITH_EDITORONLY_DATA
	UPROPERTY(config)
	FCriWareStandardVoicesSettings StandardVoicesOverrides_DEPRECATED;

	UPROPERTY(config)
	FCriWareHcaMxVoicesSettings HcaMxVoicesOverrides_DEPRECATED;
#endif

	/** Enable to override Asr Rack Configurations from common settings. */
	//UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
	//	DisplayName = "Override MasterRack",
	//	Tooltip = "Enable to override Asr Rack Configurations from common settings. This will not create more sound render than there is in common settings."))
	//bool bOverrideMasterRack;

	/** Apple platform Cook Overrides. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Cook Overrides", meta = (	
		Tooltip = "",
		ShowOnlyInnerProperties))
	FCriWareCookOverridesSettings CookOverrides;

	/** Which of the currently enabled source data override plugins to use. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Plugins", meta = (
		DisplayName = "Atom SourceDataOverridePlugin",
		Tooltip = " Which of the currently enabled source data override plugins to use."))
	FString AtomSourceDataOverridePlugin;
};
