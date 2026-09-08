
#pragma once

#include "Templates/TypeHash.h"

#include "Atom/Atom.h"

// Forward Definitions
class FAtomRuntime;

namespace Atom
{
	class FAtomExPlayerGroup;

	using FAtomPlaybackId = uint32;

	/** FAtomExPlayerKey
	 *
	 * Hashable unique Key for AtomExPlayer Sets.
	 */
	struct FAtomExPlayerKey
	{
	public:

		uint32 ObjectId = INDEX_NONE;
		uint32 InstanceId = INDEX_NONE;

		FAtomExPlayerKey()
			: InstanceId(InstanceIdCounter++)
		{
		}

		// For construction with a given unique id 
		FAtomExPlayerKey(uint32 InObjectId)
			: ObjectId(InObjectId)
		{
		}

		const bool IsValid() const
		{
			return ObjectId != INDEX_NONE || InstanceId != INDEX_NONE;
		}

		FORCEINLINE friend uint32 GetTypeHash(const FAtomExPlayerKey& Key)
		{
			return HashCombineFast(Key.ObjectId, Key.InstanceId);
		}

		FORCEINLINE friend bool operator==(const FAtomExPlayerKey& InLHS, const FAtomExPlayerKey& InRHS)
		{
			return (InLHS.ObjectId == InRHS.ObjectId) && (InLHS.InstanceId == InRHS.InstanceId);
		}

		FORCEINLINE friend bool operator!=(const FAtomExPlayerKey& InLHS, const FAtomExPlayerKey& InRHS)
		{
			return !(InLHS == InRHS);
		}

	private:

		static std::atomic<uint32> InstanceIdCounter;

	};

	struct FAtomExPlayerArgs
	{
		FAtomRuntime* AtomRuntime = nullptr;
		FAtomActiveSound* ActiveSound = nullptr;
		UAtomSoundBase* SoundData = nullptr;
	};

	/**
	 * FAtomExPlayer class
	 *
	 * Class which implements a mixer sound source controller object using for Playback Instancing.
	 */
	class FAtomExPlayer
	{
	public:

		struct IPlaybackListener
		{
			enum class EPlaybackEvent : uint8
			{
				AllocateAndVirtualized = 0,
				Realized,
				Virtualized,
				Removed
			};

		public:
			virtual ~IPlaybackListener() = default;

			virtual void OnPlaybackEvent(EPlaybackEvent PlaybackEvent) = 0;
			virtual void OnCueBlockIndexChanged(int32 BlockIndex) = 0;
			virtual void OnFilter(EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[]) = 0;
		};

		/** Constructors. */
		FAtomExPlayer();	
		FAtomExPlayer(const FAtomExPlayerArgs& InArgs, FCriAtomExPlayerPtr&& ExternalPlayer = FCriAtomExPlayerPtr());

		/** Destructor. */
		~FAtomExPlayer();

		/** ADX Api wrap */
		FAtomPlaybackId Prepare();
		void Resume(FAtomPlaybackId PlaybackID);
		void Pause(FAtomPlaybackId PlaybackID);
		void StopWithoutReleaseTime(FAtomPlaybackId PlaybackID);
		void Stop(FAtomPlaybackId PlaybackID);
		void UpdateParameters(FAtomPlaybackId PlaybackID);

		/** Executes event listener and return true if player is still playing some playback. */
		bool Update();

		inline bool IsInitialized() const { return bIsInitialized; }
		inline bool IsExternal() const { return bIsExternal; }
		inline void* GetNativeHandle() const { return ExPlayer.Get(); }
		inline FAtomRuntime* GetAtomRuntime() const { return AtomRuntime; }

		void RegisterPlaybackListener(TSharedRef<IPlaybackListener> PlaybackListener, FAtomPlaybackId PlaybackID);
		void UnregisterPlaybackListener(FAtomPlaybackId PlaybackID);
		void EnablePcmFilter(bool bEnable);

		static TSharedPtr<FAtomExPlayer, ESPMode::ThreadSafe> Create(const FAtomExPlayerArgs& Args);
		static TSharedPtr<FAtomExPlayer, ESPMode::ThreadSafe> CreateWithExternalPlayer(const FAtomExPlayerArgs& Args, FCriAtomExPlayerPtr&& ExternalPlayer);

		/*struct FExSource
		{
		public:

			FExSource()
			{
				CriAtomEx3dSourceConfig Config;
				criAtomEx3dSource_SetDefaultConfig(&Config);
				ExSource = MakeCriHandle(FCriWareApi::criAtomEx3dSource_Create(&Config, nullptr, 0));
			}

			~FExSource() = default;

			operator CriAtomEx3dSourceHn() const { return ExSource.Get(); }

		private:

			FCriAtomExSourcePtr ExSource;
		};*/

		void ResetSpatialization(const FAtomPlaybackInstance& PlaybackInstance);
		bool UpdateSpatialization(const FAtomPlaybackInstance& PlaybackInstance);

	private:

		struct FPlaybackEventInfo
		{
			CriAtomExPlaybackEvent Event = (CriAtomExPlaybackEvent)0;
			FAtomPlaybackId PlaybackID = INDEX_NONE;
		};

		void ProcessEvent(FPlaybackEventInfo EventInfo);

		void AddSpatialization(const FAtomPlaybackInstance& PlaybackInstance);
		void RemoveSpatialization();
		void Update3DSource(FCriAtomExSourcePtr& InExSource, const FTransform& InTransform, const FAtomAttenuationSettings* InAttenuationSettings = nullptr, bool bSpatialize = false, bool bForceAttenuate = false);

		/** the atom runtime. */
		FAtomRuntime* AtomRuntime;

		/** the linked active sound. */
		FAtomActiveSound* ActiveSound;

		/** the lock for player handle. */
		FCriticalSection PlayerLock;

		/** The native AtomEx player handle. */
		FCriAtomExPlayerPtr ExPlayer;

		/** The native Status of the player. */
		CriAtomExPlayerStatus Status;

		FThreadSafeBool bIsExternal;

		uint8 bIsInitialized : 1;
		uint8 bUsePcmFilter : 1;
		uint8 bIs3D : 1;

		/** The playback event queue */
		TQueue<FPlaybackEventInfo, EQueueMode::Mpsc> QueuedEvents;

		FCriticalSection PlaybackListenersLock;

		// redirect player playaback events to registerd listener 
		TMap<FAtomPlaybackId, TSharedRef<IPlaybackListener>> PlaybackListeners;

		/** The native AtomEx source handle if 3d sound. */
		FCriAtomExSourcePtr ExSource;

		/** The native AtomEx source list handle if using multiple 3d sound positions. */
		FCriAtomExSourceListPtr ExSourceList;

		/** The native AtomEx source handles if using multiple 3d sound positions. */
		TArray<FCriAtomExSourcePtr> ExSources;

		/** The player group this player is attached. */
		TSharedPtr<FAtomExPlayerGroup> PlayerGroup;

		friend FAtomExPlayback;

	public:

		// event callbacks from ExPlayer
		void HandleNativeOnPlaybackEvent(CriAtomExPlaybackEvent PlaybackEvent, const CriAtomExPlaybackInfoDetail* Info);
		void HandleNativeOnCueBlockIndexChanged(CriAtomExPlaybackId PlaybackID, CriAtomExBlockIndex BlockIndex);
		void HandleNativePlayerOnFilter(CriAtomExPlaybackId PlaybackID, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumFrames, void* Data[]);

		// utility
		static void GetPlaybackInfo(FAtomPlaybackInstance* PlaybackInstance, FAtomPlaybackId PlaybackID);
	};

	/** Handy struct for a source */
	struct FMixerSourcePlayer
	{
		// AtomExPlayer Object which handles source buffer decoding and playback
		TSharedPtr<FAtomExPlayer, ESPMode::ThreadSafe> Player = nullptr;

		// ID for source playback from associated AtomExPlayer.
		FAtomPlaybackId PlaybackID = (uint32)INDEX_NONE;
	};

	/**
	 * FAtomExPlayerGroup class
	 *
	 * Class which implements AtomEx "sound object" used for grouping voice and playback to scope limitation from Atom Craft paramters.
	 */
	class FAtomExPlayerGroup
	{
	public:

		FAtomExPlayerGroup(bool bEnableVoiceLimitScope = false, bool bEnableCategoryCueLimitScope = false);

		inline bool IsVoiceLimitScopeEnabled() const { return bEnableVoiceLimitScope; }
		inline bool IsCategoryCueLimitScopeEnabled() const { return bEnableCategoryCueLimitScope; }

	private:

		void AddPlayer(FAtomExPlayer& Player);
		void RemovePlayer(FAtomExPlayer& Player);

		FCriAtomExSoundObjectPtr ExSoundObject;

		bool bEnableVoiceLimitScope;
		bool bEnableCategoryCueLimitScope;

		friend FAtomExPlayer;
	};
} // namespace
