/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Module Settings for CriWare
* File     : CriWareCoreSettings.h
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

#include "CriWareWindowsSettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareWindowsSettings, Log, All);

USTRUCT()
struct FCriWareWASAPISettings
{
	GENERATED_BODY()

public:

	/** Use Windows Audio Session API (WASAPI) with Hardware Device 1 instead of Microsoft Spatial Sound. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		DisplayName = "Enable WASAPI for Hardware Device 1",
		Tooltip = "Use Windows Audio Session API (WASAPI) with Hardware Device 1 instead of Microsoft Spatial Sound."))
	bool bEnableHardware1 = false;

	/** Use Windows Audio Session API (WASAPI) with Hardware Device 2 instead of Microsoft Spatial Sound. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		DisplayName = "Enable WASAPI for Hardware Device 2",
		Tooltip = "Use Windows Audio Session API (WASAPI) with Hardware Device 2 instead of Microsoft Spatial Sound."))
	bool bEnableHardware2 = false;

	/** Use Windows Audio Session API (WASAPI) with Hardware Device 3 instead of Microsoft Spatial Sound. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		DisplayName = "Enable WASAPI for Hardware Device 3",
		Tooltip = "Use Windows Audio Session API (WASAPI) with Hardware Device 3 instead of Microsoft Spatial Sound."))
	bool bEnableHardware3 = false;

	/** Use Windows Audio Session API (WASAPI) with Hardware Device 4 instead of Microsoft Spatial Sound. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		DisplayName = "Enable WASAPI for Hardware Device 4",
		Tooltip = "Use Windows Audio Session API (WASAPI) with Hardware Device 4 instead of Microsoft Spatial Sound."))
	bool bEnableHardware4 = false;

	/** WASAPI is exclusive. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		DisplayName = "WASAPI Is Exclusive",
		Tooltip = "WASAPI is exclusive.",
		EditCondition = "bEnableHardware1 || bEnableHardware2 || bEnableHardware3 || bEnableHardware4"))
	bool bWASAPI_IsExclusive = false;

	/** WASAPI bits per sample. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		ClampMin = "0",
		Tooltip = "WASAPI bits per sample.",
		EditCondition = "(bEnableHardware1 || bEnableHardware2 || bEnableHardware3 || bEnableHardware4) && bWASAPI_IsExclusive"))
	int32 WASAPI_BitsPerSample = 24;

	/** WASAPI sampling rate. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		ClampMin = "0",
		Units = Hz,
		Tooltip = "WASAPI sampling rate.",
		EditCondition = "(bEnableHardware1 || bEnableHardware2 || bEnableHardware3 || bEnableHardware4) && bWASAPI_IsExclusive"))
	int32 WASAPI_SamplingRate = 48000;

	/** WASAPI number of output channels. */
	UPROPERTY(EditAnyWhere, Category = "WASAPI", meta = (
		ClampMin = "0",
		DisplayName = "WASAPI Number of Output Channels",
		Tooltip = "WASAPI number of output channels.",
		EditCondition = "(bEnableHardware1 || bEnableHardware2 || bEnableHardware3 || bEnableHardware4) && bWASAPI_IsExclusive"))
	int32 WASAPI_NumChannels = 2;
};

/**
 * Implements the settings for CriWare on Windows platform.
 */
UCLASS(config = CriWare, DefaultConfig, meta = (DisplayName = "ADX Atom Windows"))
class CRIWAREWINDOWSSETTINGS_API UCriWareWindowsSettings 
	: public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UCriWareWindowsSettings(const FObjectInitializer& ObjectInitializer);

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
	/** Called whenever a registered windows property changes. */
	//DECLARE_MULTICAST_DELEGATE_OneParam(FPropertyChanged, struct FPropertyChangedEvent&);
	//FPropertyChanged OnPropertyChanged;

#endif

public:
	//~ Windows

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

	/** */
	UPROPERTY(config, EditAnyWhere, Category = "Atom" , meta = (
		DisplayName = "WASAPI Settings"))
	FCriWareWASAPISettings WASAPISettings;

	/** Enable to override Asr Rack Configurations from common settings. */
	//UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
	//	DisplayName = "Override MasterRack",
	//	Tooltip = "Enable to override Asr Rack Configurations from common settings. This will not create more sound render than there is in common settings."))
	//bool bOverrideMasterRack;

	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		ArraySizeEnum = "EAtomSoundRendererType",
		GetOptions = GetHardwareDeviceOptions,
		DisplayName = "Endpoint Mappings"))
	FName EndpointMappings[(int32)EAtomSoundRendererType::Num];

	/** Hardware device 1 mapping. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Hardware Device 1",
		Tooltip = "Audio Device used as 'Hardware 1' endpoint."))
	FName Hardware1;

	/** Hardware device 2 mapping. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Hardware Device 2",
		Tooltip = "Audio Device used as 'Hardware 2' endpoint."))
	FName Hardware2;

	/** Hardware device 3 mapping. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Hardware Device 3",
		Tooltip = "Audio Device used as 'Hardware 3' endpoint."))
	FName Hardware3;

	/** Hardware device 4 mapping. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Hardware Device 4",
		Tooltip = "Audio Device used as 'Hardware 4' endpoint."))
	FName Hardware4;

	/** Windows platform Cook Overrides. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Cook Overrides", meta = (	
		Tooltip = "",
		ShowOnlyInnerProperties))
	FCriWareCookOverridesSettings CookOverrides;

	/** Which of the currently enabled source data override plugins to use. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Plugins", meta = (
		DisplayName = "Atom SourceDataOverridePlugin",
		Tooltip = " Which of the currently enabled source data override plugins to use."))
	FString AtomSourceDataOverridePlugin;

private:

	UFUNCTION()
	TArray<FString> GetHardwareDeviceOptions() const;
};
