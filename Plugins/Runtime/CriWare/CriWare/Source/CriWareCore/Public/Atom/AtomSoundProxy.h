
#pragma once

#include "AtomSoundBase.h"
#include "AtomAttenuation.h"
#include "AtomActiveSound.h"

#include "AtomSoundProxy.generated.h"

/**
 * An Atom sound that reditect to another sound.
 */
UCLASS(BlueprintType, MinimalAPI)
class UAtomSoundProxy
	: public UAtomSoundBase
{
	GENERATED_BODY()

	CRIWARECORE_API UAtomSoundProxy(const FObjectInitializer& ObjectInitializer);

private:

	/** Cached Atom sound proxy. */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UAtomSoundBase> DynamicProxy;

	/** Atom sound source proxy. */
	UPROPERTY(EditAnywhere, Category = "Sound", meta = (DisplayName = "Proxy Sound"))
	TObjectPtr<UAtomSoundBase> Proxy;

public:

	/** Whether modulation priority defined in sound proxy overrides priority set in sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Management|Priority", meta = (DisplayAfter = ""))
	bool bOverridePriority = false;

	/** Whether modulation settings defined in sound proxy overrides settings set in sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modulation")
	bool bOverrideModulationSettings = false;

	/**
	 * Get the Atom sound proxy.
	 *
	 * @return The Atom sound, or nullptr if not set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound Proxy")
	CRIWARECORE_API UAtomSoundBase* GetSound() const;

	/**
	 * Get the last element of the Atom sound chain that is not a Atom sound proxy.
	 *
	 * @return The Atom sound, or nullptr if not set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound Proxy")
	CRIWARECORE_API UAtomSoundBase* GetLeafSound() const;

	/**
	 * Is the media proxy has a valid proxy.
	 *
	 * @return true if the proxy is valid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound Proxy")
	CRIWARECORE_API bool IsProxyValid() const;

	/**
	 * Set the dynamic Atom sound proxy.
	 * 
	 * @param InProxy The proxy to use.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom|Sound Proxy")
	CRIWARECORE_API void SetDynamicSound(UAtomSoundBase* InProxy);

#if WITH_EDITOR
	/**
	 * Set the Atom sound proxy.
	 *
	 * @param InProxy The proxy to set.
	 */
	CRIWARECORE_API void SetSound(UAtomSoundBase* InProxy);
#endif

public:

	//~ Begin UAtomSoundBase Interface
	CRIWARECORE_API virtual bool IsPlayable() const override;
	CRIWARECORE_API virtual bool IsStreaming(const TCHAR* PlatformName = nullptr) const override;
	CRIWARECORE_API virtual bool IsSeekable() const override;
	CRIWARECORE_API virtual const FAtomAttenuationSettings* GetAttenuationSettingsToApply() const override;
	CRIWARECORE_API virtual float GetMaxDistance() const override;
	CRIWARECORE_API virtual float GetDuration() const override;
	CRIWARECORE_API virtual bool IsPlayWhenSilent() const override;
	CRIWARECORE_API virtual float GetVolumeMultiplier() override;
	CRIWARECORE_API virtual float GetPitchMultiplier() override;
	CRIWARECORE_API virtual EAtomVirtualizationMode GetVirtualizationMode() const override;
	CRIWARECORE_API virtual bool ShouldApplyInteriorVolumes() override;
	CRIWARECORE_API virtual IAtomSoundResource* GetSoundResource() override;
	CRIWARECORE_API virtual void GetAllSoundResources(TArray<IAtomSoundResource*>& Resources) override;
	CRIWARECORE_API virtual bool IsLooping() const override;
	CRIWARECORE_API virtual bool IsOneShot() const override;
	CRIWARECORE_API virtual void Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& OutPlaybackInstances) override;
	CRIWARECORE_API virtual UAtomSoundClass* GetSoundClass() const override;
	CRIWARECORE_API virtual UAtomRackBase* GetAtomRack() const override;
	CRIWARECORE_API virtual void GetAtomBusSends(TArray<FAtomSoundToBusSend>& OutSends) const override;
	CRIWARECORE_API virtual void GetSoundSourceBusSends(EAtomBusSendStage BusSendStage, TArray<FAtomSoundSourceBusSendInfo>& OutSends) const override;
	CRIWARECORE_API virtual void GetConcurrencyHandles(TArray<FAtomConcurrencyHandle>& OutConcurrencyHandles) const override;
	CRIWARECORE_API virtual float GetPriority() const override;
	CRIWARECORE_API virtual const FAtomSoundModulationRoutingSettings& GetModulationSettings() const override;
#if WITH_EDITOR
	CRIWARECORE_API virtual bool GetWaveInfo(FAtomResourceHandle& InHandle, FAtomWaveInfo& OutWaveInfo) const override;
#endif
	//~ End UAtomSoundBase Interface

private:

	mutable bool bIsPlayableGuard;
	mutable bool bAttenuationSettingsToApplyGuard;
	mutable bool bGetMaxDistanceGuard;
	mutable bool bGetDurationGuard;
	mutable bool bGetVirtualizationModeGuard;
	mutable bool bGetResourceGuard;
	mutable bool bGetSoundClassGuard;
	mutable bool bCommonGuard;
	mutable bool bLeafSound;
};

/// Customized Sound

class UAtomSoundCue;

/**
 * MySound is a user customized example asset.
 * It currently shows how to create your own UAtomSound assets in c++ for your game needs.
 * Note: This asset may change in future versions.
 *
 * MySound can hold 1 or 2 AtomSoundCue assets.
 * In case two cues are used, when the first cue has ended or stopped whith GetAllPlaybacks()/StopPlayback(),
 * the second cue starts to play.
 * The sound duration is based on the 1st cue.
 */
UCLASS(BlueprintType, MinimalAPI)
class UMySound : public UAtomSoundBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = SoundToPlay)
	TObjectPtr<UAtomSoundCue> Cue;

	UPROPERTY(EditAnywhere, Category = SoundToPlay)
	TObjectPtr<UAtomSoundCue> Cue2;

	CRIWARECORE_API virtual bool IsPlayable() const override;
	CRIWARECORE_API virtual float GetMaxDistance() const override;
	CRIWARECORE_API virtual float GetDuration() const override;

	CRIWARECORE_API virtual void Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& OutPlaybackInstances) override;

	// disable the waveform thumbnail
#if WITH_EDITOR
	CRIWARECORE_API virtual bool CanVisualizeAsset() const override { return false; }
#endif
};
