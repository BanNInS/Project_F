
#include "Atom/Mixer/AtomMixerPlayer.h"

#include "Templates/Function.h"

#include "CriWareDefines.h"
#include "CriWareLLM.h"
#include "CriWareTrace.h" // move to Mixer/AtomMixerTrace.h
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"

// dev debug defines for output log
//#define ATOM_SOUND_DEV_DEBUG
//#define ATOM_SOUND_STATUS_DEBUG
//#define ATOM_SOUND_INIT_DEBUG

namespace Atom
{
	/*
	 * FAtomExPlayer implementation
	 *****************************************************************************/

	namespace FAtomExPlayer_NativeCallbacks
	{
		extern "C" void CRIAPI OnPlaybackEvent(void* Obj, CriAtomExPlaybackEvent PlaybackEvent, const CriAtomExPlaybackInfoDetail * Info)
		{
			// ADX should not call this function if player is destroyed
			if (FAtomExPlayer* Self = static_cast<FAtomExPlayer*>(Obj))
			{
				Self->HandleNativeOnPlaybackEvent(PlaybackEvent, Info);
			}
		}

		extern "C" void CRIAPI OnCueBlockIndexChanged(void* Obj, CriAtomExPlaybackId PlaybackID, CriAtomExBlockIndex BlockIndex)
		{
			if (FAtomExPlayer* Self = static_cast<FAtomExPlayer*>(Obj))
			{
				Self->HandleNativeOnCueBlockIndexChanged(PlaybackID, BlockIndex);
			}
		}

		extern "C" void CRIAPI OnFilter(void* Obj, CriAtomExPlaybackId PlaybackID, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[])
		{
			// ADX do not call this function if player is destroyed
			if (FAtomExPlayer* Self = static_cast<FAtomExPlayer*>(Obj))
			{
				Self->HandleNativePlayerOnFilter(PlaybackID, Format, NumChannels, NumSamples, Data);
			}
		}
	}

	// this occur from atom asr thread / and can happen from User thread too
	void FAtomExPlayer::HandleNativeOnPlaybackEvent(CriAtomExPlaybackEvent PlaybackEvent, const CriAtomExPlaybackInfoDetail* Info)
	{
		if (!Info)
		{
			return;
		}

		FAtomPlaybackId PlaybackID = Info->id;

#ifdef ATOM_SOUND_STATUS_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("OnPlaybackEvent Player<%p> ID[%d] Event:%s"), ExPlayer.Get(), PlaybackID,
			PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_ALLOCATE ? TEXT("Alloc") :
			PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_FROM_NORMAL_TO_VIRTUAL ? TEXT("Virtualize") :
			PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_FROM_VIRTUAL_TO_NORMAL ? TEXT("Realize") :
			TEXT("Remove"));
#endif

		QueuedEvents.Enqueue({ PlaybackEvent, PlaybackID });
	}

	// this occur from atom render thread
	void FAtomExPlayer::HandleNativeOnCueBlockIndexChanged(CriAtomExPlaybackId PlaybackID, CriAtomExBlockIndex BlockIndex)
	{
		FScopeLock Lock(&PlaybackListenersLock);

		if (TSharedRef<IPlaybackListener>* ListenerPtr = PlaybackListeners.Find(PlaybackID))
		{
			TSharedRef<IPlaybackListener> Listener = *ListenerPtr;
			Listener->OnCueBlockIndexChanged((int32)BlockIndex);
		}
	}

	// this occur from atom render thread
	void FAtomExPlayer::HandleNativePlayerOnFilter(CriAtomExPlaybackId PlaybackID, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumFrames, void* Data[])
	{
		// In some case (voice pool removed) Atom call this once from user thread...
		// Since it is an unexpected behaviour, we forbid this here.
		if (IsInAtomThread())
		{
			return;
		}

		FScopeLock Lock(&PlaybackListenersLock);

		if (TSharedRef<IPlaybackListener>* ListenerPtr = PlaybackListeners.Find(PlaybackID))
		{
			TSharedRef<IPlaybackListener> Listener = *ListenerPtr;
			Listener->OnFilter(FAtomRuntime::GetPcmBitDepthFromAtomPcmFormat(Format), NumChannels, NumFrames, Data);
		}
	}

	FAtomExPlayer::FAtomExPlayer()
		: AtomRuntime(nullptr)
		, ActiveSound(nullptr)
		, ExPlayer(nullptr)
		, bIsInitialized(false)
		, bUsePcmFilter(false)
		, bIs3D(false)
	{
	}

	FAtomExPlayer::FAtomExPlayer(const FAtomExPlayerArgs& InArgs, FCriAtomExPlayerPtr&& ExternalExPlayer)
		: AtomRuntime(InArgs.AtomRuntime)
		, ActiveSound(InArgs.ActiveSound)
		, bIsInitialized(false)
		, bUsePcmFilter(false)
		, bIs3D(false)
	{
		CriAtomExPlayerConfig Config;
		criAtomExPlayer_SetDefaultConfig(&Config);
		//Config.updates_time = atomconfig; // can get playback time time
		//Config.enable_audio_synced_timer = atomconfig; // can get sample synced time
		//Config.voice_allocation_method = CRIATOMEX_RETRY_VOICE_ALLOCATION; // virtualize if sound not possible

		if (ExternalExPlayer.IsValid())
		{
			ExPlayer = MoveTemp(ExternalExPlayer);
			check(ExPlayer.IsExternal());
			bIsExternal = true;
		}
		else
		{
			ExPlayer = MakeCriHandle(FCriWareApi::criAtomExPlayer_Create(&Config, nullptr, 0));
		}

		if (!ExPlayer)
		{
			// error
			return;
		}

		if (!bIsExternal) // impossible to control lifecycle of the player with external atom player
		{
			// register event callbacks
			FCriWareApi::criAtomExPlayer_SetPlaybackEventCallback(ExPlayer, FAtomExPlayer_NativeCallbacks::OnPlaybackEvent, this);
			FCriWareApi::criAtomExPlayer_SetBlockTransitionCallback(ExPlayer, FAtomExPlayer_NativeCallbacks::OnCueBlockIndexChanged, this);
		
			// register sound object
			if (ActiveSound)
			{
				if (UAtomSoundClass* SoundClass = ActiveSound->GetSoundClass())
				{
					if (auto PlayerGroupPtr = AtomRuntime->SoundClassPlayerGroups.Find(SoundClass))
					{
						PlayerGroup = *PlayerGroupPtr;
						PlayerGroup->AddPlayer(*this);
					}
				}
			}
		}

		// default to 2D sound
		FCriWareApi::criAtomExPlayer_SetPanType(ExPlayer, CRIATOMEX_PAN_TYPE_PAN3D);

		bIsInitialized = true;
	}

	FAtomExPlayer::~FAtomExPlayer()
	{
		if (ExPlayer.IsValid())
		{
			RemoveSpatialization();

			if (!bIsExternal)
			{
				// unregister events (To ensure Atom will not call it after player is destroyed!)
				FCriWareApi::criAtomExPlayer_SetPlaybackEventCallback(ExPlayer, nullptr, nullptr);
				FCriWareApi::criAtomExPlayer_SetBlockTransitionCallback(ExPlayer, nullptr, nullptr);

				// remove player from group
				if (PlayerGroup.IsValid())
				{
					PlayerGroup->RemovePlayer(*this);
					PlayerGroup.Reset();
				}
			}

			if (bUsePcmFilter)
			{
				FCriWareApi::criAtomExPlayer_SetFilterCallback(ExPlayer, nullptr, nullptr);
				bUsePcmFilter = false;
			}

			FCriWareApi::criAtomExPlayer_UpdateAll(ExPlayer);
		}

		// destroy 3d sources
		ExSource.Reset();
		ExSources.Reset();
		ExSourceList.Reset();

		ExPlayer.Reset();
	}

	FAtomPlaybackId FAtomExPlayer::Prepare()
	{
		return FCriWareApi::criAtomExPlayer_Prepare(ExPlayer);
	}

	void FAtomExPlayer::UpdateParameters(FAtomPlaybackId PlaybackID)
	{
		FCriWareApi::criAtomExPlayer_Update(ExPlayer, PlaybackID);
	}

	void FAtomExPlayer::Resume(FAtomPlaybackId PlaybackID)
	{
		FCriWareApi::criAtomExPlayback_Resume(PlaybackID, CriAtomExResumeMode::CRIATOMEX_RESUME_ALL_PLAYBACK);
	}

	void FAtomExPlayer::Pause(FAtomPlaybackId PlaybackID)
	{
		FCriWareApi::criAtomExPlayback_Pause(PlaybackID, CRI_TRUE);
	}

	void FAtomExPlayer::StopWithoutReleaseTime(FAtomPlaybackId PlaybackID)
	{
		FCriWareApi::criAtomExPlayback_StopWithoutReleaseTime(PlaybackID);
	}
	
	void FAtomExPlayer::Stop(FAtomPlaybackId PlaybackID)
	{
		FCriWareApi::criAtomExPlayback_Stop(PlaybackID);
	}

	bool FAtomExPlayer::Update()
	{
		FPlaybackEventInfo EventInfo;
		while (QueuedEvents.Dequeue(EventInfo))
		{
			ProcessEvent(EventInfo);
		}

		// check if player is still playing something even is all playbacks are removed.
		const bool bIsPlayerAlive = ExPlayer.IsValid() && FCriWareApi::criAtomExPlayer_GetNumPlaybacks(ExPlayer) > 0;
#ifdef ATOM_SOUND_STATUS_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("UpdateExPlayer: Player<%p> is %s."), ExPlayer.Get(), bIsPlayerAlive ? TEXT("playing") : TEXT("not playing"));
#endif
		return bIsPlayerAlive;
	}

	void FAtomExPlayer::ProcessEvent(FPlaybackEventInfo EventInfo)
	{
        check(IsInAtomThread());
        
		CriAtomExPlaybackEvent PlaybackEvent = EventInfo.Event;
		FAtomPlaybackId PlaybackID = EventInfo.PlaybackID;

		// DEV NOTES:
		//
		// Info.id is ExPlaybackId
		// Info.player is the caller ExPlayer of this FAtomExPlayer.
		//
		// Atom class mapping:
		// -> CriAtomExPlayer (static cue settings, overall playback control)	==> Encapsulated into FAtomExPlayer/FMixerPlayer	this is the main player unit that control sub-playbacks. It is shared between ActiveSound and FAtomExPlayback
		// --> CriAtomExPlaybackId (dsp effects, per playback id control)		==> Encapsulated into FAtomExPlayback/FMixerSource	this are real playback controller from Atom SDK and used by Atom runtime as playback sub-unit
		// ---> CriAtomPlayer (aka voice) (per voice playback control)			==> NOT USED (FAtomPlayer/FMixerSource is used for pure voice playback)
		//
		// From Info we can know with element is allocated or created, removed etc. and inform UE layer when it need to stop or continue ActiveSound. 
		// An active sound control multilple playaback instances that are parameters for each mixer sound source that derives to FAtomExPlayeback object.
		//
		// UE design mapping:
		// The goal is to control, in real time, any sound statuses and parameters then inform the game about any change over time that may hapend (virtualized, realized, stopped, paused, restarted etc.).
		// This may be triggerd by game actions or by the cue itself from sound designer. 
		// 
		// -> AtomComponent			==> the first UObject to play and control one or more sounds.
		// --> ActiveSound			==> instance of a sound/cue/mana sound stream/generated sound. any sound that can be played and controlled by the game. [AC controls multiple AS].
		// ---> PlaybackInstances	==> that are real time parameters for a sound. [AS controls multiple PI].
		// 
		// PlaybackInstance is assigned to one FAtomSoundSource the smallest control/param element in unreal -> an FAtomSoundSource derives to FMixerSource and then to FAtomExPlayback (multi-voices sound, cue...) or FAtomPlayer (single voice).
		// ActiveSound and FAtomSource are assigned to the corresponding FAtomExPlayer that controls them.
		// 
		// FAtomSoundSources are pooled and managed by AtomMixerSourceManager.
		// ActiveSounds are created from c++ or AtomComponent for Gameplay usage depending situation. 
		// Concurrency system works on ActiveSound layer.

		if (!ActiveSound || !ExPlayer.IsValid())
		{
			// sound was invalidated - nothing to do
			return;
		}

		auto ProcessPlaybackInstance = [this, PlaybackID, PlaybackEvent](bool bCanCreate)
		{
			{ // if exists, redirect to each playback listener
				FScopeLock Lock(&PlaybackListenersLock);
				TSharedRef<IPlaybackListener>* ListenerPtr = PlaybackListeners.Find(PlaybackID);
				if (ListenerPtr)
				{
					TSharedRef<IPlaybackListener> Listener = *ListenerPtr;
					Listener->OnPlaybackEvent((IPlaybackListener::EPlaybackEvent)PlaybackEvent);
					return;
				}
				if (PlaybackListeners.IsEmpty())
				{
					// no yet listeners - first
					return;
				}
			}

			if (bCanCreate)
			{
				// this PlaybackID is unkown so we creates a new playback instance 
				// that will be associated to a free FAtomSource/FAtomExPlayback by AtomRuntime.

				const UPTRINT Hash = static_cast<UPTRINT>(PointerHash(this, PlaybackID)) << 32; // Hash in higher part
				const UPTRINT ChildHash = Hash + static_cast<UPTRINT>(PlaybackID); // PlaybackID in lower part

				// Get the PlaybackInstance to use or create new one.
				FAtomPlaybackInstance* ChildPlaybackInstance = ActiveSound->FindPlaybackInstance(ChildHash);
				if (!ChildPlaybackInstance)
				{
					ChildPlaybackInstance = &ActiveSound->AddPlaybackInstance(ChildHash);
				}

				// setup info as soon as possible
				if (ChildPlaybackInstance)
				{
					GetPlaybackInfo(ChildPlaybackInstance, PlaybackID);
				}
			}
		};

		auto VirtualizeActiveSound = [this](FAtomActiveSound& ActiveSoundRef, bool bVirtualize, const TCHAR* ReasonLog)
		{
			if (bVirtualize)
			{
				const bool bDoRangeCheck = false;
				FAtomVirtualLoop VirtualLoop;
				if (FAtomVirtualLoop::Virtualize(ActiveSoundRef, bDoRangeCheck, VirtualLoop))
				{
#if !UE_BUILD_SHIPPING
					if (UAtomSoundBase* Sound = ActiveSoundRef.GetSound())
					{
						UE_LOG(LogCriWareAtomMixerDebug, Verbose, TEXT("Playing ActiveSound %s Virtualizing: %s"), *Sound->GetName(), ReasonLog);
					}
#endif
					ActiveSoundRef.GetAtomRuntime()->AddVirtualLoopFromMixer(VirtualLoop);
				}
			}
			else
			{
#if !UE_BUILD_SHIPPING
				if (UAtomSoundBase* Sound = ActiveSoundRef.GetSound())
				{
					UE_LOG(LogCriWareAtomMixerDebug, Verbose, TEXT("Playing ActiveSound %s Realizing: %s"), *Sound->GetName(), ReasonLog);
				}
#endif
				ActiveSoundRef.GetAtomRuntime()->RealizeVirtualLoopFromMixer(ActiveSoundRef);
			}
		};

		// only Atom cues can generate instances
		bool bCanCreate = true;

		if (PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_ALLOCATE)
		{
			ProcessPlaybackInstance(bCanCreate);
			VirtualizeActiveSound(*ActiveSound, true, TEXT("by allocation"));
		}
		else if (PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_FROM_NORMAL_TO_VIRTUAL)
		{
			ProcessPlaybackInstance(bCanCreate);
			VirtualizeActiveSound(*ActiveSound, true, TEXT("Sound's voice stollen due to Atom concurrency group maximum met or silent."));
		}
		else if (PlaybackEvent == CRIATOMEX_PLAYBACK_EVENT_FROM_VIRTUAL_TO_NORMAL)
		{
			ProcessPlaybackInstance(bCanCreate);
			VirtualizeActiveSound(*ActiveSound, false, TEXT("Sound's voice is available or sound became audible."));
		}
		else  // CRIATOMEX_PLAYBACK_EVENT_REMOVE
		{
			ProcessPlaybackInstance(false);
		}
	}

	void FAtomExPlayer::RegisterPlaybackListener(TSharedRef<IPlaybackListener> PlaybackListener, FAtomPlaybackId PlaybackID)
	{
		FScopeLock Lock(&PlaybackListenersLock);
		check(!PlaybackListeners.Contains(PlaybackID));
		PlaybackListeners.Emplace(PlaybackID, PlaybackListener);
	}

	void FAtomExPlayer::UnregisterPlaybackListener(FAtomPlaybackId PlaybackID)
	{
		FScopeLock Lock(&PlaybackListenersLock);
		check(PlaybackListeners.Contains(PlaybackID));
		PlaybackListeners.Remove(PlaybackID);
	}

	void FAtomExPlayer::EnablePcmFilter(bool bEnable)
	{
		if (bEnable)
		{
			if (!bUsePcmFilter)
			{
				bUsePcmFilter = true;
				FCriWareApi::criAtomExPlayer_SetFilterCallback(ExPlayer, FAtomExPlayer_NativeCallbacks::OnFilter, this);
			}
		}
		else
		{
			FCriWareApi::criAtomExPlayer_SetFilterCallback(ExPlayer, nullptr, nullptr);
		}
	}

	TSharedPtr<FAtomExPlayer, ESPMode::ThreadSafe> FAtomExPlayer::Create(const FAtomExPlayerArgs& InArgs)
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		// Fail if the Wave has been flagged to contain an error
		//if (InArgs.SoundData /* && InArgs.SoundData->HasError()*/)
		//{
		//	UE_LOG(LogCriWareAtomMixer, VeryVerbose, TEXT("FMixerSourceBuffer::Create failed as '%s' is flagged as containing errors"), *InArgs.SoundData->GetName());
		//	return {};
		//}

		if (InArgs.ActiveSound)
		{
			// find if existing explayer for this activesound
			if (!InArgs.ActiveSound->InstancePlayer.IsValid())
			{
				InArgs.ActiveSound->InstancePlayer = MakeShared<FAtomExPlayer>(InArgs);
			}

			return InArgs.ActiveSound->InstancePlayer;
		}

		return MakeShared<FAtomExPlayer>(InArgs);
	}

	TSharedPtr<FAtomExPlayer, ESPMode::ThreadSafe> FAtomExPlayer::CreateWithExternalPlayer(const FAtomExPlayerArgs& InArgs, FCriAtomExPlayerPtr&& ExternalPlayer)
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		// Fail if the sdk player is invalid
		if (!ExternalPlayer.IsValid())
		{
			UE_LOG(LogCriWareAtomMixer, VeryVerbose, TEXT("FAtomExPlayer::Create failed as '%s' uses an invalid external player."), *InArgs.SoundData->GetName());
			return {};
		}
		
		// Fail if the Wave has been flagged to contain an error
		//if (InArgs.SoundData /* && InArgs.SoundData->HasError()*/)
		//{
		//	UE_LOG(LogCriWareAtomMixer, VeryVerbose, TEXT("FAtomExPlayer::Create failed as '%s' is flagged as containing errors."), *InArgs.SoundData->GetName());
		//	return {};
		//}

		if (InArgs.ActiveSound)
		{
			// find if existing explayer for this activesound
			if (!InArgs.ActiveSound->InstancePlayer.IsValid())
			{
				InArgs.ActiveSound->InstancePlayer = MakeShared<FAtomExPlayer>(InArgs, MoveTemp(ExternalPlayer));
			}

			return InArgs.ActiveSound->InstancePlayer;
		}

		return MakeShared<FAtomExPlayer>(InArgs, MoveTemp(ExternalPlayer));
	}

	void FAtomExPlayer::GetPlaybackInfo(FAtomPlaybackInstance* PlaybackInstance, FAtomPlaybackId PlaybackID)
	{
		if (PlaybackInstance && PlaybackInstance->SoundInfo.SoundID == INDEX_NONE)
		{
			// todo wave info update
			//FCriWareApi::criAtomExPlayback_GetFormatInfo(PlaybackID, &Info);

			CriAtomExSourceInfo SourceInfo;
			FCriWareApi::criAtomExPlayback_GetSource(PlaybackID, &SourceInfo);

			auto& SoundInfo = PlaybackInstance->SoundInfo;

			switch (SourceInfo.type)
			{
			case CRIATOMEX_SOURCE_TYPE_CUE_ID:
			{
				CriAtomExCueInfo Info;
				FCriWareApi::criAtomExAcb_GetCueInfoById(SourceInfo.info.cue_id.acb, SourceInfo.info.cue_id.id, &Info);
				SoundInfo.Type = EAtomSoundPlaybackType::Cue;
				SoundInfo.SoundID = (int32)Info.id;
				SoundInfo.SoundName = UTF8_TO_TCHAR(Info.name);
				SoundInfo.bIs3D = Info.pan_type == CRIATOMEX_PAN_TYPE_3D_POS;
				break;
			}
			case CRIATOMEX_SOURCE_TYPE_CUE_NAME:
			{
				CriAtomExCueInfo Info;
				FCriWareApi::criAtomExAcb_GetCueInfoByName(SourceInfo.info.cue_name.acb, SourceInfo.info.cue_name.name, &Info);
				SoundInfo.Type = EAtomSoundPlaybackType::Cue;
				SoundInfo.SoundID = (int32)Info.id;
				SoundInfo.SoundName = UTF8_TO_TCHAR(Info.name);
				SoundInfo.bIs3D = Info.pan_type == CRIATOMEX_PAN_TYPE_3D_POS;
				break;
			}
			case CRIATOMEX_SOURCE_TYPE_CUE_INDEX:
			{
				CriAtomExCueInfo Info;
				FCriWareApi::criAtomExAcb_GetCueInfoByIndex(SourceInfo.info.cue_index.acb, SourceInfo.info.cue_index.index, &Info);
				SoundInfo.Type = EAtomSoundPlaybackType::Cue;
				SoundInfo.SoundID = (int32)Info.id;
				SoundInfo.SoundName = UTF8_TO_TCHAR(Info.name);
				SoundInfo.bIs3D = Info.pan_type == CRIATOMEX_PAN_TYPE_3D_POS;
				break;
			}
			case CRIATOMEX_SOURCE_TYPE_WAVE_ID:
			{
				// todo use the awb point to retrieve the name of the bank in editor.
				SoundInfo.Type = EAtomSoundPlaybackType::Wave;
				SoundInfo.SoundID = (int32)SourceInfo.info.wave_id.id;
				SoundInfo.SoundName = FName(TEXT("Wave") + FString::FromInt(SourceInfo.info.wave_id.id));
				break;
			}
			case CRIATOMEX_SOURCE_TYPE_RAW_PCM_FLOAT_ID:
				SoundInfo.Type = EAtomSoundPlaybackType::RawPCM;
				SoundInfo.SoundID = 0;
				SoundInfo.SoundName = TEXT("External PCM");
				break;
			case CRIATOMEX_SOURCE_TYPE_INPUT_PORT:
				// todo retrieve the port name
				SoundInfo.Type = EAtomSoundPlaybackType::InputPort;
				SoundInfo.SoundID = 0;
				SoundInfo.SoundName = TEXT("InputPort PCM");
				break;
			default:
				SoundInfo.Type = EAtomSoundPlaybackType::Unknown;
				SoundInfo.SoundID = 0;
				SoundInfo.SoundName = TEXT("Unknown");
				break;
			}

			if (SoundInfo.Type == EAtomSoundPlaybackType::Cue)
			{
				CriAtomExPlaybackTrackInfo CurrentTrackInfo;
				FCriWareApi::criAtomExPlayback_GetPlaybackTrackInfo(PlaybackID, &CurrentTrackInfo);
				SoundInfo.StartTrackIndex = (int32)CurrentTrackInfo.track_no;
				SoundInfo.StartBlockIndex = (int32)FCriWareApi::criAtomExPlayback_GetCurrentBlockIndex(PlaybackID);
			}
			else
			{
				SoundInfo.StartTrackIndex = INDEX_NONE;
				SoundInfo.StartBlockIndex = INDEX_NONE;
			}
		}
	}

	void FAtomExPlayer::AddSpatialization(const FAtomPlaybackInstance& PlaybackInstance)
	{
		check(ActiveSound);

		// Set the 3d sources. Set3dSource and Set3dSourcelist overrides each other.
		if (PlaybackInstance.MultiPositionType == EAtomMultiPositionType::SingleSource)
		{
			if (!ExSource.IsValid())
			{
				// Create a source
				CriAtomEx3dSourceConfig Config;
				criAtomEx3dSource_SetDefaultConfig(&Config);
				ExSource = MakeCriHandle(FCriWareApi::criAtomEx3dSource_Create(&Config, nullptr, 0));
			}

			// attach source
			//UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("ExPlayer_Set3dSourceHn <%p> <%p>"), ExPlayer.Get(), ExSource.Get());
			FCriWareApi::criAtomExPlayer_Set3dSourceHn(ExPlayer, ExSource);
		}
		else // Multi positions
		{
			// Create a source list
			if (!ExSourceList.IsValid())
			{
				CriAtomEx3dSourceListConfig Config;
				criAtomEx3dSourceList_SetDefaultConfig(&Config);
				ExSourceList = MakeCriHandle(FCriWareApi::criAtomEx3dSourceList_Create(&Config, nullptr, 0));
			}

			int NumPositions = PlaybackInstance.MultiPositions.Num();

			// add positions
			for (int Index = ExSources.Num(); Index < NumPositions; ++Index)
			{
				// Create a source
				CriAtomEx3dSourceConfig Config;
				criAtomEx3dSource_SetDefaultConfig(&Config);
				FCriAtomExSourcePtr NewExSource = MakeCriHandle(FCriWareApi::criAtomEx3dSource_Create(&Config, nullptr, 0));
				if (NewExSource.IsValid())
				{
					FCriWareApi::criAtomEx3dSourceList_Add(ExSourceList, NewExSource);
				}
				ExSources.Add(Forward<FCriAtomExSourcePtr>(NewExSource));
			}

			// remove unused positions
			for (int LastIndex = ExSources.Num() - 1; LastIndex >= NumPositions; --LastIndex)
			{
				if (ExSources[LastIndex].IsValid())
				{
					FCriWareApi::criAtomEx3dSourceList_Remove(ExSourceList, ExSources[LastIndex]);
				}

				ExSources.RemoveAt(LastIndex);
			}

			// attach source list
			FCriWareApi::criAtomExPlayer_Set3dSourceListHn(ExPlayer, ExSourceList);
		}

		// Note: by default atom use the closet listener present
		// attach closest listener
		//int32 Index = AtomRuntime->FindClosestListenerIndex(PlaybackInstance->Transform);
		//auto& ClosestListener = AtomRuntime->GetListeners()[Index];
		//FCriWareApi::criAtomExPlayer_Set3dListenerHn(MixerPlayer->ExPlayer, ClosestListener.ExListener);
	}

	void FAtomExPlayer::RemoveSpatialization()
	{
		// Note: by default atom use the closet listener present
		//FCriWareApi::criAtomExPlayer_Set3dListenerHn(MixerPlayer->ExPlayer, nullptr);

		if (ExSource.IsValid())
		{
			//UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("criAtomEx3dSource_ResetParameters <%p> <%p>"), ExPlayer.Get(), ExSource.Get());
			FCriWareApi::criAtomEx3dSource_ResetParameters(ExSource);

			if (ExPlayer.IsValid())
			{
				//UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("ExPlayer_Set3dSourceHn(nullptr) <%p> <%p>"), ExPlayer.Get(), ExSource.Get());
				FCriWareApi::criAtomExPlayer_Set3dSourceHn(ExPlayer, nullptr);
			}
		}

		if (ExSourceList.IsValid())
		{
			for (auto& Source : ExSources)
			{
				FCriWareApi::criAtomEx3dSource_ResetParameters(Source);
			}

			if (ExPlayer.IsValid())
			{
				FCriWareApi::criAtomExPlayer_Set3dSourceListHn(ExPlayer, nullptr);
			}
		}
	}

	void FAtomExPlayer::Update3DSource(FCriAtomExSourcePtr& InExSource, const FTransform& InTransform, const FAtomAttenuationSettings* InAttenuationSettings, bool bSpatialize, bool bForceAttenuate)
	{
		const float DistanceFactor = 0.01f; // UE5 (cm) to Atom (meters)
		auto Pos = ToCriAtomExVector(InTransform.GetTranslation() * DistanceFactor);
		auto Front = ToCriAtomExVector(InTransform.GetUnitAxis(EAxis::X));
		auto Up = ToCriAtomExVector(InTransform.GetUnitAxis(EAxis::Z));
		auto Velocity = ToCriAtomExVector(InTransform.GetScale3D() * DistanceFactor);

		//UE_LOG(LogTemp, Warning, TEXT("Source SetPos(%f, %f, %f)"), Pos.x, Pos.y, Pos.z);
		//UE_LOG(LogTemp, Warning, TEXT("Source SetOri(%f, %f, %f | %f, %f, %f)"), Front.x, Front.y, Front.z, Up.x, Up.y, Up.z);
		//UE_LOG(LogTemp, Warning, TEXT("Source SetVel(%f, %f, %f)"), Velocity.x, Velocity.y, Velocity.z);

		// position
		FCriWareApi::criAtomEx3dSource_SetPosition(InExSource, &Pos);
		FCriWareApi::criAtomEx3dSource_SetOrientation(InExSource, &Front, &Up);
		FCriWareApi::criAtomEx3dSource_SetVelocity(InExSource, &Velocity);

		// ADX internal doppler fx - (it uses velocity to control pitch)
		//FCriWareApi::criAtomEx3dSource_SetDopplerFactor(InExSource, AttenuationSettings.DopplerIntensity);

		if (InAttenuationSettings)
		{
			auto& AttenuationSettings = *InAttenuationSettings;

			// on/off
			if (bForceAttenuate)
			{
				FCriWareApi::criAtomEx3dSource_SetAttenuationDistanceSetting(InExSource, AttenuationSettings.bAttenuate ? CRI_TRUE : CRI_FALSE);
			}

			if (AttenuationSettings.bAttenuate)
			{
				if (AttenuationSettings.AttenuationShape == EAtomAttenuationShape::Sphere)
				{
					// disable cone
					FCriWareApi::criAtomEx3dSource_SetConeParameter(InExSource, 360.0f, 360.0f, 0.0f);
				}
				else if (AttenuationSettings.AttenuationShape == EAtomAttenuationShape::Cone)
				{
					float InnerAngle = 0.0f;
					float OuterAngle = 0.0f;
					const float OutsideLevel = AttenuationSettings.GetConeAnglesAndOutsideLevel(InnerAngle, OuterAngle);

					FCriWareApi::criAtomEx3dSource_SetConeParameter(InExSource, InnerAngle * 2.0f, OuterAngle * 2.0f, OutsideLevel);
				}

				const float MinDistance = AttenuationSettings.GetMinDimension() * DistanceFactor;
				const float MaxDistance = AttenuationSettings.GetMaxDimension() * DistanceFactor;

				FCriWareApi::criAtomEx3dSource_SetMinMaxAttenuationDistance(InExSource, MinDistance, MaxDistance);
			}

			if (bSpatialize)
			{
				// NonSpatializedRadius (interior pan field in ADX sdk)
				const float SourceRadius = AttenuationSettings.NonSpatializedRadiusEnd * DistanceFactor;
				const float InteriorDistance = FMath::Max(0.0f, AttenuationSettings.NonSpatializedRadiusStart - AttenuationSettings.NonSpatializedRadiusEnd) * DistanceFactor;

				FCriWareApi::criAtomEx3dSource_SetInteriorPanField(InExSource, SourceRadius, InteriorDistance);
			}
			// Note: by default atom use the closest listener present
			// update closest listener
			//int32 Index = AtomRuntime->FindClosestListenerIndex(InTransform);
			//auto& ClosestListener = AtomRuntime->GetListeners()[Index];
			//FCriWareApi::criAtomExPlayer_Set3dListenerHn(ExPlayer, ClosestListener.ExListener);
		}

		// update
		FCriWareApi::criAtomEx3dSource_Update(InExSource);
	}

	void FAtomExPlayer::ResetSpatialization(const FAtomPlaybackInstance& PlaybackInstance)
	{
		if (bIs3D)
		{
			if (ExSource.IsValid())
			{
				Update3DSource(ExSource, PlaybackInstance.Transform);
			}
			else if (ExSourceList.IsValid() && ActiveSound)
			{
				for (int Index = 0; Index < PlaybackInstance.MultiPositions.Num(); Index++)
				{
					if (ExSources.IsValidIndex(Index) && ExSources[Index].IsValid())
					{
						Update3DSource(ExSources[Index], PlaybackInstance.MultiPositions[Index]);
					}
				}
			}
		}
	}

	bool FAtomExPlayer::UpdateSpatialization(const FAtomPlaybackInstance& PlaybackInstance)
	{
		check(ActiveSound);

		// todo: set attenutation plain data for atom in playback instance ? 
		// taking them from ActiveSound means you can't override attenuation settings from plugins.
		FAtomAttenuationSettings* AttenuationSettings = nullptr;
		if (ActiveSound->bHasAttenuationSettings)
		{
			AttenuationSettings = &ActiveSound->AttenuationSettings;
		}

		const bool bAllowSpatialization = ActiveSound->bAllowSpatialization;
		const bool bIs3DCue = PlaybackInstance.SoundInfo.bIs3D;
		const bool bAttenuate = AttenuationSettings ? AttenuationSettings->bAttenuate : false;
		const bool bSpatialize = PlaybackInstance.GetUseSpatialization();

		if (bAllowSpatialization && (bIs3DCue || bAttenuate || bSpatialize))
		{
			if (!bIs3D)
			{
				AddSpatialization(PlaybackInstance);

				// enable 3d positionning
				FCriWareApi::criAtomExPlayer_SetPanType(ExPlayer, CRIATOMEX_PAN_TYPE_3D_POS);

				bIs3D = true;
			}

			const bool bCanOverrideAttenuate = !bIs3DCue || bAttenuate;

			if (PlaybackInstance.MultiPositionType == EAtomMultiPositionType::SingleSource)
			{
				if (ExSource.IsValid())
				{
					if (ActiveSound->MultiPositions.IsEmpty())
					{
						// normal
						Update3DSource(ExSource, PlaybackInstance.Transform, AttenuationSettings, bSpatialize, bCanOverrideAttenuate);
					}
					else
					{
						// use the first given position if exists.
						Update3DSource(ExSource, PlaybackInstance.MultiPositions[0], AttenuationSettings, bSpatialize, bCanOverrideAttenuate);
					}
				}
			}
			else
			{
				// Update list if changed
				if (ExSources.Num() != PlaybackInstance.MultiPositions.Num())
				{
					AddSpatialization(PlaybackInstance);
				}

				// update sources values
				for (int Index = 0; Index < ExSources.Num(); ++Index)
				{
					if (ExSources[Index].IsValid() && PlaybackInstance.MultiPositions.IsValidIndex(Index))
					{
						Update3DSource(ExSources[Index], PlaybackInstance.MultiPositions[Index], AttenuationSettings, bSpatialize, bCanOverrideAttenuate);
					}
				}
			}
		}
		else
		{
			if (bIs3D)
			{
				bIs3D = false;

				//if (ExSource.IsValid() || ExSourceList.IsValid())
				//{
				//	RemoveSpatialization();
				//}

				// disable 3d positionning
				FCriWareApi::criAtomExPlayer_SetPanType(ExPlayer, CRIATOMEX_PAN_TYPE_PAN3D);
			}
		}

		return bIs3D;
	}

	FAtomExPlayerGroup::FAtomExPlayerGroup(bool bInEnableVoiceLimitScope, bool bInEnableCategoryCueLimitScope)
		: bEnableVoiceLimitScope(bInEnableVoiceLimitScope)
		, bEnableCategoryCueLimitScope(bInEnableCategoryCueLimitScope)
	{
		CriAtomExSoundObjectConfig Config;
		Config.enable_voice_limit_scope = bEnableVoiceLimitScope ? CRI_TRUE : CRI_FALSE;
		Config.enable_category_cue_limit_scope = bEnableCategoryCueLimitScope ? CRI_TRUE : CRI_FALSE;

		ExSoundObject = MakeCriHandle(FCriWareApi::criAtomExSoundObject_Create(&Config, nullptr, 0));
	}

	void FAtomExPlayerGroup::AddPlayer(FAtomExPlayer& Player)
	{
		if (ExSoundObject.IsValid() && !Player.IsExternal() && Player.IsInitialized())
		{
			if (CriAtomExPlayerHn ExPlayerHandle = (CriAtomExPlayerHn)Player.GetNativeHandle())
			{
				FCriWareApi::criAtomExSoundObject_AddPlayer(ExSoundObject, ExPlayerHandle);
			}
		}
	}

	void FAtomExPlayerGroup::RemovePlayer(FAtomExPlayer& Player)
	{
		if (ExSoundObject.IsValid() && !Player.IsExternal() && Player.IsInitialized())
		{
			if (CriAtomExPlayerHn ExPlayerHandle = (CriAtomExPlayerHn)Player.GetNativeHandle())
			{
				FCriWareApi::criAtomExSoundObject_DeletePlayer(ExSoundObject, ExPlayerHandle);
			}
		}
	}

} // namespace
