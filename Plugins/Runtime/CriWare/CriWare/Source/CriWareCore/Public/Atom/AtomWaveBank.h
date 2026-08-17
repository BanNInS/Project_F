/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomWaveBank.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "Async/Async.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CriWareUtils.h"
#include "AtomSoundBank.h"

#include "AtomWaveBank.generated.h"

// Forward declarations.
struct FOwnedBulkDataPtr;
class UAtomSoundWave;

 /**
  * UAtomWaveBank Class
  */
UCLASS(hidecategories = object, meta = (ToolTip = "Asset holding Atom Wave Bank (AWB) file or data."))
class CRIWARECORE_API UAtomWaveBank
	: public UAtomSoundBank
{
	GENERATED_BODY()

public:

	UAtomWaveBank(const FObjectInitializer& ObjectInitializer);

	/**
	 * Validates the Atom wave bank settings.
	 *
	 * @return true if validation passed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|WaveBank")
	virtual bool Validate() const;

	/**
	 * Gets the number of wave in this bank.
	 *
	 * @return The number of waves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|WaveBank")
	int32 GetNumWaves() const;

	/**
	 * Gets the wave object from this bank at choosen index.
	 *
	 * @return The UAtomSoundWave at the index in the bank or nullptr if an invalid index was provided.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|WaveBank")
	UAtomSoundWave* GetSoundWave(int Index) const
	{
		return SoundWaves.IsValidIndex(Index) ? SoundWaves[Index] : nullptr;
	}

	/**
	 * Returns load status of the the bank in Atom library and if ready to be accessed.
	 *
	 * @return true if the bank is loaded.
	 * @see SetFilePath
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|WaveBank")
	bool IsLoaded() const;

	//~ Begin IAtomSoundResource Interface
	/**
	 * Copy/Steal the acb data from the bulk data
	 */
	virtual bool InitAtomResource() override;

	/**
	 * Remove the data associated with the passed acb
	 */
	virtual void RemoveAtomResource() override;

	/**
	 * Get the pointer on acb data if available, otherwise nullptr.
	 */
	virtual const uint8* GetResourceData() override;

	virtual FAtomResource* CreateAtomResource() override;
	virtual void DestroyAtomResource() override;
	virtual bool CanUnloadAtomResource() override;

	virtual UClass* GetSoundClass() const override;
	virtual int GetNumSounds() const override;
	virtual UAtomSoundBase* GetSound(int Index) const override;
	//~ End IAtomSoundResource Interface

public:

	//~ Begin UObject interface.
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
	virtual void FinishDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void PostInitProperties() override;
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITORONLY_DATA
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
#endif
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
#endif
	//~ End UObject interface.

private:
	UPROPERTY(VisibleAnyWhere, Category = Waves)
	TArray<TObjectPtr<UAtomSoundWave>> SoundWaves;

	/** Memory containing the data copied or mapped from the raw bulk data. */
	FOwnedBulkDataPtr* OwnedBulkDataPtr{ nullptr };
	const uint8* ResourceData{ nullptr };

	/** Size of resource copied from the bulk data. */
	int32 ResourceSize;

	friend class UAtomWaveBankFactory;
};
