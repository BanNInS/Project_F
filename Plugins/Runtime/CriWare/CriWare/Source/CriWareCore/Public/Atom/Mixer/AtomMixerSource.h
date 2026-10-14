
#pragma once

#include "Atom/Atom.h"
#include "Atom/Modulation/AtomModulationDestination.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/AtomSoundSourceBusSend.h"

// Forward Definitions
class FAtomRuntime;

namespace Atom
{
	// Forward Definitions
	class FMixerSourceVoice;
	class FAtomExPlayer;

	/** Base class for sound sources that uses mixer ("Ex" in CriAtom) and source voices. */
	class FMixerSource
		: public FAtomSource
		, public ISourceListener
	{
	public:

		FMixerSource(FAtomRuntime* InAtomRuntime);
		virtual ~FMixerSource();

		//~ Begin ISourceListener Interface
		virtual void OnBeginGenerate() override {}
		//virtual void OnLoopEnd() override {}
		virtual void OnDone() override {}
		//virtual void OnEffectTailsDone() override {}
		//~ End ISourceListener Interface

	protected:

		enum EFreeReason
		{
			Deleter,
			Stop,
			Error,
		};

		inline const TCHAR* ToString(EFreeReason Value)
		{
			switch (Value) {
			case EFreeReason::Deleter:	return TEXT("Deleter");
			case EFreeReason::Stop:		return TEXT("Stop");
			case EFreeReason::Error:	return TEXT("Error");
			default:					return TEXT("Unknown");
			}
		}

		virtual void SetupSourceBusData(TArray<FInitAudioBusSend>* OutAudioBusSends = nullptr, bool bEnableBusSends = true) {}

		virtual void FreeResources(EFreeReason InFreeReason) = 0;

		// Init the voice with MixerSourceManager.
		bool InitSourceVoice(FAtomPlaybackInstance* InPlaybackInstance);

		/** Whether or not we should create the source voice with the HRTF spatializer. */
		bool UseObjectBasedSpatialization() const;

		/** Whether or not existing or new sources will use the HRTF spatializer. */
		bool IsUsingObjectBasedSpatialization() const;

		/** Whether or not to use the source data override plugin */
		bool UseSourceDataOverridePlugin() const;

		int64 GetNumFramesPlayed() const;

		/** the explayer object that play may be null in case of using atomPlayer. */
		FMixerSourcePlayer MixerSourcePlayer;
		FMixerSourceVoice* MixerSourceVoice = nullptr;

		bool bDebugMode = false;
	};

	/**
	 * FAtomPlayer class
	 * 
	 * Class which implements a simplest sound source object using an Atom Player (CriAtomPlayer is a voice by itself).
	 */
	class FAtomPlayer : public FAtomSource
	{
	public:

		FAtomPlayer(FAtomRuntime* InAtomRuntime);
		~FAtomPlayer();

		//~ Begin FAtomSource Interface
		void* GetNativeHandle() const override { return Player.Get(); }
		FString Describe(bool bUseLongName) override { return bUseLongName ? TEXT("Atom Player") : TEXT("Player"); }
		bool IsVoiceSource() const override { return true; }
		bool PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance) override;
		bool IsPreparedToInit() override;
		bool Init(FAtomPlaybackInstance* InPlaybackInstance) override;
		void Update() override;
		void Play() override;
		void Pause() override;
		void Stop() override;
		void StopNow() override;
		bool IsStopping() override;
		bool IsFinished() override;
		//~ End FAtomSource Interface 

	protected:

		void FreeResources();

		bool IsPlaybackFinished();

		/** Updates the volume parameter set from the game thread. */
		void UpdateVolume();

		FCriAtomPlayerPtr Player;

		CriAtomPlayerStatus Status;

		/** Task used to create player. */
		FGraphEventRef CreatePlayerTask;

		FThreadSafeBool bIsPreparingForInit; // global prepare status -> replace with InitializationState
		FThreadSafeBool bIsLoadingResource; // asset resource loading
		FThreadSafeBool bIsPreparingPlayer; // atom player preparing
		FThreadSafeBool bIsPrepareFailed; // on someting failed
		FThreadSafeBool bIsStopping;

	public:

		void HandleNativePlayerOnPcmDecode(CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[]);
	};

	/** State to track initialization stages. */
	enum class EAtomExPlaybackInitializationState : uint8
	{
		NotInitialized,
		Initializing,
		Initialized
	};

	/** Shared atom render event listener between AtomExPlayer and AtomExPlayback. */
	struct FAtomExPlaybackListener
		: public FAtomExPlayer::IPlaybackListener
		, public TSharedFromThis<FAtomExPlaybackListener>
	{
	public:
		FAtomExPlaybackListener(FAtomPlaybackId InPlaybackID, FAtomPlaybackInstance* InPlaybackInstance, FAtomExPlayback* InOwner)
			: PlaybackID(InPlaybackID)
			, PlaybackInstance(InPlaybackInstance)
			, Owner(InOwner)
		{}

		//~ Begin IPlaybackListener
		virtual void OnPlaybackEvent(EPlaybackEvent PlaybackEvent) override;
		virtual void OnCueBlockIndexChanged(int32 BlockIndex) override;
		virtual void OnFilter(EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[]) override;
		//~ End IPlaybackListener

	private:

		FAtomPlaybackId PlaybackID = INDEX_NONE;
		FAtomPlaybackInstance* PlaybackInstance = nullptr;
		FAtomExPlayback* Owner = nullptr;
	};

	/**
	 * FAtomExPlaback class
	 * 
	 * Class which implements a mixer sound source object using the advanced Atom ExPlayer as controller
	 * for a source unit represented by ExPlaybackID.
	 */
	class FAtomExPlayback : public FMixerSource
	{
	public:

		/** Constructor. */
		FAtomExPlayback(FAtomRuntime* InAtomRuntime);

		/** Destructor. */
		~FAtomExPlayback();

		//~ Begin FAtomSource Interface
		virtual void* GetNativeHandle() const override { return MixerPlayer->GetNativeHandle(); }
		virtual FString Describe(bool bUseLongName) override { return bUseLongName ? TEXT("AtomEx Playback") : TEXT("ExPlayback"); }
		virtual bool IsVoiceSource() const override { return false; }
		virtual bool PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance) override;
		virtual bool IsPreparedToInit() override;
		virtual bool Init(FAtomPlaybackInstance* InPlaybackInstance) override;
		virtual void Update() override;
		virtual void Play() override;
		virtual void Pause() override;
		virtual void Stop() override;
		virtual void StopNow() override;
		virtual bool IsStopping() override;
		virtual bool IsFinished() override;
		virtual float GetPlaybackPercent() const override;
		virtual float GetEnvelopeValue() const override;
		//~ End FAtomSource Interface

		//~ Begin IAtomSourceListener
		//virtual void OnBeginGenerate() override;
		//virtual void OnDone() override;
		//virtual void OnEffectTailsDone() override;
		//virtual void OnLoopEnd() override { bLoopCallback = true; };
		//~ End IAtomSourceListener

		/** Returns true if playback is finished playing. */
		bool IsPlaybackFinished();

		FAtomRuntime* GetAtomRuntime() const { return AtomRuntime; }

	protected:

		/** Initializes the source bus sends. */
		virtual void SetupSourceBusData(TArray<FInitAudioBusSend>* OutAudioBusSends = nullptr, bool bEnableBusSends = true) override;

		/** Frees any resources for this sound source. */
		virtual void FreeResources(EFreeReason InReason) override;

		bool LoadSoundInternal(FAtomPlaybackInstance* InPlaybackInstance);
		bool PrepareSoundInternal();

		void SetLooping();

		void SetCategories();
		void UnsetCategories();

		void AttachAisacPatches();
		void DetachAisacPatches();

		void SetCueSelectorLabels();
		void SetCueFirstBlockIndex();

		/** Updates the pitch parameter set from the game thread. */
		void UpdatePitch();

		/** Updates the volume parameter set from the game thread. */
		void UpdateVolume();

		/** Updates the spatialization information set from the game thread. */
		void UpdateSpatialization();

		/** Updates source effects of the sound. */
		void UpdateEffects();

		/** Updates the Modulation Routing settings on this voice. */
		void UpdateModulation();

		/** Updates Atom bus send levels based on game data. */
		void UpdateBusSends();

		/** Updates source bus send levels based on game data. */
		void UpdateSourceBusSends();

		/** Updates the channel map of the sound.*/
		void UpdateChannelMap();

		/** Updates all parameters used by sound cue.*/
		void UpdateCueParameters();

		bool ComputeChannelMap(const int32 NumSourceChannels);
		bool ComputeMonoChannelMap();
		bool ComputeStereoChannelMap();

	protected:

		/** the explayer object that generated this playback. */
		TSharedRef<FAtomExPlayer, ESPMode::ThreadSafe> MixerPlayer;

		/** The native PlaybackID of the first playback or first current from external source when attached. */
		FAtomPlaybackId PlaybackID;

		/** The native Status of the playback. */
		CriAtomExPlaybackStatus Status;

		EAtomExPlaybackInitializationState InitializationState; // global init state

		FThreadSafeBool bIsPreparingForInit; // global prepare status -> replace with InitializationState
		FThreadSafeBool bIsLoadingResource; // asset resource loading
		FThreadSafeBool bIsPreparingPlayer; // atom player preparing
		FThreadSafeBool bIsPrepareFailed; // on someting failed
		FThreadSafeBool bIsStopping;
		FThreadSafeBool bIsExternal;

		uint32 bIs3D : 1;
		uint32 bDebugMode : 1;
		//uint32 bIsVorbis : 1; // bIsHCA ...
		uint32 bAttenuationReady : 1;

		uint32 bPreviousSourceBusEnablement : 1;
		uint32 bPreviousBaseRackEnablement : 1;
		uint32 bSendingAudioToSourceBuses : 1;

		// This holds data copied from FAtomSoundSourceBusSendInfo when a new sound starts playing
		// so that distance-based level control can be calculated during rendering
		struct FDynamicSourceBusSendInfo
		{
			float SendLevel = 0.0f;
			uint32 BusID = 0;
#if ENABLE_ATOM_DEBUG
			FString BusName;
#endif // ENABLE_ATOM_DEBUG
			EAtomSourceBusSendLevelControlMethod BusSendLevelControlMethod = EAtomSourceBusSendLevelControlMethod::Manual;
			EAtomBusSendStage BusSendStage = EAtomBusSendStage::PreEffect;
			float MinSendLevel = 0.0f;
			float MaxSendLevel = 0.0f;
			float MinSendDistance = 0.0f;
			float MaxSendDistance = 0.0f;
			FRuntimeFloatCurve CustomSendLevelCurve;
			bool bIsInit = true;
		};

		mutable float PreviousPlaybackPercent;
		float PreviousAzimuth;

		// Array of copied FSoundSourceBusSendInfo data for all the bus sends this
		// source may need to live-update during its lifespan
		TArray<FDynamicSourceBusSendInfo> DynamicSourceBusSendInfos;

		// An array of bus sends from previous update. Allows us to clear out bus sends if they are no longer being sent.
		TArray<FAtomSoundToBusSend> PreviousBusSendSettings;

		// An array of cue selector labels from previous update. Allows us to clear out labels if they are no longer set.
		TArray<FAtomSelectorParam> PreviousCueSelectorParams;

		// cue block index to set from previous update. Allows us to call native set function only if index is modified.
		int32 PreviousCueNextBlockIndex;

		// synchronized cue's beatsync info
		TSharedPtr<FAtomBeatSyncInfo, ESPMode::ThreadSafe> CueBeatSyncInfo;

		// the AtomExPlayer's PlaybackListener
		TSharedPtr<FAtomExPlaybackListener, ESPMode::ThreadSafe> AtomListener;

		friend FAtomExPlaybackListener;
	};

	/**
	 * FAtomExternalExPlayback class
	 *
	 * Class which implements a mixer sound source object using an external AtomExPlayer.
	 */
	class FAtomExternalExPlayback : public FAtomExPlayback
	{
	public:

		FAtomExternalExPlayback(FAtomRuntime* InAtomRuntime, FCriAtomExPlayerPtr&& InExternalExPlayer);
		~FAtomExternalExPlayback();

		virtual FString Describe(bool bUseLongName) override { return bUseLongName ? TEXT("External AtomEx Player") : TEXT("External ExPlayer"); }
		virtual bool PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance) override;
		virtual bool IsVoiceSource() const override { return true; }

	private:

		FCriAtomExPlayerPtr TempExternalExPlayer;
	};

} // namespace
