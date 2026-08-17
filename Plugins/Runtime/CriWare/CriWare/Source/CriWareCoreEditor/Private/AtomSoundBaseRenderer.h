
#pragma once

#include "CoreMinimal.h"
#include "TickableEditorObject.h"
#include "UObject/Object.h"

#include "Atom/AtomSoundBaseRendererInterface.h"
#include "Atom/AtomResourceManager.h"

#include "AtomSoundBaseRenderer.generated.h"

class UAtomComponent;
class UAtomSoundBase;
class UTexture;

/** Renders a Atom sound to a byte bulk data in editor builds. */
UCLASS()
class UAtomSoundBaseRenderer
	: public UObject
	, public IAtomSoundBaseRendererInterface
	, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	/** IAtomSoundBaseRendererInterface interface */
	virtual FByteBulkData& Open(UAtomSoundBase* InAtomSound, int InNumFrames) override;

	/** FTickableEditorObject interface */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAtomSoundBaseRenderer, STATGROUP_Tickables); }

private:
	// not used yet
	/**
	 * Callback when the playback is done.
	 */
	UFUNCTION()
	void OnPlaybackEnded();

	// not used yet
	/**
	 * Cleans everything up.
	 */
	void Close();

	/** Holds the Atom sound we are using. */
	UPROPERTY(Transient)
	TObjectPtr<UAtomSoundBase> AtomSound = nullptr;

	// not used yet (write to sound inner RawSnapshot)
	/** Holds the data we are using. */
	FByteBulkData RawSnapshot;
	int32 RawSnapshotNumChannels;

	/** True if awaiting to generated a snapshot of PCM data. */
	bool bIsRequestingSnapshot = false;

	/** Current Resource handle used when generating the snapshot. */
	FAtomResourceHandle SnapshotResource;

	static const int MaxSimultaneousSnapshotGenerators = 5;
	inline static int NumSnapshotGeneratorsInFlight = 0;

	// --- PCM full Data --- 
	// todo: merge the two generators

public:

	/** IAtomSoundBaseRendererInterface interface */
	virtual FByteBulkData& Open(UAtomSoundBase* InAtomSound) override;

private:

	FByteBulkData RawPCMData; // unused yet

	bool bIsRequestingPCMData = false;

	/** Current Resource handle used when generating the pcmRawData. */
	FAtomResourceHandle RawPCMDataResource;

	static const int MaxSimultaneousPCMGenerators = 1;
	inline static int NumPCMGeneratorsInFlight = 0;
};
