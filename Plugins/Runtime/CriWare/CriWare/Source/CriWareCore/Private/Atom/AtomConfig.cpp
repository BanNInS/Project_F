/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomConfig.cpp
 *
 ****************************************************************************/

#include "Atom/AtomConfig.h"

#include "Serialization/CustomVersion.h"
#include "EditorFramework/AssetImportData.h"

#if WITH_EDITOR
#ifndef CRIWARE_UE_LE
#include "cri_atom_ex_acf_reader.h"
#else
#include "cri_le_atom_ex_acf_reader.h"
#endif
#include "CriWareMemory.h"
#endif

#include "CriWareCorePrivate.h"
#include "Atom/Atom.h"
#include "Atom/AtomCustomVersion.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomConfiguration"

 /*
  * FAtomBusSendInfo struct
  *****************************************************************************/

FAtomBusSendInfo::FAtomBusSendInfo()
	: SendToBusName()
	, Level(0.0f)
	, SendType(EAtomBusSendType::PostPanning)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomBusSendInfo::FAtomBusSendInfo(const FString InSendToBusName, float InLevel, EAtomBusSendType InSendType)
	: SendToBusName(InSendToBusName)
	, Level(InLevel)
	, SendType(InSendType)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomBusEffectInfo struct
 *****************************************************************************/

FAtomBusEffectInfo::FAtomBusEffectInfo()
	: BusEffectName()
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomBusEffectInfo::FAtomBusEffectInfo(const FString InBusEffectName)
	: BusEffectName(InBusEffectName)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

 /*
  * FAtomBusInfo struct
  *****************************************************************************/

FAtomBusInfo::FAtomBusInfo()
	: Name()
	, Volume(0.0f)
	, Panning()
	, SpeakerChannelMap(EAtomSpeakerChannelMap::Main)
	, SoundRendererType(EAtomSoundRendererType::Default)
	, SoundfieldRendererType(EAtomSoundfieldRendererType::Default)
	, OutputOptions(0)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomBusInfo::FAtomBusInfo(const FString& InName, float InVolume, const FAtomPanning& InPanning, 
	EAtomSpeakerChannelMap InChannelMap, EAtomSoundRendererType InSoundType, EAtomSoundfieldRendererType InSoundfieldType, int8 InOutputOptions)
	: Name(InName)
	, Volume(InVolume)
	, Panning(InPanning)
	, SpeakerChannelMap(InChannelMap)
	, SoundRendererType(InSoundType)
	, SoundfieldRendererType(InSoundfieldType)
	, OutputOptions(InOutputOptions)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomDSPSettingInfo struct
 *****************************************************************************/

FAtomDSPSettingInfo::FAtomDSPSettingInfo()
	: Name()
	, NumMixerAisacs(0)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomDSPSettingInfo::FAtomDSPSettingInfo(const FString& InName, int32 InNumMixerAisacs)
	: Name(InName)
	, NumMixerAisacs(InNumMixerAisacs)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomAisacControlInfo struct
 *****************************************************************************/

FAtomAisacControlInfo::FAtomAisacControlInfo()
	: Name()
	, ID(INDEX_NONE)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomAisacControlInfo::FAtomAisacControlInfo(const FString& InName, int32 InID)
	: Name(InName)
	, ID(InID)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomCategoryInfo struct
 *****************************************************************************/

FAtomCategoryInfo::FAtomCategoryInfo()
	: Name()
	, ID(INDEX_NONE)
	, GroupNumber(0)
	, Volume(0.0f)
	, bIsPaused(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomCategoryInfo::FAtomCategoryInfo(const FString& InName, int32 InID, int32 InGroupNumber, float InVolume, bool bInIsPaused)
	: Name(InName)
	, ID(InID)
	, GroupNumber(InGroupNumber)
	, Volume(InVolume)
	, bIsPaused(bInIsPaused)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomSelectorInfo struct
 *****************************************************************************/

FAtomSelectorInfo::FAtomSelectorInfo()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomSelectorInfo::FAtomSelectorInfo(const FString& InName)
	: Name(InName)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomGameVariableInfo struct
 *****************************************************************************/

FAtomGameVariableInfo::FAtomGameVariableInfo()
	: ID(INDEX_NONE)
	, Value(0.0f)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomGameVariableInfo::FAtomGameVariableInfo(const FString& InName, int32 InID, float InValue)
	: Name(InName)
	, ID(InID)
	, Value(InValue)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomGameVariableInfo struct
 *****************************************************************************/

FAtomOutputPortInfo::FAtomOutputPortInfo()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomOutputPortInfo::FAtomOutputPortInfo(const FString& InName)
	: Name(InName)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomAcfInfo struct
 *****************************************************************************/

FAtomAcfInfo::FAtomAcfInfo()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * FAtomDspBusSettingSnapshotInfo struct
 *****************************************************************************/

FAtomDspBusSettingSnapshotInfo::FAtomDspBusSettingSnapshotInfo()
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

FAtomDspBusSettingSnapshotInfo::FAtomDspBusSettingSnapshotInfo(const FString& InName)
	: Name(InName)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomDspBusSetting class
 *****************************************************************************/

UAtomDspBusSetting::UAtomDspBusSetting(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

#if WITH_EDITOR

static EAtomBusSendType GetAtomBusSendType(CriAtomExAcfDspBusLinkType LinkType)
{
	switch (LinkType)
	{
	case CRIATOMEXACF_DSP_BUS_LINK_TYPE_PRE_VOLUME: return EAtomBusSendType::PreVolume;
	case CRIATOMEXACF_DSP_BUS_LINK_TYPE_POST_VOLUME: return EAtomBusSendType::PostVolume;
	case CRIATOMEXACF_DSP_BUS_LINK_TYPE_POST_PAN:  return EAtomBusSendType::PostPanning;
	default: return EAtomBusSendType::PostPanning;
	}
}

void UAtomDspBusSetting::Init(const CriAtomExAcfDspSettingInfo& InSettingInfo)
{
	BusNames.Reset();
	SnapshotNames.Reset();
	BusEffectNames.Reset();

	FString NameStr(UTF8_TO_TCHAR(InSettingInfo.name));
	Name = NameStr;

	DspSetting.Name = NameStr;
	DspSetting.NumMixerAisacs = InSettingInfo.num_mixer_aisacs;
	
	// Common function to grab bus information from acf data
	auto CacheBusInfo = [&](const CriUint16 SettingBusIndexes[], CriUint16 BusIndex, FAtomBusInfo& NewBusInfo)
	{
		CriAtomExAcfDspBusInfo BusInfo;
		criAtomExAcfReader_GetDspBusInformation((CriUint16)SettingBusIndexes[BusIndex], &BusInfo);

		FString BusNameStr(UTF8_TO_TCHAR(BusInfo.name));
		EAtomSpeakerChannelMap SpeakerChannelMap;
		EAtomSoundRendererType SoundRendererType;
		EAtomSoundfieldRendererType SoundfieldRendererType;
		GetDspBusEndPointInfoFromAcfDspBusInfo(BusInfo, SpeakerChannelMap, SoundRendererType, SoundfieldRendererType);
		NewBusInfo = { BusNameStr, BusInfo.volume,
			FAtomPanning(BusInfo.pan3d_volume, BusInfo.pan3d_angle, BusInfo.pan3d_distance, BusInfo.pan3d_wideness, BusInfo.pan3d_spread),
			SpeakerChannelMap, SoundRendererType, SoundfieldRendererType, (int8)BusInfo.output_options
		};

		// Sends
		for (int SendIndex = 0; SendIndex < BusInfo.num_bus_links; ++SendIndex)
		{
			const int LinkIndex = BusInfo.bus_link_indexes[SendIndex];

			CriAtomExAcfDspBusLinkInfo BusLinkInfo;
			criAtomExAcfReader_GetDspBusLinkInformation((CriUint16)LinkIndex, &BusLinkInfo);

			CriAtomExAcfDspBusInfo SendToBusInfo;
			criAtomExAcfReader_GetDspBusInformation((CriUint16)SettingBusIndexes[BusLinkInfo.bus_no], &SendToBusInfo);

			FString SendToBusNameStr(UTF8_TO_TCHAR(SendToBusInfo.name));

			NewBusInfo.Sends.Emplace(SendToBusNameStr, BusLinkInfo.send_level, GetAtomBusSendType(BusLinkInfo.type));
		}

		// Effects
		TArray<FName> EffectNames;
		for (int EffectIndex = 0; EffectIndex < BusInfo.num_fxes; ++EffectIndex)
		{
			const int FxIndex = BusInfo.fx_indexes[EffectIndex];

			// the effect name identifier
			FString FxNameStr(UTF8_TO_TCHAR(criAtomExAcfReader_GetDspFxName(FxIndex)));
			FName BusEffectName = FName(FxNameStr);

			/*FString FxName = FxNameStr;
			FString PluginName, EffectName;
			if (FxName.Split(TEXT("/"), &PluginName, &EffectName))
			{
				// generate each effect parameters info here.  
				//IAtomEffectInfo NewEffectInfo = CreateEffetInfo(PluginName, EffectName);
			}*/

			int NewIndex = NewBusInfo.Effects.Emplace(FAtomBusEffectInfo(FxNameStr));
			
			if (NewIndex != INDEX_NONE)
			{
				auto& NewEffectInfo = NewBusInfo.Effects[NewIndex];

				const int ParamSize = UAtomBusEffectPreset::GetBusEffectPresetNumParameters(BusEffectName);
				NewEffectInfo.Parameters.AddUninitialized(ParamSize);

				CriSint32 Size = sizeof(float) * ParamSize;
				CriBool DspBypass;
				criAtomExAcfReader_GetDspFxParameters(FxIndex, NewEffectInfo.Parameters.GetData(), Size, &DspBypass);
				NewEffectInfo.bBypass = DspBypass == CRI_TRUE;
			}

			EffectNames.Emplace(BusEffectName);
		}

		BusEffectNames.Emplace(EffectNames);
	};

	const int NumBuses = InSettingInfo.num_buses;
	const int NumExtendBuses = InSettingInfo.num_extend_buses;

	// Buses
	DspSetting.Buses.Empty();
	for (int BusIndex = 0; BusIndex < NumBuses; ++BusIndex)
	{
		FAtomBusInfo NewBusInfo;
		CacheBusInfo(InSettingInfo.bus_indexes, BusIndex, NewBusInfo);

		// add to editor data
		DspSetting.Buses.Add(NewBusInfo);

		// add to bus names
		BusNames.Add(NewBusInfo.Name);
	}

	// Extend buses
	DspSetting.ExtendBuses.Empty();
	for (int BusIndex = 0; BusIndex < NumExtendBuses; ++BusIndex)
	{
		FAtomBusInfo NewBusInfo;
		CacheBusInfo(InSettingInfo.extend_bus_indexes, BusIndex, NewBusInfo);

		DspSetting.ExtendBuses.Add(NewBusInfo);
	}

	// Snapshots
	for (int SnapshotIndex = 0; SnapshotIndex < InSettingInfo.num_snapshots; ++SnapshotIndex)
	{
		CriAtomExAcfDspSettingSnapshotInfo SnapshotInfo;
		criAtomExAcfReader_GetDspSettingSnapshotInformation(InSettingInfo.snapshot_start_index + (CriUint16)SnapshotIndex, &SnapshotInfo);

		FString SnapshotNameStr(UTF8_TO_TCHAR(SnapshotInfo.name));
		FAtomDspBusSettingSnapshotInfo NewSnapshotInfo = { SnapshotNameStr };

		const int NumSnapshotBuses = SnapshotInfo.num_buses;
		const int NumSnapshotExtendBuses = SnapshotInfo.num_extend_buses;

		// Buses
		NewSnapshotInfo.Buses.Empty();
		for (int BusIndex = 0; BusIndex < NumSnapshotBuses; ++BusIndex)
		{
			FAtomBusInfo NewBusInfo;
			CacheBusInfo(SnapshotInfo.bus_indexes, BusIndex, NewBusInfo);

			// add to editor data
			NewSnapshotInfo.Buses.Add(NewBusInfo);
		}

		// Extend buses
		NewSnapshotInfo.ExtendBuses.Empty();
		for (int BusIndex = 0; BusIndex < NumSnapshotExtendBuses; ++BusIndex)
		{
			FAtomBusInfo NewBusInfo;
			CacheBusInfo(SnapshotInfo.extend_bus_indexes, BusIndex, NewBusInfo);

			NewSnapshotInfo.ExtendBuses.Add(NewBusInfo);
		}

		DspSetting.Snapshots.Add(NewSnapshotInfo);

		// add to snapshot names
		SnapshotNames.Add(SnapshotNameStr);
	}

///
// CriUint16 bus_indexes[CRIATOMEXACF_MAX_BUSES]; 		/* DSP bus index array */
// CriUint16 extend_bus_indexes[CRIATOMEXACF_MAX_BUSES];/* DSP extension bus index array */
// CriUint16 snapshot_start_index; 						/* snapshot start index */
// CriUint8 num_buses; 									/* Number of effective DSP buses */
// CriUint8 num_extend_buses; 							/* Number of effective extended DSP buses */
// CriUint16 num_snapshots; 							/* number of snapshots */
// CriUint16 snapshot_work_size; 						/* work area size for snapshot */
// CriUint16 num_mixer_aisacs; 							/* Number of mixer AISACs */
// CriUint16 mixer_aisac_start_index; 					/* Mixer AISAC start index */
///

///
// const CriChar8* name;								/* name */
// CriFloat32 volume;									/* volume */
// CriFloat32 pan3d_volume;								/* Pan3D volume */
// CriFloat32 pan3d_angle;								/* Pan3D angle */
// CriFloat32 pan3d_distance;							/* Pan3D interior distance */
// CriFloat32 pan3d_spread;								/* Pan3D spread */
// CriUint16 fx_indexes[CRIATOMEXACF_MAX_FXES];			/* DSP FX index array */
// CriUint16 bus_link_indexes[CRIATOMEXACF_MAX_BUSES];	/* DSP bus link index array */
// CriUint16 bus_no;									/* DSP bus number in settings */
// CriUint8 num_fxes;									/* DSP FX number */
// CriUint8 num_bus_links;								/* Number of DSP bus links */
///

/// CriAtomExAcfInfo
// const CriChar8* name;								/* name */
// CriUint32 size;										/* size */
// CriUint32 version;									/* ACB version */
// CriAtomExCharacterEncoding character_encoding;		/* Character code */
// CriSint32 num_dsp_settings;							/* Number of DSP settings */
// CriSint32 num_categories;							/* number of categories */
// CriSint32 num_categories_per_playback;				/* Number of category views per playback */
// CriSint32 num_reacts;								/* number of REACTs */
// CriSint32 num_aisac_controls;						/* Number of AISAC controls */
// CriSint32 num_global_aisacs;							/* Global AISAC number */
// CriSint32 num_game_variables;						/* number of game variables */
// CriSint32 max_buses_of_dsp_bus_settings;				/* Maximum number of buses in DSP settings */
// CriSint32 num_buses;									/* number of buses */
// CriSint32 num_voice_limit_groups;					/* Number of voice limit groups */
// CriSint32 num_output_ports;							/* Number of output ports */
///
}
#endif

void UAtomDspBusSetting::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	//Ar.UsingCustomVersion(FAtomDspCustomVersion::GUID);

	// Serialize Effects
	Ar << BusEffectNames;
}

void UAtomDspBusSetting::PostRename(UObject* OldOuter, FName OldName)
{
	Super::PostRename(OldOuter, OldName);
}

void UAtomDspBusSetting::PreDuplicate(FObjectDuplicationParameters& DupParams)
{
	//Super::PreDuplicate(DupParams);
}

void UAtomDspBusSetting::PostDuplicate(bool bDuplicateForPIE)
{
	//Super::PostDuplicate(bDuplicateForPIE);
}

// Custom serialization version for AtomConfig asset
struct FAtomConfigCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
	};

	// The GUID for this custom version number
	static const FGuid GUID;
};

const FGuid FAtomConfigCustomVersion::GUID(0x5E386D6A, 0xA41D11E7, 0xB7FFF832, 0xE49EFF0B);
// Register the custom version with core
FCustomVersionRegistration GRegisterAtomConfigCustomVersion(FAtomConfigCustomVersion::GUID, FAtomConfigCustomVersion::LatestVersion, TEXT("AtomConfigVer"));

/*
 * UAtomConfig class
 *****************************************************************************/

UAtomConfig::UAtomConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DefaultDspBusSettingIndex(0)
	, BinauralizerName(NAME_None)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

/*
TArray<FAtomCategoryInfo> UAtomConfig::GetCurrentCategoryInfos()
{
	TArray<FAtomCategoryInfo> Result;
	
	int32 NumCategories = FCriWareApi::criAtomExAcf_GetNumCategories();
	for (int32 i = 0; i < NumCategories; ++i)
	{
		CriAtomExCategoryInfo info;
		if (FCriWareApi::criAtomExAcf_GetCategoryInfo(i, &info) == CRI_TRUE)
		{
			auto Str = FString(TEXT("hello"));
			FAtomCategoryInfo Info2 = { Str, 0, 0, 10.0f, true };

			FAtomCategoryInfo Info = { FString(info.name), (int32)info.id, (int32)info.group_no, info.volume, (criAtomExCategory_IsPausedByName(info.name) == CRI_TRUE) };
			Result.Add(Info);
		}
	}

	return Result;
}*/

bool UAtomConfig::LoadSettings()
{
	const void* AcfData = RawData.LockReadOnly();
	if (AcfData)
	{
#if WITH_EDITOR

		// Check acf compatibilty
		FUintVector2 RangeVersions = FAtomRuntime::GetAllowedACFVersions();
		CriBool bIsDataValid;
		uint32 ACFDataVersion = (uint32)criAtomExAcfReader_GetVersionFromData(static_cast<CriUint8*>(const_cast<void*>(AcfData)), RawData.GetBulkDataSize(), &bIsDataValid);
		if (!bIsDataValid || !FMath::IsWithinInclusive(ACFDataVersion, RangeVersions.X, RangeVersions.Y))
		{
			RawData.Unlock();
			return false;
		}

		// Load acf data to reader
		size_t WorkSize = criAtomExAcfReader_CalculateWorkSize();
		void* Work = FMemory::Malloc(WorkSize);
		criAtomExAcfReader_Initialize(nullptr, Work, WorkSize);
		criAtomExAcfReader_Register(static_cast<CriUint8*>(const_cast<void*>(AcfData)), RawData.GetBulkDataSize());

		CriAtomExAcfInfo Info;
		criAtomExAcfReader_GetAcfInfo(&Info);

		AcfInfo.Name = UTF8_TO_TCHAR(Info.name);
		AcfInfo.ACFVersion = (uint32)Info.version;

		// AISAC
		AisacControls.Reset();
		for (int Index = 0; Index < Info.num_aisac_controls; ++Index)
		{
			CriAtomExAisacControlInfo ControlInfo;
			criAtomExAcfReader_GetAisacControlInfoByIdIndex((CriUint16)Index, &ControlInfo);
			FString ControlNameStr(UTF8_TO_TCHAR(ControlInfo.name));
			int32 ControlID = FMath::Max((uint32)0, (uint32)ControlInfo.id) & 0x7FFF;
			FAtomAisacControlInfo AisacControlInfo = { ControlNameStr, ControlID };

			AcfInfo.AisacControls.Add(AisacControlInfo);
			AisacControls.Emplace(FName(ControlNameStr), ControlID);
		}

		GlobalAisacPatches.Reset();
		for (int Index = 0; Index < Info.num_global_aisacs; ++Index)
		{
			CriAtomExGlobalAisacInfo AisacInfo;
			criAtomExAcfReader_GetGlobalAisacInfo((CriUint16)Index, &AisacInfo);
			CriBool DefaultControlFlag = CRI_FALSE;
			CriFloat32 DefaultControlValue = 0.0f;
			criAtomExAcfReader_GetGlobalAisacDefaultControl((CriUint16)Index, &DefaultControlFlag, &DefaultControlValue);

			const FName PatchName(UTF8_TO_TCHAR(AisacInfo.name));
			const int32 ControlID = FMath::Max((uint32)0, (uint32)AisacInfo.control_id);
			const bool Flag = DefaultControlFlag == CRI_TRUE;
			const float Value = (float)DefaultControlValue;

			FAtomAisacControl Control;
			if (AisacInfo.type != CRIATOMEXACF_AISAC_TYPE_AUTO_MODULATION)
			{
				if (FAtomAisacControl* ControlPtr = AisacControls.FindByPredicate([=](const FAtomAisacControl& Item) { return Item.ID == ControlID; }))
				{
					Control = *ControlPtr;
				}
			}
			else
			{
				// control id == index modulation
			}

			FString PatchNameStr = FString::Printf(TEXT("Patch_%s"), *PatchName.ToString());
			UAtomAisacPatch* Patch = NewObject<UAtomAisacPatch>(this, UAtomAisacPatch::StaticClass(), *PatchNameStr, RF_Public);
			if (Patch)
			{
				Patch->Init(PatchName, Flag, Value, Control);
				GlobalAisacPatches.Add(Patch);
			}
		}

		// Categories
		CategorieNames.Reset();
		for (int Index = 0; Index < Info.num_categories; ++Index)
		{
			CriAtomExCategoryInfo ExCategoryInfo;
			if (criAtomExAcfReader_GetCategoryInfo((CriUint16)Index, &ExCategoryInfo) == CRI_TRUE)
			{
				FString CategoryNameStr(UTF8_TO_TCHAR(ExCategoryInfo.name));
				FAtomCategoryInfo CategoryInfo = { CategoryNameStr, INDEX_NONE, INDEX_NONE, 0.0f, false }; // to get !

				AcfInfo.Categories.Add(CategoryInfo);

				CategorieNames.Emplace(CategoryNameStr);
			}
		}

		// Selectors
		Selectors.Reset();
		CriUint16 NumSelectors = criAtomExAcfReader_GetNumberOfSelectors();
		for (int SelectorIndex = 0; SelectorIndex < NumSelectors; ++SelectorIndex)
		{
			const CriChar8* SelectorName = criAtomExAcfReader_GetSelectorName(SelectorIndex);
			FString SelectorNameStr(UTF8_TO_TCHAR(SelectorName));
			FAtomSelectorInfo SelectorInfo = { SelectorNameStr };

			TArray<FName> Labels;
			CriUint16 NumLabels = criAtomExAcfReader_GetNumberOfSelectorLabels(SelectorIndex);
			for (int LabelIndex = 0; LabelIndex < NumLabels; ++LabelIndex)
			{
				const CriChar8* LabelName = criAtomExAcfReader_GetSelectorLabel(SelectorIndex, LabelIndex);
				FString LabelNameStr(UTF8_TO_TCHAR(LabelName));

				SelectorInfo.Labels.Add(LabelNameStr);
				Labels.Emplace(LabelNameStr);
			}

			AcfInfo.Selectors.Add(SelectorInfo);
			Selectors.Emplace(FName(SelectorNameStr), Labels);
		}

		// Game Variables
		GameVariables.Reset();
		for (int VariableIndex = 0; VariableIndex < Info.num_game_variables; ++VariableIndex)
		{
			CriAtomExGameVariableInfo VariableInfo;
			if (criAtomExAcfReader_GetGameVariableInfo(VariableIndex, &VariableInfo) == CRI_TRUE)
			{
				FString GameVariableNameStr(UTF8_TO_TCHAR(VariableInfo.name));
				const int32 VariableID = FMath::Max((uint32)0, (uint32)VariableInfo.id) & 0x7FFF;
				const float VariableValue = (float)VariableInfo.value;
				FAtomGameVariableInfo GameVariableInfo = { GameVariableNameStr, VariableID, VariableValue };

				AcfInfo.GameVariables.Add(GameVariableInfo);
				GameVariables.Emplace(FName(GameVariableNameStr), VariableID, VariableValue);
			}
		}

		// DSP settings
		FString DefaultDspSettingName(UTF8_TO_TCHAR(criAtomExAcfReader_GetDefaultDspBusSettingName()));

		int NumDspSettings = criAtomExAcfReader_GetNumberOfDspSettings();
		for (int Index = 0; Index < NumDspSettings; ++Index)
		{
			const CriChar8* DspSettingName = criAtomExAcfReader_GetDspSettingNameByIndex((CriUint16)Index);  // this function is CRIAPI but not exposed in real time version (missing!)
			CriAtomExAcfDspSettingInfo DspSettings;
			criAtomExAcfReader_GetDspSettingInformation(DspSettingName, &DspSettings);

			FString DspBusSettingName = FString::Printf(TEXT("%s_%s"), *GetName(), UTF8_TO_TCHAR(DspSettingName));
			
			UAtomDspBusSetting* NewDspSettings = nullptr;
			int SettingIndex = Index;
			if (TObjectPtr<UAtomDspBusSetting>* NewDspSettingsPtr = DspBusSettings.FindByPredicate([&DspBusSettingName](UAtomDspBusSetting* Settings) { return Settings->GetName() == DspBusSettingName; }))
			{
				// Use existing DspSetting object
				NewDspSettings = *NewDspSettingsPtr;
				SettingIndex = DspBusSettings.Find(NewDspSettings);
			}
			else
			{
				// Create DspSetting object
				NewDspSettings = NewObject<UAtomDspBusSetting>(this, UAtomDspBusSetting::StaticClass(), *DspBusSettingName, RF_Public);
				// Add to setting list
				SettingIndex = DspBusSettings.Insert(NewDspSettings, Index);
			}

			NewDspSettings->Init(DspSettings);
			
			// Check if default setting and store index
			if (DefaultDspSettingName.Compare(DspBusSettingName) == 0)
			{
				DefaultDspBusSettingIndex = SettingIndex;
			}
		}

		// Binauralizer
		CriSint32 BinauralizerType = INDEX_NONE;
		criAtomExAcfReader_GetBinauralizerType(&BinauralizerType);
		BinauralizerName = *FString::FromInt(BinauralizerType);

		// Advanced

		// Output ports
		OutputPortNames.Reset();
		for (int PortIndex = 0; PortIndex < Info.num_output_ports; ++PortIndex)
		{
			CriAtomExOutputPortInfo PortInfo;
			if (criAtomExAcfReader_GetOutputPortInfo(PortIndex, &PortInfo) == CRI_TRUE)
			{	
				FString OutputPortNameStr(UTF8_TO_TCHAR(PortInfo.name));
				FAtomOutputPortInfo OutputPortInfo = { OutputPortNameStr };

				AcfInfo.OutputPorts.Add(OutputPortInfo);
				OutputPortNames.Emplace(OutputPortNameStr);
			}
		}

		RackConfig.Update(OutputPortNames);

		criAtomExAcfReader_Finalize();
		FMemory::Free(Work);
#endif

		// Load DSP settings names
		/*DSPSettings.Empty();

		int NumDspSettings = FCriWareApi::criAtomExAcf_GetNumDspSettingsFromAcfData(const_cast<void*>(AcfData), RawData.GetBulkDataSize());
		for (int i = 0; i < NumDspSettings; ++i)
		{
			const CriChar8* DspSettingName = FCriWareApi::criAtomExAcf_GetDspSettingNameByIndexFromAcfData(const_cast<void*>(AcfData), RawData.GetBulkDataSize(), (CriUint16)i);
			auto* NewDspSettings = NewObject<UAtomDspBusSetting>(this, UAtomDspBusSetting::StaticClass(), DspSettingName, RF_Public);
			NewDspSettings->Name = DspSettingName;
			DSPSettings.Add(NewDspSettings);
		}

		// Load Category names
		Categories.Empty();

		int NumCategories = FCriWareApi::criAtomExAcf_GetNumCategoriesFromAcfData(const_cast<void*>(AcfData), RawData.GetBulkDataSize());
		for (int i = 0; i < NumCategories; ++i)
		{
			const CriChar8* Name = FCriWareApi::criAtomExAcf_Get???FromAcfData(const_cast<void*>(AcfData), RawData.GetBulkDataSize(), (CriUint16)i);
			Categories.Add(UTF8_TO_TCHAR(Name));
		}*/
	}
	RawData.Unlock();

	return true;
}

UAtomDspBusSetting* UAtomConfig::GetDefaultDspBusSetting() const
{
	if (DspBusSettings.IsValidIndex(DefaultDspBusSettingIndex))
	{
		return DspBusSettings[DefaultDspBusSettingIndex];
	}

	return nullptr;
}

void UAtomConfig::PostLoad()
{
	Super::PostLoad();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
#if WITH_EDITORONLY_DATA
		if (GetLinkerCustomVersion(FAtomCustomVersion::GUID) < FAtomCustomVersion::LatestVersion)
		{
			// reload info from raw data
			LoadSettings();

			auto Package = GetOutermost();
			if (Package)
			{
				Package->SetDirtyFlag(true);
				Package->PackageMarkedDirtyEvent.Broadcast(Package, true);
			}
		}
#endif
	}
}

void UAtomConfig::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif

	Super::PostInitProperties();
}

void UAtomConfig::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomConfigCustomVersion::GUID);
	Ar.UsingCustomVersion(FAtomCustomVersion::GUID);

#if WITH_EDITORONLY_DATA
	if ((Ar.IsLoading()) && (AssetImportData == nullptr))
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif

	// Serialize ACF data
	RawData.Serialize(Ar, this);
};

void UAtomConfig::PostRename(UObject* OldOuter, FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	for (auto DspBusSetting : DspBusSettings)
	{
		FString DspBusSettingName = FString::Printf(TEXT("%s_%s"), *GetName(), *DspBusSetting->GetSettingName());
		DspBusSetting->Rename(*DspBusSettingName, this);
	}
}

void UAtomConfig::PreDuplicate(FObjectDuplicationParameters& DupParams)
{
	Super::PreDuplicate(DupParams);
}

void UAtomConfig::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	LoadSettings();
}

#if WITH_EDITORONLY_DATA
void UAtomConfig::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    Super::GetAssetRegistryTags(OutTags);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS;
#else
    if (AssetImportData)
    {
        OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }
    
    Super::GetAssetRegistryTags(OutTags);
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
void UAtomConfig::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);

	if (AssetImportData)
	{
        Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}
}
#endif
#endif

#if WITH_EDITOR
void UAtomDspBusSetting::GetDspBusEndPointInfoFromAcfDspBusInfo(
	const CriAtomExAcfDspBusInfo& InBusInfo,
	EAtomSpeakerChannelMap& OutSpeakerChannelMap,
	EAtomSoundRendererType& OutSoundRendererType,
	EAtomSoundfieldRendererType& OutSoundfieldRendererType)
{
	OutSpeakerChannelMap = FAtomRuntime::GetSpeakerChannelMapFromAtomSpeakerMapping(InBusInfo.speaker_mapping);
	OutSoundRendererType = EAtomSoundRendererType::Default;
	OutSoundfieldRendererType = EAtomSoundfieldRendererType::Default;

	// Determine output renderer type.
	switch (InBusInfo.output_type)
	{
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_NONE:
		OutSoundRendererType = EAtomSoundRendererType::Muted;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_MAIN:
		OutSoundRendererType = EAtomSoundRendererType::Main;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_MAIN_PASSTHROUGH:
		OutSoundRendererType = EAtomSoundRendererType::Main;
		OutSoundfieldRendererType = EAtomSoundfieldRendererType::Passthrough;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_PAD_HAPTIC:
		OutSoundRendererType = EAtomSoundRendererType::Haptic;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_PAD_SPEAKER:
		OutSoundRendererType = EAtomSoundRendererType::Pad;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_PERSONAL:
		OutSoundRendererType = EAtomSoundRendererType::User;
		break;
	case CRIATOMEXACF_DSP_BUS_OUTPUT_TYPE_PERSONAL_PASSTHROUGH:
		OutSoundRendererType = EAtomSoundRendererType::User;
		OutSoundfieldRendererType = EAtomSoundfieldRendererType::Passthrough;
		break;
	default:
		OutSoundRendererType = EAtomSoundRendererType::Default;
		break;
	}

	// Determine spatialization encoding type.
	if (OutSoundfieldRendererType != EAtomSoundfieldRendererType::Passthrough)
	{
		switch (InBusInfo.speaker_mapping)
		{
		case CRIATOM_SPEAKER_MAPPING_AUTO:
			OutSoundfieldRendererType = EAtomSoundfieldRendererType::Spatializer;
			break;
		case CRIATOM_SPEAKER_MAPPING_MONO:
		case CRIATOM_SPEAKER_MAPPING_STEREO:
		case CRIATOM_SPEAKER_MAPPING_5_1:
		case CRIATOM_SPEAKER_MAPPING_7_1:
		case CRIATOM_SPEAKER_MAPPING_5_1_2:
		case CRIATOM_SPEAKER_MAPPING_7_1_2:
		case CRIATOM_SPEAKER_MAPPING_7_1_4:
		case CRIATOM_SPEAKER_MAPPING_7_1_4_4:
			OutSoundfieldRendererType = EAtomSoundfieldRendererType::Default;
			break;
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_1P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_2P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_3P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_4P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_5P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_6P:
		case CRIATOM_SPEAKER_MAPPING_AMBISONICS_7P:
			OutSoundfieldRendererType = EAtomSoundfieldRendererType::Ambisonics;
			break;
		case CRIATOM_SPEAKER_MAPPING_OBJECT_16:
		case CRIATOM_SPEAKER_MAPPING_OBJECT_32:
		case CRIATOM_SPEAKER_MAPPING_OBJECT_64:
		case CRIATOM_SPEAKER_MAPPING_OBJECT_128:
			OutSoundfieldRendererType = EAtomSoundfieldRendererType::SoundObject;
			break;
		case CRIATOM_SPEAKER_MAPPING_CUSTOM:
		default:
			OutSoundfieldRendererType = EAtomSoundfieldRendererType::Default;
			break;
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
