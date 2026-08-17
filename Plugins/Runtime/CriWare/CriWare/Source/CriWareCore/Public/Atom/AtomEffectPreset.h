/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomEffect.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "AtomEffectPreset.generated.h"

 // The following macro code creates boiler-plate code for a sound effect preset and hides unnecessary details from user-created effects.

 // Macro chain to expand "MyEffectName" to "FMyEffectNameSettings"
#define ATOM_EFFECT_SETTINGS_NAME2(CLASS_NAME, SUFFIX) F ## CLASS_NAME ## SUFFIX
#define ATOM_EFFECT_SETTINGS_NAME1(CLASS_NAME, SUFFIX) ATOM_EFFECT_SETTINGS_NAME2(CLASS_NAME, SUFFIX)
#define ATOM_EFFECT_SETTINGS_NAME(CLASS_NAME)		   ATOM_EFFECT_SETTINGS_NAME1(CLASS_NAME, Settings)

#define ATOM_EFFECT_PRESET_NAME2(CLASS_NAME, SUFFIX)  U ## CLASS_NAME ## SUFFIX
#define ATOM_EFFECT_PRESET_NAME1(CLASS_NAME, SUFFIX)  ATOM_EFFECT_PRESET_NAME2(CLASS_NAME, SUFFIX)
#define ATOM_EFFECT_PRESET_NAME(CLASS_NAME)			  ATOM_EFFECT_PRESET_NAME1(CLASS_NAME, Preset)

#define GET_ATOM_EFFECT_SETTINGS(EFFECT_NAME) \
		U##EFFECT_NAME##Preset* _Preset = Cast<U##EFFECT_NAME##Preset>(Preset); \
		F##EFFECT_NAME##Settings Settings = _Preset != nullptr ? _Preset->GetSettings() : F##EFFECT_NAME##Settings();

#define ATOM_EFFECT_PRESET_METHODS(EFFECT_NAME) \
		virtual bool CanFilter() const override { return false; } \
		virtual bool HasAssetActions() const override { return true; } \
		virtual FText GetAssetActionName() const override { return FText::FromString(#EFFECT_NAME); } \
		virtual UClass* GetSupportedClass() const override { return ATOM_EFFECT_PRESET_NAME(EFFECT_NAME)::StaticClass(); } \
		virtual UAtomEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const override \
		{ \
			UAtomEffectPreset* NewPreset = NewObject<ATOM_EFFECT_PRESET_NAME(EFFECT_NAME)>(InParent, GetSupportedClass(), Name, Flags); \
			NewPreset->Init(); \
			return NewPreset; \
		} \
		virtual void Init() override \
		{ \
			FScopeLock ScopeLock(&SettingsCritSect); \
			DynamicSettings = Settings; \
		} \
		void UpdateSettings(const F##EFFECT_NAME##Settings& InSettings) \
		{ \
			FScopeLock ScopeLock(&SettingsCritSect); \
			DynamicSettings = InSettings; \
			Update(); \
		} \
		void UpdateSettings(TUniqueFunction<void(F##EFFECT_NAME##Settings&)> InCommand) \
		{ \
			FScopeLock ScopeLock(&SettingsCritSect); \
			InCommand(DynamicSettings); \
			Update(); \
		} \
		F##EFFECT_NAME##Settings GetSettings() \
		{ \
			FScopeLock ScopeLock(&SettingsCritSect); \
			return DynamicSettings; \
		} \
		virtual void ApplyToDefautlsSettings() override \
		{ \
			FScopeLock ScopeLock(&SettingsCritSect); \
			Settings = DynamicSettings; \
		} \
		FCriticalSection SettingsCritSect;
		//F##EFFECT_NAME##Settings SettingsCopy; 
		// not used yet, Updates to runtime parameters seems a thread safe operation in Atom lib.
		// we actually use DynamicSettings Uproperty as source for Runtime

#define ATOM_BUS_EFFECT_PRESET_METHODS(EFFECT_NAME) \
		bool IsBypassed() const override \
		{ \
			return DynamicSettings.bBypass; \
		} \
		void SetBypass(bool bBypass) override \
		{ \
			DynamicSettings.bBypass = bBypass; \
		}

/*
 * UAtomEffectPreset Class
 *****************************************************************************/

UCLASS(config = CriWare, abstract, editinlinenew, BlueprintType)
class CRIWARECORE_API UAtomEffectPreset
	: public UObject
{
	GENERATED_BODY()

public:
	UAtomEffectPreset(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomEffectPreset() = default;

	virtual bool CanFilter() const { return true; }
	virtual bool HasAssetActions() const { return false; }
	virtual FText GetAssetActionName() const PURE_VIRTUAL(UAtomEffectPreset::GetAssetActionName, return FText(););
	virtual UClass* GetSupportedClass() const PURE_VIRTUAL(UAtomEffectPreset::GetSupportedClass, return nullptr;);
	virtual UAtomEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const PURE_VIRTUAL(UAtomEffectPreset::CreateNewPreset, return nullptr;);
	virtual void Init() PURE_VIRTUAL(UAtomEffectPreset::Init, );
	virtual FColor GetPresetColor() const { return FColor(200.0f, 100.0f, 100.0f); }

	/** Get the Atom effect plugin from this effect belongs. */
	FORCEINLINE FName GetPluginName() const { return PluginName; }

	/** Get the full effect name. */
	FORCEINLINE FName GetEffectName() const { return EffectName; }

	/** Get the number of parameters of the effect. */
	FORCEINLINE int32 GetNumParameters() const { return NumParameters; }

protected:

	/** The Atom effect plugin from this effect belongs. */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Effect")
	FName PluginName;

	/** The full name that identifies the effect in Atom. */
	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Effect")
	FName EffectName;

	/** The number of parameters of the effect. */
	UPROPERTY()
	int32 NumParameters;
};
