/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffect.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "AtomEffectPreset.h"

#include "AtomBusEffectPreset.generated.h"

// Forward declarations
class UAtomBus;

/** Preset of a bus effect that can be shared between sounds. */
UCLASS(config = CriWare, hidecategories = Object, abstract, editinlinenew, BlueprintType)
class CRIWARECORE_API UAtomBusEffectPreset 
	: public UAtomEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectPreset(const FObjectInitializer& ObjectInitializer);

	//~ UAtomEffectPreset Interface
	virtual FColor GetPresetColor() const override { return FColor(162.0f, 84.0f, 101.0f); }

	/** Gets the owner AtomBus. It can be nullptr if preset is not bus default preset. (Not present in same package than rack/bus.) */
	UAtomBus* GetBus() const;

public:

	// Interface to convert parameters to/from atom runtime.
	virtual float GetParameterValue(int ParameterIndex) const { return 0.0f; }
	virtual void SetParameterValue(int ParameterIndex, float Value) {}
	virtual bool IsBypassed() const { return false; }
	virtual void SetBypass(bool bBypass) {}

	// Applies dynamic settings to UObject.
	virtual void ApplyToDefautlsSettings() {}

public:

	//~ Begin UObject Interface
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:

	/** Convinient function to create effect preset object from ADX bus effect name. */
	static UAtomBusEffectPreset* CreateBusEffectPreset(UObject* InOuter, const FName& InBusEffectName);

	/** Convinient function to create effect preset object from a UAtomBusEffectPreset sub-class. */
	static UAtomBusEffectPreset* CreateBusEffectPreset(UObject* InOuter, const UClass* InPresetClass, const FName& InName = NAME_None);

	/** Gets the preset class from ADX bus effect name. */
	static const UClass* GetBusEffectPresetClassFromName(const FName& InBusEffectName);

	/** Gets the ADX bus effect name from preset class. */
	static const FName GetBusEffectPresetNameFromClass(const UClass* InBusEffectClass);

	/** Gets the number of parameters used by a bus effect. */
	static int32 GetBusEffectPresetNumParameters(const FName& InBusEffectName);

private:

	/** Keeps the buses using this preset to apply effect settings to Atom runtime. */
	UPROPERTY()
	TSet<TObjectPtr<const UAtomBus>> TargetBuses;

	/** Links with a bus to apply this preset. */
	void AddTargetBus(const UAtomBus* InBus);

	/** Removes link this preset to a bus. */
	void RemoveTargetBus(const UAtomBus* InBus);

protected:

	bool bInitialized = false;

	// Updates all or individually effect parameters to Atom Runtime for all attached buses.
	void Update() const;
	void UpdateParameter(int ParamIndex) const;
	void UpdateBypass() const;

	friend class UAtomBus;
	friend class FAtomRuntime;


protected:

	static void RegisterBusEffectClass(FName BusEffectName, UClass* BusEffectClass, int32 BusEffectNumParameters)
	{
		BusEffects.Add(BusEffectName, { BusEffectClass, BusEffectNumParameters });
	}

	/** The global Atom bus effect dictionary */
	static TMap<FName, TPair<const UClass*, int> > BusEffects;
};
