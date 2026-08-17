/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomCueSheet.h
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

#include "AtomCueSheet.generated.h"

 // Forward declarations.
struct FOwnedBulkDataPtr;
class UAtomSoundCue;
class UAtomWaveBank;

/**
 * UAtomCueSheet Class
 */
UCLASS(HideCategories = Object, meta = (ToolTip = "Asset holding Atom CueSheet Binary (ACB) file or data."))
class CRIWARECORE_API UAtomCueSheet
	: public UAtomSoundBank
{
	GENERATED_BODY()

public:

	UAtomCueSheet(const FObjectInitializer& ObjectInitializer);

	/**
	 * Validates the Atom cue sheet settings.
	 *
	 * @return true if validation passed, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|CueSheet")
	virtual bool Validate() const;

	/**
	 * Gets the number of wave in this bank.
	 *
	 * @return The number of waves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|CueSheet")
	int32 GetNumCues() const;

	/**
	 * Gets the cue object from this bank at choosen index.
	 *
	 * @return The UAtomSoundCue at the index in the bank or nullptr if an invalid index was provided.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|CueSheet")
	UAtomSoundCue* GetSoundCue(int Index) const
	{
		return SoundCues.IsValidIndex(Index) ? SoundCues[Index] : nullptr;
	}

	/**
	 * Retreive the cue object from this bank by its name.
	 *
	 * @return The UAtomSoundCue with the chosen name in the bank or nullptr if the cue was not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|CueSheet")
	UAtomSoundCue* GetSoundCueByName(const FString& CueName) const;

	/**
	 * Returns load status of the the bank in Atom library and if ready to be accessed.
	 *
	 * @return true if the bank is loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|CueSheet")
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
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject interface.

	/** AtomWaveBank used by this AtomCueSheet. (Property is hidden in editor if ACB does not request any AtomWaveBanks.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, EditFixedSize, meta = (EditFixedOrder), Category = WaveBanks)
	TArray<TObjectPtr<UAtomWaveBank>> WaveBanks;

private:

	UPROPERTY(VisibleAnyWhere, Category = Cues)
	TArray<TObjectPtr<UAtomSoundCue>> SoundCues;

	/** Memory containing the data copied or mapped from the raw bulk data. */
	FOwnedBulkDataPtr* OwnedBulkDataPtr{ nullptr };
	const uint8* ResourceData{ nullptr };

	/** Size of resource copied from the bulk data. */
	int32 ResourceSize;

	friend class UAtomCueSheetFactory;
};
