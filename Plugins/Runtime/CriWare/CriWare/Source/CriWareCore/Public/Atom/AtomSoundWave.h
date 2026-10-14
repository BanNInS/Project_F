/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundWave.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ScriptMacros.h"

#include "AtomSoundBase.h"
#include "AtomAttenuation.h"

#include "Atom/AtomActiveSound.h"

#include "AtomSoundWave.generated.h"

// Forward Declarations
class UAtomWaveBank;
class IAtomSoundResource;

UCLASS(HideCategories = Object, BlueprintType, Within = AtomWaveBank)
class CRIWARECORE_API UAtomSoundWave
	: public UAtomSoundBase
{
	GENERATED_BODY()

public:

	UAtomSoundWave(const FObjectInitializer& ObjectInitializer);

	void Init(UAtomWaveBank* InWaveBank, uint32 InWaveID);
	
	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual bool Rename(const TCHAR* NewName = nullptr, UObject* NewOuter = nullptr, ERenameFlags Flags = REN_None) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UAtomSoundBase interface.
	virtual bool IsPlayable() const override;
	virtual bool IsStreaming(const TCHAR* PlatformName = nullptr) const override;
	virtual float GetMaxDistance() const override;
	virtual float GetDuration() const override;
	virtual float GetVolumeMultiplier() override;
	virtual float GetPitchMultiplier() override;
	virtual const FAtomAttenuationSettings* GetAttenuationSettingsToApply() const override;
	virtual IAtomSoundResource* GetSoundResource() override;
	virtual void GetAllSoundResources(TArray<IAtomSoundResource*>& Resources) override;
	virtual FString GetSoundName() const override;
	//~ End UAtomSoundBase interface.

	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Wave")
	TObjectPtr<UAtomWaveBank> WaveBank;

	UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = "Wave")
	int32 WaveID;

	/** If set, when played directly the wave will be played looping. */
	UPROPERTY(EditAnywhere, Category = Sound, AssetRegistrySearchable)
	uint8 bLooping : 1;

	/** Indicates whether attenuation should use the Attenuation Overrides or the Attenuation Settings asset */
	UPROPERTY(EditAnywhere, Category = Attenuation)
	uint8 bOverrideAttenuation : 1;

	/** Playback volume of sound 0 to 1 - Default is 1.0. */
	UPROPERTY(Category = Sound, meta = (ClampMin = "0.0"), EditAnywhere)
	float Volume;

	/** Playback pitch for sound. */
	UPROPERTY(Category = Sound, meta = (ClampMin = "0.125", ClampMax = "4.0"), EditAnywhere)
	float Pitch;

	/** Attenuation settings to use if Override Attenuation is set to true */
	UPROPERTY(EditAnywhere, Category=Attenuation)
	FAtomAttenuationSettings AttenuationOverrides;

	/** Call to cache any values which need to be computed from the sound wave info. */
	void CacheAggregateValues();

#if WITH_EDITOR
	bool GetWaveInfo(FAtomResourceHandle& InHandle, FAtomWaveInfo& OutWaveInfo) const;
#endif

	friend UAtomWaveBank;
};
