#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "AtomConcurrency.generated.h"

CRIWARECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomConcurrency, Log, All);

// Forward Definitions
class FAtomRuntime;
class FAtomConcurrencyManager;
struct FAtomActiveSound;

/** Sound concurrency group ID. */
using FAtomConcurrencyGroupId = uint32;

/** Sound concurrency unique object IDs. */
using FAtomConcurrencyObjectId = uint32;

/** Sound owner object IDs */
using FAtomSoundOwnerObjectId = uint32;

/** Sound instance (UAtomSoundBase) object ID. */
using FAtomSoundObjectId = uint32;

UENUM()
enum class EAtomMaxConcurrentResolutionRule
{
	/** When Max Concurrent sounds are active do not start a new sound. */
	PreventNew,

	/** When Max Concurrent sounds are active stop the oldest and start a new one. */
	StopOldest,

	/** When Max Concurrent sounds are active stop the furthest sound.  If all sounds are the same distance then do not start a new sound. */
	StopFarthestThenPreventNew,

	/** When Max Concurrent sounds are active stop the furthest sound.  If all sounds are the same distance then stop the oldest. */
	StopFarthestThenOldest,

	/** Stop the lowest priority sound in the group. If all sounds are the same priority, then it will stop the oldest sound in the group. */
	StopLowestPriority,

	/** Stop the sound that is quietest in the group. */
	StopQuietest,

	/** Stop the lowest priority sound in the group. If all sounds are the same priority, then it won't play a new sound. */
	StopLowestPriorityThenPreventNew,

	Count UMETA(Hidden)
};

inline const TCHAR* ToString(EAtomMaxConcurrentResolutionRule Rule)
{
	switch (Rule) {
	case EAtomMaxConcurrentResolutionRule::PreventNew:							return TEXT("PreventNew");
	case EAtomMaxConcurrentResolutionRule::StopOldest:							return TEXT("StopOldest");
	case EAtomMaxConcurrentResolutionRule::StopFarthestThenPreventNew:			return TEXT("StopFarthestThenPreventNew");
	case EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest:				return TEXT("StopFarthestThenOldest");
	case EAtomMaxConcurrentResolutionRule::StopLowestPriority:					return TEXT("StopLowestPriority");
	case EAtomMaxConcurrentResolutionRule::StopQuietest:						return TEXT("StopQuietest");
	case EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew:	return TEXT("StopLowestPriorityThenPreventNew");
	default:						return TEXT("Unknown");
	}
}

UENUM()
enum class EAtomConcurrencyVolumeScaleMode
{
	/* Scales volume of older sounds more than newer sounds (default) */
	Default = 0,

	/* Scales distant sounds by volume scalar more than closer sounds */
	Distance,

	/* Scales lower priority sounds by volume scalar more than closer sounds */
	Priority
};

USTRUCT(BlueprintType)
struct FAtomConcurrencySettings
{
	GENERATED_BODY()

public:

	/** The max number of allowable concurrent active voices for voices playing in this concurrency group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Concurrency, meta = (UIMin = "1", ClampMin = "1"))
	int32 MaxCount;

	/* Whether or not to limit the concurrency to per sound owner (i.e. the actor that plays the sound). If the sound doesn't have an owner, it falls back to global concurrency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Concurrency)
	uint8 bLimitToOwner : 1;

	/**
	 * Whether or not volume scaling can recover volume ducking behavior when concurrency group sounds stop (default scale mode only).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Scaling", meta = (DisplayName = "Can Recover", EditCondition = "VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default"))
	uint8 bVolumeScaleCanRelease : 1;

	/** Which concurrency resolution policy to use if max voice count is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Concurrency)
	EAtomMaxConcurrentResolutionRule ResolutionRule;

	/** Amount of time to wait (in seconds) between different sounds which play with this concurrency. Sounds rejected from this will ignore virtualization settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Concurrency, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float RetriggerTime;

private:

	/**
	 * Ducking factor to apply per older voice instance (generation), which compounds based on scaling mode
	 * and (optionally) revives them as they stop according to the provided attack/release times.
	 *
	 * Note: This is not applied until after StopQuietest rules are evaluated, in order to avoid thrashing sounds.
	 *
	 * AppliedVolumeScale = Math.Pow(DuckingScale, VoiceGeneration)
	 */
	UPROPERTY(EditAnywhere, Category = "Volume Scaling", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float VolumeScale;

public:

	/** Volume Scale mode designating how to scale voice volume based on number of member sounds active in group. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Scaling")
	EAtomConcurrencyVolumeScaleMode VolumeScaleMode;

	/**
	 * Time taken to apply duck using volume scalar.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Scaling", meta = (DisplayName = "Duck Time", UIMin = "0.0", ClampMin = "0.0", UIMax = "10.0", ClampMax = "1000000.0"))
	float VolumeScaleAttackTime;

	/**
	 * Time taken to recover volume scalar duck.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Scaling", meta = (DisplayName = "Recover Time", EditCondition = "bVolumeScaleCanRelease && VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default", UIMin = "0.0", ClampMin = "0.0", UIMax = "10.0", ClampMax = "1000000.0"))
	float VolumeScaleReleaseTime;

	/**
	 * Time taken to fade in if voice was evicted due to another voice in the group recovering.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Stealing", meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1000000.0"))
	float VoiceStealAttackTime;

	/**
	 * Time taken to fade out if voice is evicted or culled due to another voice in the group starting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Stealing", meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1000000.0"))
	float VoiceStealReleaseTime;

private:

#if !UE_BUILD_SHIPPING
	/** Name given to the settings for debug purpose. */
	FName DebugName = NAME_None;
#endif

public:

	FAtomConcurrencySettings()
		: MaxCount(16)
		, bLimitToOwner(0)
		, bVolumeScaleCanRelease(0)
		, ResolutionRule(EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest)
		, RetriggerTime(0.0f)
		, VolumeScale(1.0f)
		, VolumeScaleMode(EAtomConcurrencyVolumeScaleMode::Default)
		, VolumeScaleAttackTime(0.01f)
		, VolumeScaleReleaseTime(0.5f)
		, VoiceStealAttackTime(0.0f)
		, VoiceStealReleaseTime(0.0f)
	{}

	/**
	 * Retrieves the volume scale
	 */
	CRIWARECORE_API float GetVolumeScale() const;

	/** Whether or not ResolutionRule supports eviction, wherein eviction is the ability to keep a sound
	  * from playing prior to start and culling is the requirement of a sound to initialize and actively parse
	  * prior to being removed from a concurrency group.
	  */
	CRIWARECORE_API bool IsEvictionSupported() const;

#if !UE_BUILD_SHIPPING

	/** Sets a name that can be used to identify the settings. */
	CRIWARECORE_API void SetDebugName(const FName& Name) { DebugName = Name; }

	/** Returns a name that can be used to identify the settings. */
	CRIWARECORE_API const FName& GetDebugName() const { return DebugName; }
#endif
};

UCLASS(BlueprintType, HideCategories = Object, EditInlineNew, MinimalAPI)
class UAtomConcurrency : public UObject
{ 
	GENERATED_BODY() 

public:

	UAtomConcurrency(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Settings, meta = (ShowOnlyInnerProperties))
	FAtomConcurrencySettings Concurrency;

	virtual void PostInitProperties() override;
};

/** How the concurrency request is handled by the concurrency manager */
enum class EAtomConcurrencyMode : uint8
{
	Group,
	Owner,
	OwnerPerSound,
	Sound,
};

/** Handle to all required data to create and catalog a concurrency group */
struct FAtomConcurrencyHandle
{
	const FAtomConcurrencySettings& Settings;
	const FAtomConcurrencyObjectId ObjectID;
	const bool bIsOverride;

	/** Constructs a handle from concurrency override settings */
	FAtomConcurrencyHandle(const FAtomConcurrencySettings& InSettings);

	/** Constructs a handle to a concurrency asset */
	FAtomConcurrencyHandle(const UAtomConcurrency& Concurrency);

	EAtomConcurrencyMode GetMode(const FAtomActiveSound& ActiveSound) const;
};

/** Sound instance data pertaining to concurrency tracking */
struct FAtomConcurrencySoundData
{
	int32 Generation;
	float LerpTime;

private:

	float Elapsed;
	float DbTargetVolume;
	float DbStartVolume;

public:

	FAtomConcurrencySoundData()
		: Generation(0)
		, LerpTime(0.0f)
		, Elapsed(0.0f)
		, DbTargetVolume(0.0f)
		, DbStartVolume(0.0f)
	{
	}

	void Update(float InElapsed);

	float GetLerpTime() const;
	float GetVolume(bool bInDecibels = false) const;
	float GetTargetVolume(bool bInDecibels = false) const;

	void SetTarget(float InTargetVolume, float InLerpTime);
};


/** Class which tracks array of active sound pointers for concurrency management */
class FAtomConcurrencyGroup
{
	/** Array of active sounds for this concurrency group. */
	TArray<FAtomActiveSound*> ActiveSounds;

	FAtomConcurrencyGroupId GroupID;
	FAtomConcurrencyObjectId ObjectID;
	FAtomConcurrencySettings Settings;

	/** When a sound last played on this concurrency group. */
	float LastTimePlayed = 0.0f;

public:

	/** Constructor for the max concurrency active sound entry. */
	FAtomConcurrencyGroup(FAtomConcurrencyGroupId GroupID, const FAtomConcurrencyHandle& ConcurrencyHandle);

	static FAtomConcurrencyGroupId GenerateNewID();

	/** Returns the active sounds array. */
	const TArray<FAtomActiveSound*>& GetActiveSounds() const { return ActiveSounds; }

	/** Returns the id of the concurrency group */
	FAtomConcurrencyGroupId GetGroupID() const { return GroupID; }

	/** Returns the current generation (effectively, the number of concurrency sound instances active) */
	const int32 GetNextGeneration() const { return ActiveSounds.Num(); }

	/** Returns the settings associated with the group */
	const FAtomConcurrencySettings& GetSettings() const { return Settings; }

	/** Returns the parent object ID */
	FAtomConcurrencyObjectId GetObjectID() const { return ObjectID; }

	/** Determines if the group is full. */
	bool IsEmpty() const { return ActiveSounds.Num() == 0; }

	/** Determines if the group is full. */
	bool IsFull() const { return Settings.MaxCount <= ActiveSounds.Num(); }

	/** Adds an active sound to the active sound array. */
	void AddActiveSound(FAtomActiveSound& ActiveSound);

	/** Removes an active sound from the active sound array. */
	void RemoveActiveSound(FAtomActiveSound& ActiveSound);

	/** Updates volume based on distance generation if set as VolumeScaleMode */
	void UpdateGeneration(FAtomActiveSound* NewActiveSound = nullptr);

	/** Sorts the active sound if concurrency settings require culling post playback */
	void CullSoundsDueToMaxConcurrency();

	/** Sets when the last time a sound was played on this concurrency group. */
	void SetLastTimePlayed(float InLastTimePlayed) { LastTimePlayed = InLastTimePlayed; }

	/** Whether or not a sound would be rate limited if it tried to play right now. */
	bool CanPlaySoundNow(float InCurrentTime) const;

};

typedef TMap<FAtomConcurrencyGroupId, FAtomConcurrencyGroup*> FAtomConcurrencyGroups;

struct FAtomSoundInstanceEntry
{
	TMap<FAtomSoundObjectId, FAtomConcurrencyGroupId> SoundInstanceToConcurrencyGroup;

	FAtomSoundInstanceEntry(FAtomSoundObjectId SoundObjectID, FAtomConcurrencyGroupId GroupID)
	{
		SoundInstanceToConcurrencyGroup.Add(SoundObjectID, GroupID);
	}
};

/** Type for mapping an object id to a concurrency entry. */
typedef TMap<FAtomConcurrencyObjectId, FAtomConcurrencyGroupId> FAtomConcurrencyMap;

struct FAtomOwnerConcurrencyMapEntry
{
	FAtomConcurrencyMap ConcurrencyObjectToConcurrencyGroup;

	FAtomOwnerConcurrencyMapEntry(FAtomConcurrencyObjectId ConcurrencyObjectID, FAtomConcurrencyGroupId GroupID)
	{
		ConcurrencyObjectToConcurrencyGroup.Add(ConcurrencyObjectID, GroupID);
	}
};

/** Maps owners to concurrency maps */
typedef TMap<FAtomSoundOwnerObjectId, FAtomOwnerConcurrencyMapEntry> FAtomOwnerConcurrencyMap;

/** Maps owners to sound instances */
typedef TMap<FAtomSoundOwnerObjectId, FAtomSoundInstanceEntry> FAtomOwnerPerSoundConcurrencyMap;

/** Maps sound object ids to active sound array for global concurrency limiting */
typedef TMap<FAtomSoundObjectId, FAtomConcurrencyGroupId> FAtomPerSoundToActiveSoundsMap;


class FAtomConcurrencyManager
{
public:

	FAtomConcurrencyManager(FAtomRuntime* InAtomRuntime);
	CRIWARECORE_API ~FAtomConcurrencyManager();

	/** Returns a newly allocated active sound given the input active sound struct. Will return nullptr if the active sound concurrency evaluation doesn't allow for it. */
	FAtomActiveSound* CreateNewActiveSound(const FAtomActiveSound& NewActiveSound, bool bIsRetriggering);

	/** Try to add an existing active sound. Will return false if the active sound concurrency evaluation doesn't allow for it. */
	bool AddActiveSound(FAtomActiveSound& ActiveSound);

	/** Evaluates whether active sound can be added to concurency tracking. Will return false if the active sound concurrency evaluation doesn't allow for it.*/
	bool EvaluateActiveSound(const FAtomActiveSound& ActiveSound, bool bIsRetriggering);

	/** Removes the active sound from concurrency tracking when active sound is stopped. */
	void RemoveActiveSound(FAtomActiveSound& ActiveSound);

	/** Stops sound, applying concurrency rules for how to stop. */
	void StopDueToVoiceStealing(FAtomActiveSound& ActiveSound);

	/** Updates generations for concurrency groups set to scale active sound volumes by distance or priority */
	void UpdateVolumeScaleGenerations();

	/** Culls any active sounds due to max concurrency sound resolution rule constraints being met */
	void UpdateSoundsToCull();

#if !UE_BUILD_SHIPPING
	/** */
	CRIWARECORE_API void IterateOverConcurrencyGroups(TFunctionRef<void(FAtomConcurrencyGroup*)> Func) const;
#endif

	void ResetConcurrency();

private:

	/** Evaluates whether or not the sound can play given the concurrency group's rules. Appends permissible
	sounds to evict in order for sound to play (if required) and returns the desired concurrency group. */
	FAtomConcurrencyGroup* CanPlaySound(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroupId GroupID, TArray<FAtomActiveSound*>& OutSoundsToEvict, bool bIsRetriggering, bool bIsRealizing, bool& bOutEvictNew);

	/** Creates a new concurrency group and returns pointer to said group */
	FAtomConcurrencyGroup& CreateNewConcurrencyGroup(const FAtomConcurrencyHandle& ConcurrencyHandle);

	/** Creates new concurrency groups from handle array */
	void CreateNewGroupsFromHandles(
		const FAtomActiveSound& NewActiveSound,
		const TArray<FAtomConcurrencyHandle>& ConcurrencyHandles,
		TArray<FAtomConcurrencyGroup*>& OutGroupsToApply
	);

	/**  Creates an active sound to play, assigning it to the provided concurrency groups, and evicting required sounds */
	FAtomActiveSound* CreateAndEvictActiveSounds(FAtomActiveSound& NewActiveSound, const TArray<FAtomConcurrencyGroup*>& GroupsToApply, const TArray<FAtomActiveSound*>& SoundsToEvict, bool bIsRealizing, bool bEvictNewActiveSound);

	/** Finds an active sound able to be evicted based on the provided concurrency settings. */
	FAtomActiveSound* GetEvictableSound(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering);
	
	/** Helper functions for finding evictable sounds for the given concurrency rule. */
	FAtomActiveSound* GetEvictableSoundStopFarthest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const;
	FAtomActiveSound* GetEvictableSoundStopOldest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const;
	FAtomActiveSound* GetEvictableSoundStopQuietest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const;
	FAtomActiveSound* GetEvictableSoundStopLowestPriority(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const;
	/** Handles concurrency evaluation that happens per UAtomConcurrencyObject. */
	FAtomActiveSound* EvaluateConcurrency(FAtomActiveSound& NewActiveSound, TArray<FAtomConcurrencyHandle>& ConcurrencyHandles, bool bIsRetriggering, bool bIsRealizing);

	/** concurrency evaluation. */
	bool EvaluateConcurrencyInternal(
		const FAtomActiveSound& NewActiveSound, 
		TArray<FAtomConcurrencyHandle>& ConcurrencyHandles, 
		TArray<FAtomConcurrencyGroup*>& GroupsToApply, 
		TArray<FAtomActiveSound*>& SoundsToEvictbool, 
		bool bIsRetriggering,
		bool bIsRealizing,
		bool& bOutEvictNew);

private:

	/** Owning Atom runtime ptr for the concurrency manager. */
	FAtomRuntime* AtomRuntime;

	/** Global concurrency map that maps individual sounds instances to shared UAtomConcurrency UObjects. */
	FAtomConcurrencyMap ConcurrencyMap;

	FAtomOwnerConcurrencyMap OwnerConcurrencyMap;

	/** A map of owners to concurrency maps for sounds which are concurrency-limited per sound owner. */
	FAtomOwnerPerSoundConcurrencyMap OwnerPerSoundConcurrencyMap;

	/** Map of sound objects concurrency-limited globally */
	FAtomPerSoundToActiveSoundsMap SoundObjectToConcurrencyGroup;

	/** A map of concurrency active sound ID to concurrency active sounds */
	FAtomConcurrencyGroups ConcurrencyGroups;
};
