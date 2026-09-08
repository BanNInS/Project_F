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

#include "UObject/Object.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareCore.h"

#include "CriWareCoreSettings.generated.h"

// Forward declarations
class UAtomConfig;
class UAtomSoundClass;
class UAtomConcurrency;

USTRUCT(BlueprintType)
struct FAtomVoicesSettings
{
	GENERATED_BODY()

public:

	/** The number of standard voices used by Atom. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Voices", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Number of Voices",
		Tooltip = "Specifies the number of voices that are reserved."))
	int32 NumVoices = 0;

	/** The number of channels used by a voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Voices", meta = (
		ClampMin = "0", ClampMax = "16",
		DisplayName = "Number of Channels",
		Tooltip = "Specifies the number of channels used by a voice."))
	int32 NumChannels = 0;

	/** The sampling rate used by a voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "384000", SliderExponent = "10.0",
		Units = Hz,
		Tooltip = "Specifies the sampling rate used by a voice."))
	int32 SamplingRate = 0;

	/** Overrides with other settings when value is valid. */
	CRIWARECORE_API void OverridesWith(const FAtomVoicesSettings& Settings);
};

USTRUCT()
struct FAtomHcaMxSettings
{
	GENERATED_BODY()

public:

	/** The maximum number of voices that can be used by HCA-MX decoding mixer. */
	UPROPERTY(config, EditAnyWhere, Category = "Hca-Mx", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Maximum Number of Voices",
		Tooltip = "Specifies the maximum number of voices that can be used by HCA-MX decoding mixer. A value of 0 disable the mixer."))
	int32 MaxVoices = 0;

	/** The maximum sampling rate that can be used by HCA-MX decoding mixer. */
	UPROPERTY(config, EditAnyWhere, Category = "Hca-Mx", meta = (
		ClampMin = "0", ClampMax = "240000", SliderExponent = "10.0",
		Units = Hz,
		DisplayName = "Maximum Sampling Rate",
		Tooltip = "Specifies the sampling rate that can be used by HCA-MX decoding mixer. A value of 0 use the default value."))
	int32 MaxSamplingRate = 0;

	/** The sampling rate output by HCA-MX decoding mixer. */
	UPROPERTY(config, EditAnyWhere, Category = "Hca-Mx", meta = (
		ClampMin = "0", ClampMax = "240000", SliderExponent = "10.0",
		Units = Hz,
		Tooltip = "Specifies the sampling rate output by HCA-MX decoding mixer. A value of 0 use the default value."))
	int32 OutputSamplingRate = 0;
};
 
USTRUCT(meta = (Deprecated = "FAtomStandardVoicesSettings have been decprecated in 2.4, please use FAtomVoicesSettings."))
struct FCriWareStandardVoicesSettings
{
	GENERATED_BODY()

public:

	/** The number of voices on standard memory used by Atom. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Number of Standard Memory Voices",
		Tooltip = "Specifies the number of voices on standard memeory that are reserved by Atom. (ADX/HCA)"))
	int32 NumStandardMemoryVoices = 0;

	/** The number of channels used by a standard memory voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "16",
		DisplayName = "Number of Channels for Standard Memory Voice",
		Tooltip = "Specifies the number of channels used by a standard memory voice."))
	int32 StandardMemoryVoiceNumChannels = 0;

	/** The sampling rate used by a standard memory voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "384000",
		Units = Hz,
		Tooltip = "Specifies the sampling rate used by a standard memory voice."))
	int32 StandardMemoryVoiceSamplingRate = 0;

	/** The number of standard streaming voices used by Atom. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Number of Standard Streaming Voices",
		Tooltip = "Specifies the number of voices that are reserved for standard streaming."))
	int32 NumStandardStreamingVoices = 0;

	/** The number of channels used by a standard streaming voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "16",
		DisplayName = "Number of Channels for Standard Streaming Voice",
		Tooltip = "Specifies the number of channels used by a standard streaming voice."))
	int32 StandardStreamingVoiceNumChannels = 0;

	/** The sampling rate used by a standard streaming voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "Standard Voices", meta = (
		ClampMin = "0", ClampMax = "384000",
		Units = Hz,
		Tooltip = "Specifies the sampling rate used by a standard streaming voice."))
	int32 StandardStreamingVoiceSamplingRate = 0;

	/** Overrides with other settings when value is valid. */
	CRIWARECORE_API void OverridesWith(const FCriWareStandardVoicesSettings& Settings);
};

USTRUCT(meta = (Deprecated = "Global HCA-MX voices have been decprecated in 2.4 due to playback limitations."))
struct FCriWareHcaMxVoicesSettings
{
	GENERATED_BODY()

public:

	/** The number of voices on HCA-MX memory used by Atom. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "HCA-MX Voices", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Number of HCA-MX Memory Voices",
		Tooltip = "Specifies the number of voices on HCA-MX memory that are reserved by Atom."))
	int32 NumHcaMxMemoryVoices = 0;

	/** The number of channels used by a HCA-MX memory voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "HCA-MX Voices", meta = (
		ClampMin = "0", ClampMax = "16",
		DisplayName = "Number of Channels for HCA-MX Memory Voices",
		Tooltip = "Specifies the number of channels used by a HCA-MX memory voice."))
	int32 HcaMxMemoryVoiceNumChannels = 0;

	/** The number of HCA-MX streaming voices used by Atom. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "HCA-MX Voices", meta = (
		ClampMin = "0", ClampMax = "32767", UIMin = "0", UIMax = "128",
		DisplayName = "Number of HCA-MX Streaming Voices",
		Tooltip = "Specifies the number of voices that are reserved for HCA-MX streaming."))
	int32 NumHcaMxStreamingVoices = 0;

	/** The number of channels used by a HCA-MX streaming voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "HCA-MX Voices", meta = (
		ClampMin = "0", ClampMax = "16",
		DisplayName = "Number of Channels for HCA-MX Streaming Voice",
		Tooltip = "Specifies the number of channels used by a HCA-MX streaming voice."))
	int32 HcaMxStreamingVoiceNumChannels = 0;

	/** The sampling rate used by any HCA-MX voice. (0 means "do not override".) */
	UPROPERTY(config, EditAnyWhere, Category = "HCA-MX Voices", meta = (
		ClampMin = "0", ClampMax = "384000",
		Units = Hz,
		DisplayName = "HCA-MX Voice Sampling Rate",
		Tooltip = "Specifies the sampling rate used by any HCA-MX voice."))
	int32 HcaMxVoiceSamplingRate = 0;

	/** Overrides with other settings when value is valid. (0 means "do not override".) */
	CRIWARECORE_API void OverridesWith(const FCriWareHcaMxVoicesSettings& Settings);
};

USTRUCT()
struct FCriWareCookOverridesSettings
{
	GENERATED_BODY()

public:

	/**  */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Cook Overrides", meta = (
		ClampMin = "0",
		Tooltip = "NOT USED YET - If this is enabled any Atom wave data will be separated from the corresponding asset and loaded into a cache (CueSheets use Atom D-Bas while WaveBanks will use a dedicated cache.)."))
	bool bUseAtomStreamCaching = false;

	/**  */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Cook Overrides", meta = (
		ClampMin = "0",
		Tooltip = "NOT USED YET - If this is enabled any Mana movie data will be separated from the corresponding asset and loaded into a cache."))
	bool bUseManaStreamCaching = false;
};

USTRUCT()
struct CRIWARECORE_API FAtomSoundDebugEntry
{
	GENERATED_BODY()

	/** Short name to use when referencing sound (ex. in the command line) */
	UPROPERTY(config, EditAnywhere, Category = "Debug", meta = (DisplayName = "Name"))
	FName DebugName;

	/** Reference to a Debug Sound */
	UPROPERTY(config, EditAnywhere, Category = "Debug", meta = (AllowedClasses = "/Script/CriWareCore.AtomSoundBase"))
	FSoftObjectPath Sound;
};

USTRUCT()
struct FDefaultAtomAudioBusSettings
{
	GENERATED_BODY()

	/** The audio bus to start up by default on init. */
	UPROPERTY(EditAnywhere, Category = "Sound Renderer", meta = (AllowedClasses = "/Script/CriWareCore.AtomAudioBus"))
	FSoftObjectPath AudioBus;
};

UCLASS(config = CriWare, DefaultConfig, meta = (DisplayName = "ADX Atom"))
class CRIWARECORE_API UCriWareCoreSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UCriWareCoreSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("CriWare"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
	//~ End UDeveloperSettings interface

	//~ Begin UObject interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	
	/** Event to listen for when settings reflected properties are changed. */
	DECLARE_EVENT(UCriWareCoreSettings, FCriWareCoreSettingsChanged)
#endif
	//~ End UObject interface

public:
	//~ File System Settings
	/** Whether FileSytem binders and loaders are allocated dynamically (recommanded) or use fixed settings. */
	UPROPERTY(Config, EditAnyWhere, Category = "FileSystem", meta = (
		DisplayName = "Use Dynamic File System Management",
		Tooltip = "Whether FileSytem binders and loaders are allocated dynamically (recommanded) or use fixed settings."))
	bool bUseAutomaticFileSystemManagement;

	/** Number of binders. */
	UPROPERTY(Config, EditAnyWhere, Category = "FileSystem", meta = (
		EditCondition = "!bUseAutomaticFileSystemManagement",
		ClampMin = "5",
		DisplayName = "Number of Binders",
		Tooltip = "Number of binders. 1 binder is used per resource to read data from Unreal archives. (Note: 4 binders are reserved by the resource manager.)"))
	int32 NumBinders;

	/** Maximum number of bindings. */
	UPROPERTY(config, EditAnyWhere, Category = "FileSystem", meta = (
		EditCondition = "!bUseAutomaticFileSystemManagement",
		ClampMin = "4",
		DisplayName = "Maximum Number of Bindings",
		Tooltip = "Maximum number of binders that can access data at same time."))
	int32 MaxBindings;

	/** Number of loaders. */
	UPROPERTY(config, EditAnyWhere, Category = "FileSystem", meta = (
		EditCondition = "!bUseAutomaticFileSystemManagement",
		ClampMin = "1",
		DisplayName = "Number of Loaders",
		Tooltip = "Number of loaders. 1 Loader is used per voice of a streamed voice pool, also 1 loader is used per resource."))
	int32 NumLoaders;

	/** Enable logging of File System. */
	UPROPERTY(config, EditAnyWhere, Category = "FileSystem", meta = (
		Tooltip = "Enable logging of File System."))
	bool bEnableLoggingFileSystem;

public:
	//~ Atom Settings

	/** Atom configuration preset. (ACF) */
	UPROPERTY(config, EditAnywhere, Category = "Atom", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomConfig",
		Tooltip = "Atom configuration preset. (ACF)"))
	FSoftObjectPath AtomConfig;

	/** Enable In-Game-Preview functionality. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		Tooltip = "Enable In-Game-Preview functionality."))
	bool bUseInGamePreview;

	/** The buffer size used to communicate between Atom Monitor and CRI Atom Craft tool. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		ClampMin = "2097152", // 2*1024*1024
		Units = B,
		Tooltip = "Specifies the buffer size used to communicate between Atom Monitor and CRI Atom Craft tool."))
	int32 MonitorCommunicationBufferSize;

	/** The maximum value for the pitch change. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		ClampMin = "0",
		DisplayName = "Maximum Pitch Value (cents)",
		Tooltip = "Specifies the maximum value for the pitch change in Cents."))
	int32 MaxPitch;

	/** The distance factor to use with Atom. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		ClampMin = "0",
		Tooltip = "Specifies the distance factor to use with Atom."))
	float DistanceFactor;

	/** The maximum number of sounds that Atom can play at same time. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		ClampMin = "0", UIMin = "0",
		ClampMax = "32767", UIMax = "1024",
		DisplayName = "Maximum Sound Sources",
		Tooltip = "Specifies the maximum number of sounds that Atom can play at same time. This will be added to the Num Reserved sounds."))
	int32 MaxSoundSources;

	/** The number of additional sounds sources reserved that Atom can play at same time. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom", meta = (
		ClampMin = "0", UIMin = "0",
		ClampMax = "32767", UIMax = "32",
		DisplayName = "Number of Sound Sources to reserve.",
		Tooltip = "Specifies the number of additional sounds to reserve. Atom stops sounds with a fast DSP fade-out, this reserve will avoid abrubt stop when all sources are used."))
	int32 NumReservedSoundSources;

	/** Enable logging of Atom. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Debug", meta = (
		Tooltip = "Enable logging of Atom."))
	bool bEnableLoggingAtom;

	/**
	 * Sounds only packaged in non-shipped builds for debugging.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Debug")
	TArray<FAtomSoundDebugEntry> DebugSounds;

	//~ Sound Renderer (ASR)

	/** The Master rack through which all sounds are routed. */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sound Renderer", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomRack",
		Tooltip = "Specifies the master atom sound renderer rack through which all sounds are routed by default."))
	FSoftObjectPath MasterRack;

	/** The rack through which spatialized sounds are routed. */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sound Renderer", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomSoundfieldRack, /Script/CriWareCore.AtomSoundfieldEndpointRack, /Script/CriWareCore.AtomRack",
		Tooltip = "Specifies the Atom soundfield rack to use for sound routed to binaural spatialization."))
	FSoftObjectPath SpatializationRack;

	/** If enabled, Atom will use HRTF encoding over any spatial soundfield racks at initialization. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Enable Binaural Spatialization",
		Tooltip = "If enabled, Atom will use HRTF encoding over any spatial soundfield racks at initialization."))
	bool bEnableBinauralSpatialization;

	/** If enabled, Atom can use sound form Unreal submixes and audio sources. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Use Audio Link (Experimental)",
		Tooltip = "If enabled, Unreal can route submixes and audio sources output to Atom sound renderer as sources or to buses. (You need to restart the engine to apply.)"))
	bool bUseAudioLink;

	/** If enabled, Atom can send sound sources and buses to Unreal Audio submixes. */
	//UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
	//	DisplayName = "Use Atom Link (Experimental)",
	//	Tooltip = "If enabled, Atom can route sources and buses output to Unreal Audio submixes."))
	//bool bUseAtomLink;

	/** If enabled, Atom will route all sounds to Unreal Audio. */
	UPROPERTY(config, EditAnyWhere, Category = "Atom|Sound Renderer", meta = (
		DisplayName = "Use Unreal Audio Sound Renderer (Experimental)",
		Tooltip = "If enabled, Atom will route all sounds to Unreal Audio. (You need to restart the engine to apply.)"))
	bool bUseUnrealSoundRenderer;

	/** The Unreal submix through which Atom sound is routed. */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sound Renderer", meta = (
		EditCondition = "bUseUnrealSoundRenderer",
		DisplayName = "Unreal Submix To Route",
		AllowedClasses = "/Script/Engine.SoundSubmix",
		Tooltip = "Specifies the Unreal submix to route Atom sound output to. If unset, falls back to the 'Master Submix' as set in the 'Audio' category of Project Settings'."))
	FSoftObjectPath DefaultOutputSubmix;

	/** Enables the surround sound spatialization calculations to include the center channel. */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sound Renderer", AdvancedDisplay)
	uint32 bAllowCenterChannel3DPanning : 1;

	/** Array of AudioBuses that are automatically initialized when the AudioEngine is initialized */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sound Renderer")
	TArray<FDefaultAtomAudioBusSettings> DefaultAudioBuses;

	//~ Sound Management

	/** The SoundClass assigned to newly created sounds */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sounds", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomSoundClass",
		DisplayName = "Default Sound Class"))
	FSoftObjectPath DefaultSoundClassName;

	/** The SoundClass assigned to Mana player assets */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sounds", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomSoundClass",
		DisplayName = "Default Mana Sound Class",
		EditCondition = "bCanEditManaClassName",
		EditConditionHides, HideEditConditionToggle))
	FSoftObjectPath DefaultManaSoundClassName;

	/** The SoundConcurrency assigned to newly created sounds */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Sounds", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomConcurrency",
		DisplayName = "Default Sound Concurrency"))
	FSoftObjectPath DefaultSoundConcurrencyName;

	//~ Voices Management

	UPROPERTY(config, EditAnyWhere, Category = "Atom|Voices", meta = (
		DisplayName = "Force Dynamic Voice Management For Sound Cues",
		Tooltip = "Whether force dynamic voice management for all sound cues or use fixed defined voices by default."))
	bool bUseAutomaticVoiceManagement;

	UPROPERTY(config, EditAnyWhere, Category = "Atom|Voices", meta = (
		DisplayName = "Global Voice Pool Settings",
		Tooltip = "The voice settings for sounds that do not use automated voice management.",
		EditCondition = "!bUseAutomaticVoiceManagement"))
	FAtomVoicesSettings VoicesSettings;

#if WITH_EDITORONLY_DATA
	UPROPERTY(config, meta = (DeprecatedProperty, DeprecatedMessage = "StandardVoicesSettings have been decprecated in 2.4, please use VoicesSettings."))
	FCriWareStandardVoicesSettings StandardVoicesSettings_DEPRECATED;

	UPROPERTY(config, meta = (DeprecatedProperty, DeprecatedMessage = "Global HCA-MX voices have been decprecated in 2.4 due to playback limitations."))
	FCriWareHcaMxVoicesSettings HcaMxVoicesSettings_DEPRECATED;
#endif

	UPROPERTY(config, EditAnyWhere, Category = "Atom|Decoding", meta = (
		DisplayName = "Enable HCA-MX Decoding",
		Tooltip = "Whether to enable HCA-MX format decoding in Atom."))
	bool bEnableHcaMxDecoding;

	UPROPERTY(config, EditAnyWhere, Category = "Atom|Decoding", meta = (
		Tooltip = "Whether to enable HCA-MX format decoding in Atom.",
		EditCondition = "bEnableHcaMxDecoding",
		EditConditionHides))
	FAtomHcaMxSettings HcaMxSettings;

	/** Array of Atom Modulation Parameters that are loaded on plugin startup. */
	UPROPERTY(config, EditAnywhere, Category = "Atom|Modulation", meta = (
		AllowedClasses = "/Script/CriWareCore.AtomModulationParameter",
		Tooltip = "Array of Atom Modulation Parameters that are loaded on plugin startup."))
	TArray<FSoftObjectPath> ModulationParameters;

	// todo add global threading settings HERE

#if WITH_EDITOR
	FCriWareCoreSettingsChanged CriWareCoreSettingsChanged;
#endif // WITH_EDITOR

public:

#if WITH_EDITOR
	/** Ask to user if we want to apply some ACF settings. return false if no. */
	bool ShowApplyAtomConfigToMasterRackDialog(const UAtomConfig* InAtomConfig, bool& bOutWasCanceled);

	/** Returns event to be bound to if caller wants to know when settings are modified */
	FCriWareCoreSettingsChanged& OnCriWareCoreSettingsChanged() { return CriWareCoreSettingsChanged; }

	/** Reactivates current active Atom runtime with current settings. */
	void ReactivateActiveRuntime() const;
#endif // WITH_EDITOR

	/** */
	void LoadDefaultObjects();

	/** Registers to global list all default modulation parameters defined in settings. */
	void RegisterModulationParameters() const;

	UAtomSoundClass* GetDefaultSoundClass() const;
	UAtomSoundClass* GetDefaultManaSoundClass() const;
	UAtomConcurrency* GetDefaultSoundConcurrency() const;

private:
	
	UPROPERTY(Transient)
	TObjectPtr<UAtomSoundClass> DefaultSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<UAtomSoundClass> DefaultManaSoundClass;

	UPROPERTY(Transient)
	TObjectPtr<UAtomConcurrency> DefaultSoundConcurrency;

	UPROPERTY(Transient, NonTransactional)
	bool bCanEditManaClassName;

private:

#if WITH_EDITOR
	FSoftObjectPath CachedMasterRack;
	FSoftObjectPath CachedSpatializationRack;
	FSoftObjectPath CachedAtomConfig;
	FSoftObjectPath CachedSoundClass;
#endif // WITH_EDITOR

private:

	UPROPERTY(config)
	int32 MaxSoundPlaybackHandles_DEPRECATED = 0;
};
