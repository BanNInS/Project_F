/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomStreamingCache.cpp
 *
 ****************************************************************************/

#ifdef TOTO

#include "Atom/AtomStreamingCache.h"

FAtomStreamingCache::FAtomStreamingCache()
{

}

FAtomStreamingCache::~FAtomStreamingCache()
{

}

bool FAtomStreamingCache::RequestChunk(const FAtomSoundWaveProxy& SoundWave, uint32 ChunkIndex, TFunction<void(EAtomChunkLoadResult)> OnLoadCompleted, ENamedThreads::Type ThreadToCallOnLoadCompletedOn, bool bForImmediatePlayback = false)
{
	return true;
}

FAtomChunkHandle FAtomStreamingCache::GetLoadedChunk(const FAtomSoundWaveProxyPtr& SoundWave, uint32 ChunkIndex, bool bBlockForLoad, bool bForImmediatePlayback) const
{
	if (!ensure(SoundWave.IsValid()))
	{
		return {};
	}

	// If this sound wave is managed by a cache, use that to get the chunk:
	//FAudioChunkCache* Cache = GetCacheForWave(SoundWave);
	FAtomChunkCache Cache = MainCache;
	if (Cache)
	{
		// With this code, the zeroth chunk should never get hit.
		checkf(ChunkIndex != 0, TEXT("Decoder tried to access the zeroth chunk through the streaming manager. Use USoundWave::GetZerothChunk() instead."));

		FAtomChunkCache::FChunkKey ChunkKey(
			SoundWave
			, ChunkIndex
	#if WITH_EDITOR
			, (uint32)SoundWave->GetCurrentChunkRevision()
	#endif
		);

		if (!FAtomChunkCache::IsKeyValid(ChunkKey))
		{
			UE_LOG(LogAudioStreamCaching, Warning, TEXT("Invalid Chunk Index %d Requested for Wave %s!"), ChunkIndex, *SoundWave->GetFName().ToString());
			return FAudioChunkHandle();
		}

		// The function call below increments the reference count to the internal chunk.
		uint64 LookupIDForChunk = Cache->GetCacheLookupIDForChunk(ChunkKey);
		TArrayView<uint8> LoadedChunk = Cache->GetChunk(ChunkKey, bBlockForLoad, (bForImmediatePlayback || bBlockForLoad), LookupIDForChunk);

		// Ensure that, if we requested a synchronous load of this chunk, we didn't fail to load said chunk.
		UE_CLOG(bBlockForLoad && !LoadedChunk.GetData(), LogAudioStreamCaching, Display, TEXT("Synchronous load of chunk index %d for AtomSoundWave %s failed to return any data. Likely because the cache was blown."), ChunkIndex, *SoundWave->GetFName().ToString());

		// Set the updated cache offset for this chunk index.
		Cache->SetCacheLookupIDForChunk(ChunkKey, LookupIDForChunk);

		UE_CLOG(!bBlockForLoad && !LoadedChunk.GetData(), LogAudioStreamCaching, Verbose, TEXT("GetLoadedChunk called for chunk index %d of AtomSoundWave %s when audio was not loaded yet. This will result in latency."), ChunkIndex, *SoundWave->GetFName().ToString());

		// Finally, if there's a chunk after this in the sound, request that it is in the cache.
		const int32 NextChunk = GetNextChunkIndex(SoundWave, ChunkIndex);

		if (NextChunk != INDEX_NONE)
		{
			FAtomChunkCache::FChunkKey NextChunkKey(
				SoundWave
				, ((uint32)NextChunk)
	#if WITH_EDITOR
				, (uint32)SoundWave->GetCurrentChunkRevision()
	#endif
			);

			uint64 LookupIDForNextChunk = Cache->AddOrTouchChunk(NextChunkKey, [](EAudioChunkLoadResult) {}, ENamedThreads::AnyThread, false);
			if (LookupIDForNextChunk == InvalidAudioStreamCacheLookupID)
			{
				UE_LOG(LogAudioStreamCaching, Warning, TEXT("Failed to add follow-up chunk for %s. This should not happen with our current TrimMemory() logic"), *SoundWave->GetFName().ToString());
			}
			else
			{
				Cache->SetCacheLookupIDForChunk(NextChunkKey, LookupIDForNextChunk);
			}
		}

		return BuildChunkHandle(LoadedChunk.GetData(), LoadedChunk.Num(), SoundWave, SoundWave->GetFName(), ChunkIndex, LookupIDForChunk);
	}
	//else
	//{
	//	 ensureMsgf(false, TEXT("Failed to find cache for wave %s. Are you sure this is a streaming wave?"), *SoundWave->GetFName().ToString());
	//	 return FAudioChunkHandle();
	//}
}

#endif
