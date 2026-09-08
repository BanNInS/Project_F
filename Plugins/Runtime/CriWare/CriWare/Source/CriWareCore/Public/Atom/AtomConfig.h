/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomConfig.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "Serialization/BulkData.h"
#include "Runtime/Launch/Resources/Version.h"

#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/AtomParameter.h"

#if WITH_EDITOR
#include "CriWareApi.h"
#endif

#include "AtomConfig.generated.h"

 /**
  * FAtomBusInfo struct
  */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a Bus send."))
struct FAtomBusSendInfo
{
	GENERATED_BODY()

public:
	FAtomBusSendInfo();
	FAtomBusSendInfo(const FString InSendToBusName, float InLevel, EAtomBusSendType InSendType);

	bool operator==(const FAtomBusSendInfo& Other) const
	{
		return Other.SendToBusName == SendToBusName 
			&& Other.Level == Level
			&& Other.SendType == Other.SendType;
	}
	bool operator!=(const FAtomBusSendInfo& Other) const { return !(*this == Other); }

	/** Bus name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the send to bus."))
	FString SendToBusName;

	/** Bus volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Level of the send."))
	float Level;

	/** Bus volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Attach type of the send."))
	EAtomBusSendType SendType;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Information for a Bus effect."))
struct FAtomBusEffectInfo
{
	GENERATED_BODY()

public:
	FAtomBusEffectInfo();
	FAtomBusEffectInfo(const FString InBusEffectName);

	bool operator==(const FAtomBusEffectInfo& Other) const
	{
		return Other.BusEffectName == BusEffectName
			&& Other.Parameters == Parameters
			&& Other.bBypass == bBypass;
	}
	bool operator!=(const FAtomBusEffectInfo& Other) const { return !(*this == Other); }

	/** Bus effect name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the send to bus."))
	FString BusEffectName;

	/** Bus effect parameters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Float paramters of the bus effect."))
	TArray<float> Parameters;

	/** Bus effect bypass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Bypass value of the bus effect."))
	bool bBypass;
};

 /**
  * FAtomBusInfo struct
  */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a Bus setting."))
struct FAtomBusInfo
{
	GENERATED_BODY()

public:

	FAtomBusInfo();
	FAtomBusInfo(const FString& InName, float InVolume, const FAtomPanning& InPanning, 
		EAtomSpeakerChannelMap InChannelMap, EAtomSoundRendererType InOutputType, EAtomSoundfieldRendererType SoundfieldType, int8 InOutputOptions);

	bool operator==(const FAtomBusInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomBusInfo& Other) const { return !(*this == Other); }

	/** Bus name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the bus."))	
	FString Name;

	/** Bus volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Volume of the bus."))
	float Volume;

	/** Bus pan settings. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Pan settings of the bus."))
	FAtomPanning Panning;

	/** Bus sends. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "List of sends of the bus."))
	TArray<FAtomBusSendInfo> Sends;

	/** Bus effects. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "List of effects of the bus."))
	TArray<FAtomBusEffectInfo> Effects;

	/* Mixer2 - Speaker channel map  CriAtomSpeakerMapping speaker_mapping;*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The default speaker mapping the bus."))
	EAtomSpeakerChannelMap SpeakerChannelMap;

	/* Mixer2 - Sound Renderer output type. CriAtomExAcfDspBusOutputType output_type; */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The default output end point of the bus."))
	EAtomSoundRendererType SoundRendererType;

	/* Mixer2 - Soundfield encoding type if not channel based. CriAtomSpeakerMapping speaker_mapping;*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The default speaker mapping the bus."))
	EAtomSoundfieldRendererType SoundfieldRendererType;
	
	/* Mixer2 - Additional output options of the bus from Atom Craft. CriUint8 output_options; */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Ouput options of the bus."))
	uint8 OutputOptions;
};

/**
 * FAtomDspBusSettingSnapshotInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a Dsp bus setting snapshot."))
struct FAtomDspBusSettingSnapshotInfo
{
	GENERATED_BODY()

public:

	FAtomDspBusSettingSnapshotInfo();
	FAtomDspBusSettingSnapshotInfo(const FString& Name);

	bool operator==(const FAtomDspBusSettingSnapshotInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomDspBusSettingSnapshotInfo& Other) const { return !(*this == Other); }

	/** Setting name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the DSP bus setting."))
	FString Name;

	/** Bus list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Bus list."))
	TArray<FAtomBusInfo> Buses;

	/** Extended bus list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Extended bus list."))
	TArray<FAtomBusInfo> ExtendBuses;
};

 /**
  * FAtomDSPSettingInfo struct
  */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a DSP setting. A DSP settings defines bus layout and applied DSP effects for a sound renderer in AtomRack."))
struct FAtomDSPSettingInfo
{
	GENERATED_BODY()

public:

	FAtomDSPSettingInfo();
	FAtomDSPSettingInfo(const FString& InName, int32 InNumMixerAisacs);

	bool operator==(const FAtomDSPSettingInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomDSPSettingInfo& Other) const { return !(*this == Other); }

	/** DSP setting name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the category."))
	FString Name;

	/** Bus list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Bus list."))
	TArray<FAtomBusInfo> Buses;

	/** Extended bus list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Extended bus list."))
	TArray<FAtomBusInfo> ExtendBuses;

	/** Setting snapsshot list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Setting snapshot list"))
	TArray<FAtomDspBusSettingSnapshotInfo> Snapshots;

	/** Group of this category. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Group number where this category belong to."))
	int32 NumMixerAisacs;
};

/**
 * FAtomAisacControlInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a sound category."))
struct FAtomAisacControlInfo
{
	GENERATED_BODY()

public:

	FAtomAisacControlInfo();
	FAtomAisacControlInfo(const FString& InName, int32 InID);

	bool operator==(const FAtomAisacControlInfo& Other) const { return Other.ID == ID; }
	bool operator!=(const FAtomAisacControlInfo& Other) const { return !(*this == Other); }

	/** AISAC control name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the AISAC control."))
	FString Name;

	/** AISAC control ID. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Identifier of the AISAC control."))
	int32 ID;
};

/**
 * FAtomCategoryInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a sound category."))
struct FAtomCategoryInfo
{
	GENERATED_BODY()

public:

	FAtomCategoryInfo();
	FAtomCategoryInfo(const FString& InName, int32 InID, int32 InGroupNumber, float InVolume, bool bInIsPaused);

	bool operator==(const FAtomCategoryInfo& Other) const { return Other.ID == ID; }
	bool operator!=(const FAtomCategoryInfo& Other) const { return !(*this == Other); }

	/** Category name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the category."))
	FString Name;

	/** Category ID. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Identifier of the category."))
	int32 ID;

	/** Group of this category. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Group number where this category belong to."))
	int32 GroupNumber;

	/** Category current volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Current volume used by this category."))
	float Volume;

	/** Is category paused. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "True if this category is paused."))
	bool bIsPaused;
};

/**
 * FAtomSelectorInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a selector."))
struct FAtomSelectorInfo
{
	GENERATED_BODY()

public:

	FAtomSelectorInfo();
	FAtomSelectorInfo(const FString& InName);

	bool operator==(const FAtomSelectorInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomSelectorInfo& Other) const { return !(*this == Other); }

	/** Selector Name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The selector name."))
	FString Name;

	/** Selector Labels */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The list of labels contained in this selector."))
	TArray<FString> Labels;
};

/** 
 * FAtomGameVariableInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for a game variable."))
struct FAtomGameVariableInfo
{
	GENERATED_BODY()

public:

	FAtomGameVariableInfo();
	FAtomGameVariableInfo(const FString& InName, int32 InID, float InValue);

	bool operator==(const FAtomGameVariableInfo& Other) const { return ID == Other.ID; }
	bool operator!=(const FAtomGameVariableInfo& Other) const { return !(*this == Other); }

	/** Name of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The game variable name."))
	FString Name;

	/** Unique Atom Craft ID of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Identifier of the game variable."))
	int32 ID;

	/** Unique Atom Craft default value of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The default value of the game variable."))
	int32 Value;
};

/**
 * FAtomGameVariableInfo struct
 */
USTRUCT(BlueprintType)
struct FAtomOutputPortInfo
{
	GENERATED_BODY()

public:

	FAtomOutputPortInfo();
	FAtomOutputPortInfo(const FString& InName);

	bool operator==(const FAtomOutputPortInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomOutputPortInfo& Other) const { return !(*this == Other); }

	/** Name of the Output Port. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The name of the output port."))
	FString Name;
};

/**
 * FAtomACFInfo struct
 */
USTRUCT(BlueprintType, meta = (ToolTip = "Information for ACF info."))
struct FAtomAcfInfo
{
	GENERATED_BODY()

public:

	FAtomAcfInfo();

	bool operator==(const FAtomAcfInfo& Other) const { return Other.Name == Name; }
	bool operator!=(const FAtomAcfInfo& Other) const { return !(*this == Other); }

	/** ACF setting name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Name of the configuration."))
	FString Name;

	/** Category list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Category list."))
	TArray<FAtomCategoryInfo> Categories;

	/** AISAC Controls list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "AISAC Controls list."))
	TArray<FAtomAisacControlInfo> AisacControls;

	/** Selector list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Selector list."))
	TArray<FAtomSelectorInfo> Selectors;

	/** Game Variable list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Game Variable list."))
	TArray<FAtomGameVariableInfo> GameVariables;

	/** Output Port list */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "Output Port list."))
	TArray<FAtomOutputPortInfo> OutputPorts;

	UPROPERTY()
	uint32 ACFVersion = 0;
};

/**
 * UAtomDspBusSetting Class
 * 
 * that can be set to an AtomRack 
 * Keep only DSP setting name and tom busse.
 * Other information reflected from ACF data (buses and effects) are editor only data.
 */
UCLASS(HideCategories = Object, MinimalAPI, BlueprintType)
class UAtomDspBusSetting
	: public UObject
{
	GENERATED_BODY()

public:

	UAtomDspBusSetting(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	void Init(const CriAtomExAcfDspSettingInfo& InSettingInfo);
#endif

	//~ Begin UObject interface
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostRename(UObject* OldOuter, FName OldName) override;
	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	//~ End UOBject interface

	/** Gets the Parent holder config of this Mixer setting. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Mixer")
	UAtomConfig* GetConfig() const
	{
		return Cast<UAtomConfig>(GetOuter());
	}

	/** */
	FORCEINLINE const FString& GetSettingName() const { return Name; }

	/** */
	FORCEINLINE int GetNumBuses() const { return BusNames.Num(); }

	/** */
	FORCEINLINE const FString& GetBusName(int Index) const
	{ 
		static const FString EmptyStr = FString(); 
		return BusNames.IsValidIndex(Index) ? BusNames[Index] : EmptyStr;
	}

	/** */
	FORCEINLINE bool IsValidBusName(const FString& InName) const { return BusNames.Contains(InName); }

	/** */
	FORCEINLINE int GetNumSnapshots() const { return SnapshotNames.Num(); }

	/** */
	FORCEINLINE const FString& GetSnapshotName(int Index) const
	{
		static const FString EmptyStr = FString();
		return SnapshotNames.IsValidIndex(Index) ? SnapshotNames[Index] : EmptyStr;
	}

	/** */
	FORCEINLINE bool IsValidSnapshotName(const FString& InName) const { return SnapshotNames.Contains(InName); }

	/** */
	FORCEINLINE int GetNumBusEffects(int BusIndex) const { return BusEffectNames.IsValidIndex(BusIndex) ? BusEffectNames[BusIndex].Num() : 0; }

	/** */
	FORCEINLINE FName GetBusEffectName(int BusIndex, int Index) const { return BusEffectNames.IsValidIndex(BusIndex) && BusEffectNames[BusIndex].IsValidIndex(Index) ? BusEffectNames[BusIndex][Index] : NAME_None; }

	/** */
	FORCEINLINE bool IsValidBusEffectName(int BusIndex, const FName& InName) const { return BusEffectNames.IsValidIndex(BusIndex) && BusEffectNames[BusIndex].Contains(InName); }

#if WITH_EDITORONLY_DATA
	/** C++ access to cached dsp bus setting information for editor. */
	FORCEINLINE const FAtomDSPSettingInfo& GetEditorDspSetting() const { return DspSetting; }
#endif

protected:

	/** The DSP setting name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DSP Bus Setting")
	FString Name;

	/** The bus name list. Premit to spawn UAtomBus instances in UAtomRack when DSPSettings are updated dynamically. */
	UPROPERTY()
	TArray<FString> BusNames;

	/** The snapshot name list. Premit to list and check possible value in UAtomRack.*/
	UPROPERTY()
	TArray<FString> SnapshotNames;

	/** The effect name list by buses. Premit to list effect in UAtomBus and setup effect parameters values. */
	TArray<TArray<FName>> BusEffectNames;

#if WITH_EDITORONLY_DATA
	/** The full settings reflection */
	UPROPERTY(VisibleAnywhere, Category = "DSP Bus Setting")
	FAtomDSPSettingInfo DspSetting;
#endif

private:

	UPROPERTY()
	FString IdentifierName;

#if WITH_EDITOR
	static void GetDspBusEndPointInfoFromAcfDspBusInfo(
		const CriAtomExAcfDspBusInfo& InBusInfo,
		EAtomSpeakerChannelMap& OutSpeakerChannelMap,
		EAtomSoundRendererType& OutSoundRendererType,
		EAtomSoundfieldRendererType& OutSoundfieldRendererType);
#endif
};

/** Struct that define a Game Variable. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomGameVariable
{
	GENERATED_BODY()

public:

	FAtomGameVariable()
		: ID(INDEX_NONE)
		, Value(0.0f)
	{}

	FAtomGameVariable(FName InName, int32 InID, float InValue)
		: Name(InName)
		, ID(InID)
		, Value(InValue)
	{}

	bool operator==(const FAtomGameVariable& Other) const
	{
		return ID == Other.ID;
	}

	bool operator!=(const FAtomGameVariable& Other) const
	{
		return !(*this == Other);
	}

	/** Name of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Variable")
	FName Name;

	/** Unique Atom Craft ID of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Variable")
	int32 ID;

	/** Unique Atom Craft default value of the Game Variable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Variable")
	int32 Value;
};

inline uint32 GetTypeHash(FAtomGameVariable A) { return static_cast<uint32>(A.ID); }

/** Struct that define an Output Port. */
USTRUCT(BlueprintType)
struct FAtomOutputPort
{
	GENERATED_BODY()

public:

	FAtomOutputPort()
		: Name(NAME_None)
		, TargetRack(nullptr)
	{}

	FAtomOutputPort(FName InName)
		: Name(InName)
		, TargetRack(nullptr)
	{}

	bool operator==(const FAtomOutputPort& Other) const
	{
		return Other.Name == Name;
	}
	
	bool operator!=(const FAtomOutputPort& Other) const
	{
		return !(*this == Other);
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rack Configuration")
	FName Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration")
	TSoftObjectPtr<UAtomRackWithParentBase> TargetRack;
};

/**
 * FAtomRackConfig Struct
 */
USTRUCT(BlueprintType)
struct FAtomRackConfig
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (InlineEditConditionToggle))
	uint8 bOverrideMasterRack : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (InlineEditConditionToggle))
	uint8 bOverrideSpatializationRack : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (DisplayAfter = "SpatializationRackOverrides"))
	uint8 bOverrideDefaultOutputPorts : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (EditCondition = "bOverrideMasterRack", DisplayName = "Master Rack Overrides"))
	TSoftObjectPtr<UAtomRack> MasterRack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (EditCondition = "bOverrideSpatializationRack", DisplayName = "Spatialization Rack Overrides"))
	TSoftObjectPtr<UAtomRackWithParentBase> SpatializationRack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, Category = "Rack Configuration")
	TArray<FAtomOutputPort> OutputPorts;

	FAtomRackConfig()
		: bOverrideMasterRack(false)
		, bOverrideSpatializationRack(false)
		, bOverrideDefaultOutputPorts(false)
		, MasterRack(nullptr)
		, SpatializationRack(nullptr)
	{}

	void Update(const TArray<FName>& OuputPortNames)
	{
		// add new ports
		for (const FName& Name : OuputPortNames)
		{
			if (!OutputPorts.ContainsByPredicate([Name](const FAtomOutputPort& Port) { return Port.Name == Name; }))
			{
				OutputPorts.Add({ Name });
			}
		}

		// remove old ports
		TArray<FAtomOutputPort> PortsToRemove;
		for (const FAtomOutputPort& Port : OutputPorts)
		{
			if (!OuputPortNames.Contains(Port.Name))
			{
				PortsToRemove.Add(Port);
			}
		}
		for (FAtomOutputPort& PortToRemove : PortsToRemove)
		{
			OutputPorts.Remove(PortToRemove);
		}

		// alphabetically ordered
		OutputPorts.Sort([](const FAtomOutputPort& First, const FAtomOutputPort& Second)
		{
			return !First.Name.LexicalLess(Second.Name);
		});
	}
};

/**
 * UAtomConfig Class 
 */
UCLASS(hidecategories = object, meta = (ToolTip = "Asset holding Atom ConFiguration (ACF) data."))
class CRIWARECORE_API UAtomConfig 
	: public UObject
{
	GENERATED_BODY()

public:

	UAtomConfig(const FObjectInitializer& ObjectInitializer);

	/** Reload Properties Settings that are acessible in Acf Data  */
	bool LoadSettings();

	/** Get the default DspBusSetting that is used for the master rack when creating Atom.*/
	UAtomDspBusSetting* GetDefaultDspBusSetting() const;

	/** */
	FORCEINLINE int32 GetNumDspBusSettings() const { return DspBusSettings.Num(); }

	/** */
	FORCEINLINE UAtomDspBusSetting* GetDspBusSetting(int Index) const { return DspBusSettings.IsValidIndex(Index) ? DspBusSettings[Index] : nullptr; }

	/** */
	FORCEINLINE UAtomDspBusSetting* GetDspBusSetting(const FString& InName) const {
		auto Setting = DspBusSettings.FindByPredicate([&](const UAtomDspBusSetting* Setting) { return Setting->GetSettingName() == InName; });
		return Setting ? *Setting : nullptr;
	}

	/** */
	FORCEINLINE bool IsValidDspBusSetting(const FString& InName) const {
		return DspBusSettings.ContainsByPredicate([&](const UAtomDspBusSetting* Setting) { return Setting->GetSettingName() == InName; });
	}

	/** */
	FORCEINLINE int GetNumGlobalAisacs() const { return GlobalAisacPatches.Num(); }

	/** */
	FORCEINLINE FName GetGlobalAisacName(int Index) const { return GlobalAisacPatches.IsValidIndex(Index) ? GlobalAisacPatches[Index]->Name : NAME_None; }
	FORCEINLINE UAtomAisacPatch* GetGlobalAisac(int Index) const { return GlobalAisacPatches.IsValidIndex(Index) ? GlobalAisacPatches[Index] : nullptr; }

	/** */
	FORCEINLINE bool IsValidGlobalAisacName(const FName& InName) const { 
		return GlobalAisacPatches.ContainsByPredicate([&](auto Patch) { return Patch->Name == InName; });
	}

	/** */
	FORCEINLINE int GetNumAisacControls() const { return AisacControls.Num(); }

	/** */
	FORCEINLINE FName GetAisacControlName(int Index) const { return AisacControls.IsValidIndex(Index) ? AisacControls[Index].Name : NAME_None; }
	FORCEINLINE FAtomAisacControl GetAisacControl(int Index) const { return AisacControls.IsValidIndex(Index) ? AisacControls[Index] : FAtomAisacControl(); }
	
	/** */
	FORCEINLINE const FAtomAisacControl* GetAisacControlByID(int InID) const {
		return AisacControls.FindByPredicate([&](auto& Control) { return Control.ID == InID; });
	}

	/** */
	FORCEINLINE bool IsValidAisacControlName(const FName& InName) const {
		return AisacControls.ContainsByPredicate([&](auto& Control) { return Control.Name == InName; });
	}

	/** */
	FORCEINLINE int GetNumCategories() const { return CategorieNames.Num(); }

	/** */
	FORCEINLINE FName GetCategorieName(int Index) const { return CategorieNames.IsValidIndex(Index) ? CategorieNames[Index] : NAME_None; }

	/** */
	FORCEINLINE bool IsValidCategorieName(const FName& InName) const { return CategorieNames.Contains(InName); }

	/** */
	FORCEINLINE int GetNumSelectors() const { return Selectors.Num(); }

	/** */
	FORCEINLINE FName GetSelectorName(int Index) const { return Selectors.IsValidIndex(Index) ? Selectors[Index].GetName() : NAME_None; }
	FORCEINLINE const FAtomSelector* GetSelector(int Index) const { return Selectors.IsValidIndex(Index) ? &Selectors[Index] : nullptr; }
	FORCEINLINE const FAtomSelector* GetSelector(FName InName) const {
		return Selectors.FindByPredicate([&](auto& Selector) { return Selector.GetName() == InName; });
	}

	/** */
	FORCEINLINE bool IsValidSelectorName(const FName& InName) const {
		return Selectors.ContainsByPredicate([&](auto& Selector) { return Selector.GetName() == InName; });
	}

	/** */
	FORCEINLINE int GetNumGameVariables() const { return GameVariables.Num(); }

	/** */
	FORCEINLINE FName GetGameVariableName(int Index) const { return GameVariables.IsValidIndex(Index) ? GameVariables[Index].Name : NAME_None; }
	FORCEINLINE FAtomGameVariable GetGameVariable(int Index) const { return GameVariables.IsValidIndex(Index) ? GameVariables[Index] : FAtomGameVariable(); }

	/** */
	FORCEINLINE bool IsValidGameVariableName(const FName& InName) const {
		return GameVariables.ContainsByPredicate([&](auto& GameVariable) { return GameVariable.Name == InName; });
	}

	/** */
	FORCEINLINE FName GetBinauralizerName() const { return BinauralizerName; }

	/** */
	FORCEINLINE int GetNumOutputPorts() const { return OutputPortNames.Num(); }
	
	/** */
	FORCEINLINE FName GetOutputPortName(int Index) const { return OutputPortNames.IsValidIndex(Index) ? OutputPortNames[Index] : NAME_None; }

	/** */
	FORCEINLINE bool IsValidOutputPortName(const FName& InName) const { return OutputPortNames.Contains(InName); }

#if WITH_EDITORONLY_DATA
	/** */
	FORCEINLINE uint32 GetACFVersion() const { return AcfInfo.ACFVersion; }
#endif

	//~ Begin UObject interface.
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostRename(UObject* OldOuter, FName OldName) override;
	virtual void PreDuplicate(FObjectDuplicationParameters& DupParams) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	//~ End UObject interface.

	//~ Begin AssetUserData Interface
#if WITH_EDITORONLY_DATA
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
#endif
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
#endif
	//~ End AssetUserData Interface

protected:

	/** Available Atom category name list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	TArray<FName> CategorieNames;

	/** Available global AISAC patches. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	TArray<TObjectPtr<UAtomAisacPatch>> GlobalAisacPatches;

	/** Available Aisac controls. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	TArray<FAtomAisacControl> AisacControls;

	/** Available Cue selectors. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	TArray<FAtomSelector> Selectors;

	/** Available Atom game variables. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	TArray<FAtomGameVariable> GameVariables;

	/** Available DSP bus settings for Atom racks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Atom Configuration")
	TArray<TObjectPtr<UAtomDspBusSetting>> DspBusSettings;

	int DefaultDspBusSettingIndex;

	/** Name of the default HRTF binauralizer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom Configuration")
	FName BinauralizerName;

	/** Output ports used. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "Atom Configuration", AdvancedDisplay)
	TArray<FName> OutputPortNames;

#if WITH_EDITORONLY_DATA
	/** The ACF info reflection for editor. */
	UPROPERTY()
	FAtomAcfInfo AcfInfo;
#endif

public:

	/** The Atom rack mapping to use when this Atom configuration is loaded in Atom runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rack Configuration", meta = (ShowInnerProperties, DisplayAfter = "OutputPortNames"))
	FAtomRackConfig RackConfig;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Instanced, Category = Reimport)
	TObjectPtr<UAssetImportData> AssetImportData;
#endif

	/** ACF raw data */
	FByteBulkData RawData;
};
