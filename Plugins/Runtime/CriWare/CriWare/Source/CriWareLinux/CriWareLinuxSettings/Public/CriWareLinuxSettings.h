/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Linux Module Settings for CriWare
* File     : CriWareLinuxSettings.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareCoreSettings.h"
#include "Atom/Atom.h"

#include "CriWareLinuxSettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareLinuxSettings, Log, All);

/**
 * Implements the settings for CriWare on Linux platform.
 */
UCLASS(config = CriWare, DefaultConfig, meta = (DisplayName = "ADX Atom Linux"))
class CRIWARELINUXSETTINGS_API UCriWareLinuxSettings 
	: public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UCriWareLinuxSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("CriWare"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
	//~ End UDeveloperSettings interface

	//~ Begin UObject interface
	virtual void PostReloadConfig(class FProperty* PropertyThatWasLoaded) override;

#if WITH_EDITOR
private:

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
	//~ End UObject interface

public:
	/** Called whenever a registered Linux property changes. */
	//DECLARE_MULTICAST_DELEGATE_OneParam(FPropertyChanged, struct FPropertyChangedEvent&);
	//FPropertyChanged OnPropertyChanged;

#endif

public:
	//~ Linux

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

	/** Linux platform Cook Overrides. */
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
