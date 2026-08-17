/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomStreamingCache.h
 *
 ****************************************************************************/

#ifdef NOT_USED_YET

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"

#include "Atom.h"

enum EAtomChunkLoadResult
{

};

struct FAtomSoundWaveProxy
{

};

struct FAtomChunkHandle
{

};

// an LRU cache 
struct FAtomChunkCache
{
	struct FChunkKey
	{
	public:

		FChunkKey() {};
		FChunkKey(const FChunkKey& Other);
		FChunkKey(
			const FAtomSoundWaveProxyPtr& InSoundWave
			, uint32 InChunkIndex
#if WITH_EDITOR
			, uint32 InChunkRevision = 0
#endif
		);

		FChunkKey& operator=(const FChunkKey& Other);

	private:

		TWeakPtr<FAtomSoundWaveProxy, ESPMode::ThreadSafe> SoundWaveProxyWeakPtr;

	public:

		FName SoundWaveName = FName();
		uint32 ChunkIndex = INDEX_NONE;
		FObjectKey ObjectKey = FObjectKey();

#if WITH_EDITOR
		// This is used in the editor to invalidate stale compressed chunks.
		uint32 ChunkRevision = 0;
#endif
		inline bool operator==(const FChunkKey& Other) const;

		bool IsChunkStale();
		bool IsSoundWaveValid() const { return SoundWaveProxyWeakPtr.IsValid(); }
		// Forward FSoundWaveProxy interface
		EAtomSoundWaveLoadingBehavior GetLoadingBehavior() const;
		FStreamedAudioChunk& GetChunk(uint32 InChunkIndex) const;
		uint32 GetNumChunks() const;
		bool IsRetainingAudio() const;
		void ReleaseAudio();
		bool WasLoadingBehaviorOverridden() const;

		/** Hash function */
		friend uint32 GetTypeHash(const FChunkKey& InChunkKey)
		{
			// We can use HashCombineFast since we're hashing FName and they won't give the same result across different engine restart anyway.
			uint32 Hash = HashCombineFast(HashCombineFast(GetTypeHash(InChunkKey.SoundWaveName), InChunkKey.ChunkIndex), GetTypeHash(InChunkKey.ObjectKey));
#if WITH_EDITOR
			Hash = HashCombineFast(Hash, InChunkKey.ChunkRevision);
#endif
			return Hash;
		}
	};

	// Struct containing a single element in our LRU Cache.  
	struct FCacheElement
	{
		FChunkKey Key;
		uint8* ChunkData;
		uint32 ChunkDataSize;
		FCacheElement* MoreRecentElement;
		FCacheElement* LessRecentElement;
		uint64 CacheLookupID;

		FThreadSafeBool bIsLoaded;

		// How many disparate consumers have called GetLoadedChunk.
		FThreadSafeCounter NumConsumers;

#if WITH_EDITORONLY_DATA
		TUniquePtr<FAsyncStreamDerivedChunkTask> DDCTask;
#endif

		// Handle to our async read request operation.
		IBulkDataIORequest* ReadRequest;

		FCacheElement(uint32 MaxChunkSize, uint32 InCacheIndex)
			: ChunkData(nullptr)
			, ChunkDataSize(0)
			, MoreRecentElement(nullptr)
			, LessRecentElement(nullptr)
			, CacheLookupID(InCacheIndex)
			, bIsLoaded(false)
			, ReadRequest(nullptr)
		{
		}

		void WaitForAsyncLoadCompletion(bool bCancel)
		{
#if WITH_EDITORONLY_DATA
			if (DDCTask.IsValid() && !DDCTask->IsDone())
			{
				if (bCancel)
				{
					DDCTask->Cancel();
				}

				DDCTask->EnsureCompletion(false);
			}
#endif

			// Take ownership and close the storage
			IBulkDataIORequest* LocalReadRequest = (IBulkDataIORequest*)FPlatformAtomics::InterlockedExchangePtr((void* volatile*)&ReadRequest, (void*)0x1);

			if (LocalReadRequest && (void*)LocalReadRequest != (void*)0x1)
			{
				if (bCancel)
				{
					LocalReadRequest->Cancel();
				}

				LocalReadRequest->WaitCompletion();
				delete LocalReadRequest;
			}
		}

		bool IsLoadInProgress() const { return !bIsLoaded; }

		bool IsInUse() const { return NumConsumers.GetValue() > 0; }

		bool CanEvictChunk() const { return !IsInUse() && !IsLoadInProgress(); }

		~FCacheElement()
		{
			WaitForAsyncLoadCompletion(true);
			checkf(NumConsumers.GetValue() == 0, TEXT("Tried to destroy streaming cache while the cached data was in use!"));
			if (ChunkData)
			{
				FMemory::Free(ChunkData);
			}

			ChunkData = nullptr;
		}
	};

	// Our actual memory pool.
	TArray<FCacheElement> CachePool;
	FCacheElement* MostRecentElement;
	FCacheElement* LeastRecentElement;

	// This is incremented on every call of InsertChunk until we hit CachePool.Num() or MemoryCounterBytes hits MemoryLimitBytes.
	int32 ChunksInUse;

	// This counter is used to start evicting chunks before we hit CachePool.Num().
	TAtomic<uint64> MemoryCounterBytes;
	uint64 MemoryLimitBytes;

	// Number of async load operations we have currently in flight.
	FThreadSafeCounter NumberOfLoadsInFlight;

	// Critical section: only used when we are modifying element positions in the cache. This only happens in TouchElement, EvictLeastRecentChunk, and TrimMemory.
	// Individual cache elements should be thread safe to access.
	FCriticalSection CacheMutationCriticalSection;

	// Map that USoundWaves, FSoundWaveProxys, FChunkKeys, and FAudioChunkHandles can use to
	// quickly lookup where their chunks are currently stored in the cache
	TMap<FChunkKey, uint64> CacheLookupIdMap;

	// Returns cached element if it exists in our cache, nullptr otherwise.
	// If the index of the element is already known, it can be used here to avoid searching the cache.
	FCacheElement* FindElementForKey(const FChunkKey& InKey);
	FCacheElement* LinearSearchCacheForElement(const FChunkKey& InKey);
	FCacheElement* LinearSearchChunkArrayForElement(const FChunkKey& InKey);

	// Puts this element at the front of the linked list.
	void TouchElement(FCacheElement* InElement);

	// Inserts a new element into the cache, potentially evicting the oldest element in the cache.
	FCacheElement* InsertChunk(const FChunkKey& InKey);

	// This is called once we have more than one chunk in our cache:
	void SetUpLeastRecentChunk();

	// This is called in InsertChunk. it determines whether we should add a new chunk at the tail of the linked list or
	// evict the least recent chunk.
	bool ShouldAddNewChunk() const;

	// Returns the least recent chunk and fixes up the linked list accordingly.
	FCacheElement* EvictLeastRecentChunk(bool bBlockForPendingLoads = false);


	void KickOffAsyncLoad(FCacheElement* CacheElement, const FChunkKey& InKey, TFunction<void(EAudioChunkLoadResult)> OnLoadCompleted, ENamedThreads::Type CallbackThread, bool bNeededForPlayback);
	EAsyncIOPriorityAndFlags GetAsyncPriorityForChunk(const FChunkKey& InKey, bool bNeededForPlayback);

	// Calls OnLoadCompleted on current thread if CallbackThread == ENamedThreads::AnyThread, and dispatchs an async task on a named thread otherwise.
	static void ExecuteOnLoadCompleteCallback(EAudioChunkLoadResult Result, const TFunction<void(EAudioChunkLoadResult)>& OnLoadCompleted, const ENamedThreads::Type& CallbackThread);
};

// This is used to sort the cache array from smallest chunk size to biggest.
inline bool operator< (const FAtomChunkCache& Element1, const FAudioChunkCache& Element2)
{
	return Element1.MaxChunkSize < Element2.MaxChunkSize;
}

struct FAtomStreamingCacheParams
{
	struct FCacheDimensions
	{
		// The max size, in bytes, of a single chunk of compressed audio.
		// During cook, compressed audio assets will be chunked based on this amount.
		int32 MaxChunkSize;

		// The maximum number of elements stored in a single cache before it is evicted.
		// At runtime, this will be clamped to ensure that it is greater than the amount of
		// sources that can be playing simultaneously.
		int32 NumElements;

		// The maximum number of elements stored in a single cache before it is evicted.
		// At runtime, this will be clamped to ensure that it is greater than the amount of
		// sources that can be playing simultaneously.
		uint64 MaxMemoryInBytes;
	};

	// Most use cases will only use a single cache, but applications can optionally
	// use multiple LRU caches to reduce churn for specific types of sounds.
	// For example, an application can have
	// a cache for short sounds with room for many elements, and a separate cache
	// for longer sounds with fewer elements.
	TArray<FCacheDimensions> Caches;
};

class FAtomStreamingCache
{
public:

	FAtomStreamingCache(FAtomStreamingCacheParams Params);
	~FAtomStreamingCache();

	bool RequestChunk(const FAtomSoundWaveProxy& SoundWave, uint32 ChunkIndex, TFunction<void(EAtomChunkLoadResult)> OnLoadCompleted, ENamedThreads::Type ThreadToCallOnLoadCompletedOn, bool bForImmediatePlayback = false);
	FAtomChunkHandle GetLoadedChunk(const FAtomSoundWaveProxy& SoundWave, uint32 ChunkIndex, bool bBlockForLoad = false, bool bForImmediatePlayback = false) const;

private:

	// These are used to reference count consumers of audio chunks.
	virtual void AddReferenceToChunk(const FAudioChunkHandle& InHandle) override;
	virtual void RemoveReferenceToChunk(const FAudioChunkHandle& InHandle) override;

	/**
	 * Returns which cache this sound wave should be in,
	 * based on the size of this sound wave's chunk,
	 * or nullptr if MemoryLoadOnDemand is disabled.
	 */
	FAtomChunkCache* GetCacheForWave(const FAtomWaveProxyPtr& InSoundWave) const;
	FAtomChunkCache* GetCacheForChunkSize(uint32 InChunkSize) const;

	/**
	 * Returns the next chunk to kick off a load for, or INDEX_NONE if there is only one chunk to cache.
	 */
	int32 GetNextChunkIndex(const FSoundWaveProxyPtr& InSoundWave, uint32 CurrentChunkIndex) const;

	/** Audio chunk caches. These are set up on initialization. */
	TArray<FAudioChunkCache> CacheArray;
};

#endif
