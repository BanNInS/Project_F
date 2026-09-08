/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : UAtomSoundBank.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Serialization/BulkData.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomResourceManager.h"
#include "AtomResource.h"

#include "AtomSoundBank.generated.h"

// Forward declarations.
struct FCriWareCookOverridesSettings;

/*
 * Interface class for sound that are holding data 
 * -> todo a streamed source/cue proxy with tshared rawdata holder
 *****************************************************************************/
UINTERFACE(MinimalAPI)
class UAtomSoundResource : public UInterface
{
	GENERATED_BODY()
};

class IAtomSoundResource
{
	GENERATED_BODY()

public:
	// Called by resource system when a new resource for this sound is requested.
	virtual FAtomResource* CreateAtomResource() { return new FAtomResource(); }
	// Called by resource system when resource is initalized and will access to this sound data.
	virtual bool InitAtomResource() { return false; }
	// Called by resource system when resource is looking for data ptr to read if inlined.
	virtual const uint8* GetResourceData() { return nullptr; }
	// Called by resource system when resource is removed and will not use data anymore.
	virtual void RemoveAtomResource() {}
	// Called by resource system when a resource using this sound will be fully destroyed.
	virtual void DestroyAtomResource() {}
	// Called by resource system when checking if a resource could be unloaded.
	virtual bool CanUnloadAtomResource() { return true; }
};

/*
 * Common class for Wave Bank and Cue Sheet
 *****************************************************************************/
UCLASS(HideCategories = Object, abstract, BlueprintType)
class CRIWARECORE_API UAtomSoundBank
	: public UObject
	, public IAtomSoundResource
	, public IAtomSoundAsset
{
	GENERATED_BODY()

public:

	UAtomSoundBank(const FObjectInitializer& ObjectInitializer);

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
#endif
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	//~ End UObject Interface

#if WITH_EDITOR
	void PostImport();
#endif

public:
	// Loading behavior members are lazily initialized in const getters
	/** Specifies how and when wave data are loaded for asset if stream cache is enabled. */
	UPROPERTY(EditAnywhere, Category = "Loading", meta = (DisplayName = "Loading Behavior Override"))
	mutable EAtomSoundWaveLoadingBehavior LoadingBehavior;

	/** Set to true if LoadingBehavior was inherited from a AtomCueSheet. This is useful for debugging/logging */
	mutable TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> bLoadingBehaviorOverriddenPtr{ MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>() };

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Instanced, Category = Reimport)
	TObjectPtr<UAssetImportData> AssetImportData;
#endif

	bool IsStreaming(const TCHAR* PlatformName = nullptr) const;
	bool IsStreaming(const FCriWareCookOverridesSettings& Overrides) const;

	EAtomSoundWaveLoadingBehavior GetLoadingBehavior() const;
	// We cache a soundwave's loading behavior on the first call to USoundWave::GetLoadingBehaviorForWave(true);
	void CacheInheritedLoadingBehavior() const;
	mutable EAtomSoundWaveLoadingBehavior CachedSoundWaveLoadingBehavior{ EAtomSoundWaveLoadingBehavior::Uninitialized };

	virtual void RetainResource();
	virtual void ReleaseResource();
	virtual bool IsRetainingResource() const;

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound")
	virtual UClass* GetSoundClass() const PURE_VIRTUAL(UAtomSoundBank::GetSoundClass, return nullptr;);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound")
	virtual int GetNumSounds() const PURE_VIRTUAL(UAtomSoundBank::GetNumSounds, return 0;);

	/** */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound")
	virtual UAtomSoundBase* GetSound(int Index) const PURE_VIRTUAL(UAtomSoundBank::GetSound, return nullptr;);

	// todo: move this to a shared ptr FAtomStreamData and use UxxBank with proxies
	
	FByteBulkData* GetRawData();

	/** Critical section for synchronizing load/unload. */
	/** FByteBulkData doesn't currently support readonly access from multiple threads, so we limit access to RawData with a critical section on cook. */
	FCriticalSection RawDataCriticalSection;

	/** Raw data of the sound bank. */
	FByteBulkData RawData;

	/** Handle of the retained Atom resource used with Atom runtime. */
	FAtomResourceHandle AtomResource;

protected:

	TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> bIsStreamingProxyFlag{ MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>() }; // shared w/ proxies. The below functions should update this bool
};
