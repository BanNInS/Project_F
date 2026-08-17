
#include "Atom/Mixer/AtomMixerSource.h"

#include "Templates/Function.h"
#include "ProfilingDebugging/CsvProfiler.h"

#include "CriWareDefines.h"
#include "CriWareLLM.h"
#include "CriWareTrace.h" // move to Mixer/AtomMixerTrace.h
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerTrace.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/Mixer/AtomMixerSourceVoice.h"
#include "Atom/Mixer/AtomMixerPlayer.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomWaveBankResource.h"
#include "Atom/AtomCueSheetResource.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundWave.h"
#include "Atom/AtomSoundWaveProcedural.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundGenerator.h"
#include "Atom/AtomSoundSourceBus.h"
#include "Atom/Components/AtomExternalComponent.h"
#include "Atom/Components/AtomSynthComponent.h"

CSV_DECLARE_CATEGORY_MODULE_EXTERN(CRIWARECORE_API, Atom);

static int32 AtomUseListenerOverrideForSpreadCVar = 0;
FAutoConsoleVariableRef CVarAtomUseListenerOverrideForSpread(
	TEXT("atom.UseListenerOverrideForSpread"),
	AtomUseListenerOverrideForSpreadCVar,
	TEXT("Zero attenuation override distance stereo panning\n")
	TEXT("0: Use actual distance, 1: use listener override"),
	ECVF_Default);

// dev debug defines for output log
//#define ATOM_SOUND_DEV_DEBUG
//#define ATOM_SOUND_STATUS_DEBUG
//#define ATOM_SOUND_INIT_DEBUG

// normaly in manager
#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceVolume)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(uint32, ActiveSoundPlayOrder)
	UE_TRACE_EVENT_FIELD(float, Volume)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceDistanceAttenuation)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(float, DistanceAttenuation)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourcePitch)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(uint32, ActiveSoundPlayOrder)
	UE_TRACE_EVENT_FIELD(float, Pitch)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceFilters)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(float, LPFFrequency)
	UE_TRACE_EVENT_FIELD(float, HPFFrequency)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceEnvelope)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(uint32, ActiveSoundPlayOrder)
	UE_TRACE_EVENT_FIELD(float, Envelope)
UE_TRACE_EVENT_END()
#endif // ATOM_PROFILERTRACE_ENABLED*/

// trace for MixerSource
#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceStart)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(int32, SourceID)
	UE_TRACE_EVENT_FIELD(uint64, ComponentID)
	UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, MixerSourceStop)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
UE_TRACE_EVENT_END()
#endif // ATOM_PROFILERTRACE_ENABLED

namespace Atom
{
	// Modulation Utilities
	namespace AtomModulationUtils
	{
		FAtomSoundModulationSettings InitRoutedVolumeModulation(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationSettings(EAtomModulationDestination::Volume);
		}

		float GetRoutedVolume(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationValue(EAtomModulationDestination::Volume);
		}

		FAtomSoundModulationSettings InitRoutedPitchModulation(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationSettings(EAtomModulationDestination::Pitch);
		}

		float GetRoutedPitch(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationValue(EAtomModulationDestination::Pitch);
		}

		FAtomSoundModulationSettings InitRoutedHighpassModulation(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationSettings(EAtomModulationDestination::Highpass);
		}

		float GetRoutedHighpass(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationValue(EAtomModulationDestination::Highpass);
		}

		FAtomSoundModulationSettings InitRoutedLowpassModulation(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationSettings(EAtomModulationDestination::Lowpass);
		}

		float GetRoutedLowpass(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveModulationValue(EAtomModulationDestination::Lowpass);
		}

		FAtomAisacControlSettings InitRoutedAisacModulation(const FAtomAisacControl& AisacControl, const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveAisacModulationSettings(AisacControl);
		}

		float GetRoutedAisac(const FAtomAisacControl& AisacControl, const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound& InActiveSound)
		{
			return InPlaybackInstance.GetEffectiveAisacModulationValue(AisacControl);
		}

		FAtomSoundModulationDefaultSettings InitRoutedModulation(const FAtomPlaybackInstance& InPlaybackInstance, const UAtomSoundBase& InSound, const FAtomActiveSound* InActiveSound)
		{
			FAtomSoundModulationDefaultSettings Settings;
			if (InActiveSound)
			{
				Settings.VolumeModulation = InitRoutedVolumeModulation(InPlaybackInstance, InSound, *InActiveSound);
				Settings.PitchModulation = InitRoutedPitchModulation(InPlaybackInstance, InSound, *InActiveSound);
				Settings.HighpassModulation = InitRoutedHighpassModulation(InPlaybackInstance, InSound, *InActiveSound);
				Settings.LowpassModulation = InitRoutedLowpassModulation(InPlaybackInstance, InSound, *InActiveSound);

				Settings.AisacModulations.AisacControlModulations.Reset();
				TSet<FAtomAisacControl> ControlsToModulate;
				// TODO: get all control possible for this player/cue (craft + global attached)
				{
					for (auto& ControlMod : InSound.ModulationSettings.AisacModulations.AisacControlModulations)
					{
						ControlsToModulate.Add(ControlMod.Control);
					}

					for (auto& ControlMod : InActiveSound->ModulationRouting.AisacModulations.AisacControlModulations)
					{
						ControlsToModulate.Add(ControlMod.Control);
					}

					const UAtomSoundClass* EffectiveSoundClass = InActiveSound->GetSoundClass();
					if (InPlaybackInstance.SoundClass)
					{
						EffectiveSoundClass = InPlaybackInstance.SoundClass;
					}

					if (EffectiveSoundClass)
					{
						for (auto& ControlMod : EffectiveSoundClass->Properties.ModulationSettings.AisacModulations.AisacControlModulations)
						{
							ControlsToModulate.Add(ControlMod.Control);
						}
					}
				}

				for (auto& AisacControl : ControlsToModulate)
				{
					Settings.AisacModulations.AisacControlModulations.Add(InitRoutedAisacModulation(AisacControl, InPlaybackInstance, InSound, *InActiveSound));
				}
			}

			return Settings;
		}
	} // namespace

	FMixerSource::FMixerSource(FAtomRuntime* InAtomRuntime)
		: FAtomSource(InAtomRuntime)
		, MixerSourceVoice(nullptr)
		, bDebugMode(false)
	{
	}

	FMixerSource::~FMixerSource()
	{
	}

	bool FMixerSource::InitSourceVoice(FAtomPlaybackInstance* InPlaybackInstance)
	{
		ATOM_MIXER_CHECK(InPlaybackInstance);

		UAtomSoundBase* SoundData = InPlaybackInstance->SoundData;
		check(SoundData);

		if (SoundData->WaveInfo.NumChannels == 0)
		{
			if (SoundData->IsA<UAtomSoundWave>())
			{
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Soundwave %s has invalid compressed data."), *(SoundData->GetName()));
				FreeResources(EFreeReason::Error);
				return false;
			}
		}

		if (SoundData->WaveInfo.NumChannels > 0)
		{
			CSV_SCOPED_TIMING_STAT(Atom, InitSources);
			SCOPE_CYCLE_COUNTER(STAT_AtomSourceInitTime);

			// new
			MixerSourceVoice = AtomRuntime->GetMixerSourceVoice();

			if (!MixerSourceVoice)
			{
				FreeResources(EFreeReason::Error);
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Failed to get a mixer source voice for sound %s."), *InPlaybackInstance->GetName());
				return false;
			}

			auto& WaveData = SoundData->WaveInfo;
			const bool bIsProcedural = SoundData->bProcedural;
			const bool bIsSourceBus = SoundData->bIsSourceBus;

			// Initialize the source voice with the necessary format information
			FMixerSourceVoiceInitParams InitParams;
			InitParams.SourceListener = this;
			InitParams.NumInputChannels = WaveData.NumChannels;
			InitParams.NumInputFrames = WaveData.NumFrames;
			InitParams.InputFormat = WaveData.Format;
			InitParams.InputSampleRate = WaveData.SampleRate; // will be multiplied with max pitch later

			if (!bIsProcedural && !bIsSourceBus)
			{
				// determine if actually streamed data or on memory for awb
				if (auto WaveBank = Cast<UAtomWaveBank>(SoundData->GetSoundResource()))
				{
					check(WaveBank->AtomResource);
					WaveData.bIsStreamed = WaveBank->AtomResource->GetWaveBankResource()->IsUsingFileSystemBinder();
				}
	
				InitParams.StreamingType = WaveData.bIsStreamed ? EMixerSourceVoiceStreamingType::StreamOnly : EMixerSourceVoiceStreamingType::MemoryOnly;

				if (auto SoundCue = Cast<UAtomSoundCue>(SoundData))
				{
					if (AtomRuntime->IsUsingFixedVoicePools() && !SoundCue->bUseDynamicVoices)
					{
						InitParams.bUseFixedVoicePools = true;
					}
					else
					{
						InitParams.bUseFixedVoicePools = false;
						InitParams.InputVoices = SoundCue->VoiceLimit;
						if (SoundCue->VoiceSettingsOverrides.bOverrideNumChannels)
						{
							InitParams.NumInputChannels = SoundCue->VoiceSettingsOverrides.NumChannels;
						}
						if (SoundCue->VoiceSettingsOverrides.bOverrideSamplingRate)
						{
							const float MaxPitchScale = FMath::Max(AtomRuntime->GetGlobalPitchRange().GetUpperBoundValue(), UE_KINDA_SMALL_NUMBER);
							InitParams.InputSampleRate = SoundCue->VoiceSettingsOverrides.SamplingRate / MaxPitchScale;
						}
					}

					// determine if actually streamed data, on memory or both for acb and used awb
					bool bDependencyIsStreamed = false;
					if (auto CueSheet = Cast<UAtomCueSheet>(SoundData->GetSoundResource()))
					{
						check(CueSheet->AtomResource);
						CueSheet->AtomResource->IterateOverSoundResourceDependencies([&StreamingType = InitParams.StreamingType](IAtomSoundResource* Dependency)
						{
							if (auto WaveBank = Cast<UAtomWaveBank>(Dependency))
							{
								if (WaveBank->AtomResource.IsValid())
								{
									const bool bIsStreamed = WaveBank->AtomResource->GetWaveBankResource()->IsUsingFileSystemBinder();

									if ((bIsStreamed && StreamingType == EMixerSourceVoiceStreamingType::MemoryOnly) ||
										(!bIsStreamed && StreamingType == EMixerSourceVoiceStreamingType::StreamOnly))
									{
										StreamingType = EMixerSourceVoiceStreamingType::Mixed;
									}
								}
								else
								{
									StreamingType = EMixerSourceVoiceStreamingType::Mixed;
								}
							}
						});
					}
				}
			}
			else if (bIsProcedural)
			{
				if (auto ExternalSound = Cast<UAtomExternalSound>(SoundData))
				{
					InitParams.ExternalVoicePool = ExternalSound->GetExternalVoicePool();
				}
			}

			InitParams.bUseHRTFSpatialization = UseObjectBasedSpatialization();
			InitParams.bIsSoundfield = InPlaybackInstance->bIsAmbisonics && (WaveData.NumChannels >= 4);
			InitParams.SourceVoice = MixerSourceVoice;

			FAtomActiveSound* ActiveSound = InPlaybackInstance->ActiveSound;
			InitParams.ModulationSettings = AtomModulationUtils::InitRoutedModulation(*InPlaybackInstance, *SoundData, ActiveSound);

			// Copy quantization request data
			if (PlaybackInstance->QuantizedRequestData)
			{
				InitParams.QuantizedRequestData = *PlaybackInstance->QuantizedRequestData;
			}

			if (PlaybackInstance->bIsAmbisonics && (WaveData.NumChannels < 4))
			{
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Sound Wave/Cue %s was flagged as being ambisonics but had a channel count of %d. Currently the audio engine only supports FOA sources that have four channels."), *InPlaybackInstance->GetName(), WaveData.NumChannels);
			}

			if (ActiveSound)
			{
				InitParams.AtomComponentUserID = ActiveSound->GetAtomComponentUserID();
#if ATOM_MIXER_ENABLE_DEBUG_MODE
				if (InitParams.AtomComponentUserID.IsNone())
				{
					InitParams.AtomComponentUserID = ActiveSound->GetSound()->GetFName();

				}
#endif // ATOM_MIXER_ENABLE_DEBUG_MODE
				InitParams.AtomComponentID = ActiveSound->GetAtomComponentID();
			}

			InitParams.EnvelopeFollowerAttackTime = InPlaybackInstance->EnvelopeFollowerAttackTime;
			InitParams.EnvelopeFollowerReleaseTime = InPlaybackInstance->EnvelopeFollowerReleaseTime;

			//InitParams.SourceBufferListener = WaveInstance->SourceBufferListener;
			//InitParams.bShouldSourceBufferListenerZeroBuffer = WaveInstance->bShouldSourceBufferListenerZeroBuffer;

			//InitParams.bIsVorbis = bIsVorbis;

			// Setup the bus Id if this source is a bus
			if (bIsSourceBus)
			{
				// We need to check if the source bus has an audio bus specified
				UAtomSoundSourceBus* SoundSourceBus = CastChecked<UAtomSoundSourceBus>(SoundData);

				// If it does, we will use that audio bus as the source of the audio data for the source bus
				if (SoundSourceBus->AudioBus)
				{
					InitParams.AudioBusID = SoundSourceBus->AudioBus->GetUniqueID();
					InitParams.AudioBusChannels = (int32)SoundSourceBus->AudioBus->GetNumChannels();
					InitParams.NumInputChannels = InitParams.AudioBusChannels;
#if ENABLE_ATOM_DEBUG
					InitParams.AudioBusName = SoundSourceBus->AudioBus->GetPathName();
#endif // ENABLE_ATOM_DEBUG
				}
				else
				{
					InitParams.AudioBusID = SoundData->GetUniqueID();
					InitParams.AudioBusChannels = WaveData.NumChannels;
#if ENABLE_ATOM_DEBUG
					InitParams.AudioBusName = SoundData->GetPathName();
#endif // ENABLE_ATOM_DEBUG
				}

				if (!SoundData->IsLooping())
				{
					InitParams.SourceBusDuration = SoundData->GetDuration();
				}
			}

			InitParams.bEnableBusSends = PlaybackInstance->bEnableSourceBusSends;
			InitParams.bEnableBaseSubmix = PlaybackInstance->bEnableSoundRack;
			InitParams.bEnableSubmixSends = PlaybackInstance->bEnableSoundBusSends;
			InitParams.PlayOrder = PlaybackInstance->GetPlayOrder();
			InitParams.ActiveSoundPlayOrder = PlaybackInstance->ActiveSound != nullptr ? PlaybackInstance->ActiveSound->GetPlayOrder() : INDEX_NONE;

			SetupSourceBusData(InitParams.AudioBusSends, InitParams.bEnableBusSends);

			// Check to see if this sound has been flagged to be in debug mode
#if ATOM_MIXER_ENABLE_DEBUG_MODE
			InitParams.DebugName = InPlaybackInstance->GetName();

			bool bIsDebug = false;
			FString WaveInstanceName = InPlaybackInstance->GetName();
			FString TestName = GCriWare->GetAtomRuntimeManager()->GetDebugger().GetAtomMixerDebugSoundName();
			if (!TestName.IsEmpty() && WaveInstanceName.Contains(TestName))
			{
				bDebugMode = true;
				InitParams.bIsDebugMode = bDebugMode;
			}
#endif
			
			// bIs3d means -> is sound 3d source sound
			//InitParams.bIs3D = !UseObjectBasedSpatialization() && PlaybackInstance->GetUseSpatialization();

			// Hand off the mixer source player
			InitParams.MixerSourcePlayer = MixerSourcePlayer;
			MixerSourcePlayer = {};

			if (MixerSourceVoice->Init(InitParams))
			{
				// init SourceDataOverridePluginInterface plugins ?

				return true;
			}
			else
			{
				//InitializationState = EMixerSourceInitializationState::NotInitialized;
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Failed to initialize mixer source voice '%s'."), *InPlaybackInstance->GetName());
			}

		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Num channels was 0 for sound buffer '%s'."), *InPlaybackInstance->GetName());
		}

		FreeResources(EFreeReason::Error);
		return false;
	}

	bool FMixerSource::UseObjectBasedSpatialization() const
	{
		if (PlaybackInstance->SpatializationMethod == EAtomSpatializationAlgorithm::Binaural)
		{
			auto SpatializationRack = AtomRuntime->GetSpatializationRack();

			int MaxChannelsSupportedBySpatialization = 16;
			if (auto SoundfieldRack = Cast<UAtomSoundfieldRack>(SpatializationRack))
			{
				if (SoundfieldRack->GetRackType() == EAtomSoundfieldRendererType::SoundObject)
				{
					MaxChannelsSupportedBySpatialization = 1;
				}
			}

			if (AtomRuntime->IsAtomMixerPluginEnabled())
			{
				auto SpatializationBusIndex = AtomRuntime->GetSpatializationBusIndex();
				if (SpatializationBusIndex < 0)
				{
					if (UAtomRack* AtomRack = Cast<UAtomRack>(SpatializationRack))
					{
						if (AtomRack->Buses.IsValidIndex(SpatializationBusIndex))
						{
							check(AtomRack->Buses[SpatializationBusIndex]);

							if (auto Endpoint = Cast<UAtomSoundfieldEndpoint>(AtomRack->Buses[SpatializationBusIndex]->Endpoint))
							{
								if (Endpoint->SoundfieldRendererType == EAtomSoundfieldRendererType::SoundObject)
								{
									MaxChannelsSupportedBySpatialization = 1;
								}
							}
						}
					}
				}
			}

			// check if spatialization rack matches
			return SpatializationRack && WaveInfo.NumChannels <= MaxChannelsSupportedBySpatialization;
		}

		return false;

		/*return (Buffer->NumChannels <= MixerDevice->GetCurrentSpatializationPluginInterfaceInfo().MaxChannelsSupportedBySpatializationPlugin &&
			AudioDevice->IsSpatializationPluginEnabled() &&
			WaveInstance->SpatializationMethod == ESoundSpatializationAlgorithm::SPATIALIZATION_HRTF);*/	
	}

	bool FMixerSource::IsUsingObjectBasedSpatialization() const
	{
		bool bIsUsingObjectBaseSpatialization = UseObjectBasedSpatialization();

		if (MixerSourceVoice)
		{
			// If it is currently playing, check whether it actively uses HRTF spatializer.
			// HRTF spatialization cannot be altered on currently playing source. So this handles
			// the case where the source was initialized without HRTF spatialization before HRTF
			// spatialization is enabled. 
			bool bDefaultIfNoSourceID = true;
			bIsUsingObjectBaseSpatialization &= MixerSourceVoice->IsUsingHRTFSpatializer(bDefaultIfNoSourceID);
		}
		return bIsUsingObjectBaseSpatialization;
	}

	bool FMixerSource::UseSourceDataOverridePlugin() const
	{
		/*return (Buffer->NumChannels == 1 || Buffer->NumChannels == 2) &&
			AudioDevice->IsSourceDataOverridePluginEnabled() &&
			WaveInstance->SourceDataOverridePluginSettings != nullptr;*/
		return PlaybackInstance->SourceDataOverridePluginSettings != nullptr;
	}

	int64 FMixerSource::GetNumFramesPlayed() const
	{
		if (bIsInitialized && MixerSourceVoice != nullptr)
		{
			return MixerSourceVoice->GetNumFramesPlayed();
		}

		return 0;
	}

	/*
	 * FAtomPlayer implementation
	 *****************************************************************************/

	namespace FAtomPlayer_NativeCallbacks
	{
		extern "C" void CRIAPI OnPcmDecode(void* Obj, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[])
		{
			// ADX do not call this function if player is destroyed
			if (FAtomPlayer* Self = static_cast<FAtomPlayer*>(Obj))
			{
				Self->HandleNativePlayerOnPcmDecode(Format, NumChannels, NumSamples, Data);
			}
		}
	}

	// this occur from atom thread
	void FAtomPlayer::HandleNativePlayerOnPcmDecode(CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumFrames, void* Data[])
	{
		// this event is not executed on main thread for performance purpose - user code needs to use some CriticalSections and WeakPtrs.
		if (PlaybackInstance && PlaybackInstance->ActiveSound)
		{
			auto ActiveSound = PlaybackInstance->ActiveSound;
			if (ActiveSound->PcmFilterFunction)
			{
				ActiveSound->PcmFilterFunction(PlaybackInstance->PlaybackInstanceHash, FAtomRuntime::GetPcmBitDepthFromAtomPcmFormat(Format), (int32)NumChannels, (int32)NumFrames, Data);
			}
		}
	}

	FAtomPlayer::FAtomPlayer(FAtomRuntime* InAtomRuntime)
		: FAtomSource(InAtomRuntime)
		, bIsPreparingForInit(false)
		, bIsLoadingResource(false)
		, bIsPreparingPlayer(false)
		, bIsPrepareFailed(false)
		, bIsStopping(false)
	{
		bIsInitialized = false;
	}

	FAtomPlayer::~FAtomPlayer()
	{
		FreeResources();
	}

	void FAtomPlayer::FreeResources()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		if (Player.IsValid())
		{
			// criAtomExPlayer_Destroy
			Player.Reset();
		}

		bIsInitialized = false;
		bIsPreparingPlayer = false;
	}

	bool FAtomPlayer::PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance)
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		if (!ensure(InPlaybackInstance))
		{
			return false;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomPlayer::PrepareForInitialization);

		// select output if not default or native (uses master rack by default)
		CriAtomSoundRendererType AsrType = CRIATOM_SOUND_RENDERER_DEFAULT;
		CriAtomExAsrRackId RackId = 0;

		check(InPlaybackInstance);
		check(AtomRuntime);

		auto SoundRack = InPlaybackInstance->SoundRack;
		if (SoundRack)
		{
			if (SoundRack->IsA(UAtomEndpointRack::StaticClass()))
			{
				UAtomEndpointRack* EndpointRack = Cast<UAtomEndpointRack>(SoundRack);
				AsrType = GCriWare->GetActiveAtomRuntime()->GetAtomExSoundRendererType(EndpointRack->SoundRendererType); // this may not work on some platform!
			}
			else if (SoundRack->IsA(UAtomRack::StaticClass()))
			{
				RackId = GCriWare->GetActiveAtomRuntime()->GetAsrRackId(SoundRack);
			}
		}

		// SoundData must be valid beyond this point, otherwise Player would have failed to init.
		check(InPlaybackInstance->SoundData);

		WaveInfo = InPlaybackInstance->SoundData->WaveInfo;

		UAtomSoundBank* SoundBank = nullptr;
		bool bProcedural = InPlaybackInstance->SoundData->bProcedural;
		bool bIsBus = InPlaybackInstance->SoundData->bIsSourceBus;

		if (auto SoundCue = Cast<UAtomSoundCue>(InPlaybackInstance->SoundData))
		{
			SoundBank = SoundCue->CueSheet;
		}
		else if (auto SoundWave = Cast<UAtomSoundWave>(InPlaybackInstance->SoundData))
		{
			SoundBank = SoundWave->WaveBank;
		}
		else if (Cast<UAtomExternalSound>(InPlaybackInstance->SoundData))
		{
			// External AtomExPlayer, not usable with AtomPlayer.
			return false;
		}

		if (bProcedural) // no bank
		{
			uint32 PlayOrder = InPlaybackInstance->ActiveSound->GetPlayOrder();
			uint64 InstanceID = InPlaybackInstance->ActiveSound->GetAtomComponentID();
			bool bActiveSoundIsPreviewSound = InPlaybackInstance->ActiveSound->bIsPreviewSound;

			// todo: may be move this to soucevoice too
			// try to get generator if exist and assign it to ADX pcm filter
			auto SoundWaveProcedural = Cast<UAtomSoundWaveProcedural>(InPlaybackInstance->SoundData);
			FAtomSoundGeneratorInitParams InitParams;
			InitParams.AtomRuntimeID = AtomRuntime->GetAtomRuntimeID();
			InitParams.AtomComponentID = InstanceID;
			InitParams.SampleRate = WaveInfo.SampleRate;
			InitParams.AtomMixerNumOutputFrames = FAtomRuntimeManager::Get()->GetDefaultDSPBufferLength();
			InitParams.NumChannels = WaveInfo.NumChannels;
			InitParams.NumFramesPerCallback = WaveInfo.NumFrames;
			//InitParams.InstanceID = Audio::GetTransmitterID(InitParams.AtomComponentID, PlaybackInstance->PlaybackInstanceHash, PlayOrder);
			InitParams.InstanceID = HashCombineFast(static_cast<uint32>(InstanceID % TNumericLimits<uint32>::Max()), PlayOrder + PlaybackInstance->PlaybackInstanceHash);
			InitParams.bIsPreviewSound = bActiveSoundIsPreviewSound;
			IAtomSoundGeneratorPtr SoundGenerator = SoundWaveProcedural->CreateSoundGenerator(InitParams);
			if (SoundGenerator.IsValid())
			{
				InPlaybackInstance->ActiveSound->PcmFilterFunction = [SoundGenerator](UPTRINT, EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
					{
						if (SoundGenerator.IsValid() && !SoundGenerator->IsFinished())
						{
							const int32 NumRequestedSamples = NumFrames * NumChannels;
							int32 NumSampleWritten = SoundGenerator->GetNextBuffer((float**)Data, NumFrames, NumChannels, true);
							if (NumSampleWritten < NumRequestedSamples)
							{
								UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Input Port Buffer Underrun."));
							}
						}
					};
			}
		}
		else if (UAtomWaveBank* WaveBank = Cast<UAtomWaveBank>(SoundBank))
		{
			bIsPreparingForInit = true;
			PlaybackInstance = InPlaybackInstance;

			CreatePlayerTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WeakWaveBank = MakeWeakObjectPtr(WaveBank), AsrType, RackId, InPlaybackInstance]()
			{
				if (!WeakWaveBank.IsValid())
				{
					bIsPreparingForInit = false;
					return;
				}

				// ask loading
				if (!WeakWaveBank->IsLoaded())
				{
					WeakWaveBank->RetainResource();
				}

				CriAtomStandardPlayerConfig Config;
				criAtomPlayer_SetDefaultConfigForStandardPlayer(&Config);
				Config.sound_renderer_type = AsrType;

				// awb
				Config.max_channels = WaveInfo.NumChannels;
				Config.max_sampling_rate = WaveInfo.SampleRate;
				Config.streaming_flag = WaveInfo.bIsStreamed;

				CriAtomPlayerHn AtomPlayerHn = FCriWareApi::criAtomPlayer_CreateStandardPlayer(&Config, nullptr, 0);
				if (!AtomPlayerHn)
				{
					bIsPreparingForInit = false;
					return;
				}

				// when created continue Atom player prepare sequence on Mainthread
				AsyncTask(ENamedThreads::GameThread, [this, AtomPlayerHn, WeakWaveBank, WaveID = WaveInfo.WaveID, RackId, InPlaybackInstance]()
				{
					Player = MakeCriHandle(AtomPlayerHn);

					// Set rack id not default or native (uses master rack (0) by default)
					if (RackId > 0)
					{
						FCriWareApi::criAtomPlayer_SetAsrRackId(AtomPlayerHn, (CriSint32)RackId);
					}

					auto ThisWaveBank = WeakWaveBank.Get();
					if (!ThisWaveBank)
					{
						FreeResources();
						return;
					}

					if (auto Resource = ThisWaveBank->AtomResource.GetResource())
					{
						auto NativeHandle = Resource->GetWaveBankResource()->GetNativeHandle();

						if (InPlaybackInstance->ActiveSound->PcmFilterFunction)
						{
							FCriWareApi::criAtomPlayer_SetDecodeCallback(Player, FAtomPlayer_NativeCallbacks::OnPcmDecode, this);
							FCriWareApi::criAtomPlayer_LimitLoopCount(Player, CRIATOMPLAYER_IGNORE_LOOP);
						}

						FCriWareApi::criAtomPlayer_SetWaveId(Player, NativeHandle, WaveID);
						//FCriWareApi::criAtomPlayer_SetFile(Player, nullptr, SoundWave->);

						// Set startup settings
						UpdateVolume();

						FCriWareApi::criAtomPlayer_Pause(Player, CRI_TRUE);
						FCriWareApi::criAtomPlayer_Start(Player);
					}
					else
					{
						bIsPrepareFailed = true;
					}
				});
			}, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);

			return true;
		}
		else if (UAtomCueSheet* CueSheet = Cast<UAtomCueSheet>(SoundBank))
		{
			bIsPreparingForInit = true;
			PlaybackInstance = InPlaybackInstance;

			CreatePlayerTask = FFunctionGraphTask::CreateAndDispatchWhenReady([this, WeakCueSheet = MakeWeakObjectPtr(CueSheet), AsrType, RackId, InPlaybackInstance]()
			{
				if (!WeakCueSheet.IsValid())
				{
					bIsPreparingForInit = false;
					return;
				}

				// ask loading
				if (!WeakCueSheet->IsLoaded())
				{
					WeakCueSheet->RetainResource();
				}

				CriAtomStandardPlayerConfig Config;
				criAtomPlayer_SetDefaultConfigForStandardPlayer(&Config);
				Config.sound_renderer_type = AsrType;

				// acb
				Config.max_channels = WaveInfo.NumChannels;
				Config.max_sampling_rate = WaveInfo.SampleRate;
				Config.streaming_flag = WaveInfo.bIsStreamed;

				CriAtomPlayerHn AtomPlayerHn = FCriWareApi::criAtomPlayer_CreateStandardPlayer(&Config, nullptr, 0);
				if (!AtomPlayerHn)
				{
					bIsPreparingForInit = false;
					return;
				}

				DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.PreparePlayer"), STAT_AtomPreparePlayer, STATGROUP_TaskGraphTasks);

				// when created continue Atom player prepare sequence on Atom thread
				FAtomThread::RunCommandOnAtomThread([this, AtomPlayerHn, WeakCueSheet, WaveID = WaveInfo.WaveID, RackId, InPlaybackInstance]()
				{
					Player = MakeCriHandle(AtomPlayerHn);

					// Set rack id if not default or native (uses master rack (0) by default)
					if (RackId > 0)
					{
						FCriWareApi::criAtomPlayer_SetAsrRackId(Player, (CriSint32)RackId);
					}

					auto ThisCueSheet = WeakCueSheet.Get();
					if (!ThisCueSheet)
					{
						FreeResources();
						return;
					}

					if (auto Resource = ThisCueSheet->AtomResource.GetResource())
					{
						auto CueSheetResource = Resource->GetCueSheetResource();
						auto NativeHandle = CueSheetResource->GetNativeAwbHandle(WaveInfo.bIsStreamed ? 0 : INDEX_NONE);
						// obtain the used wavebank by this cue sheet

						if (InPlaybackInstance->ActiveSound->PcmFilterFunction)
						{
							FCriWareApi::criAtomPlayer_SetDecodeCallback(Player, FAtomPlayer_NativeCallbacks::OnPcmDecode, this);
							FCriWareApi::criAtomPlayer_LimitLoopCount(Player, CRIATOMPLAYER_IGNORE_LOOP);
						}

						FCriWareApi::criAtomPlayer_SetWaveId(Player, NativeHandle, WaveID);
						//FCriWareApi::criAtomPlayer_SetFile(Player, nullptr, SoundWave->);

						// Set startup settings
						UpdateVolume();

						FCriWareApi::criAtomPlayer_Pause(Player, CRI_TRUE);
						FCriWareApi::criAtomPlayer_Start(Player);
					}
					else
					{
						bIsPrepareFailed = true;
					}
				}, GET_STATID(STAT_AtomPreparePlayer));

			}, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);

			return true;
		}

		return false;
	}

	bool FAtomPlayer::IsPreparedToInit()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);
		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomPlayer::IsPreparedToInit);

		if (bIsPreparingForInit && Player.IsValid())
		{
			Status = FCriWareApi::criAtomPlayer_GetStatus(Player);
			if (Status == CRIATOMPLAYER_STATUS_PLAYING
				|| Status == CRIATOMPLAYER_STATUS_PLAYEND
				|| Status == CRIATOMPLAYER_STATUS_ERROR)
			{
				bIsPrepareFailed = Status == CRIATOMPLAYER_STATUS_ERROR;
				return true;
			}
		}

		if (!bIsPreparingForInit && !Player.IsValid())
		{
			bIsPrepareFailed = true;
			return true;
		}

		return false;
	}

	bool FAtomPlayer::Init(FAtomPlaybackInstance* InPlaybackInstance)
	{
		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomPlayer::Init);
		//AUDIO_MIXER_CHECK(MixerBuffer);
		//AUDIO_MIXER_CHECK(MixerBuffer->IsRealTimeSourceReady());

		// We've already been passed the playback instance in PrepareForInitialization, make sure we have the same one
		ATOM_MIXER_CHECK(PlaybackInstance && PlaybackInstance == InPlaybackInstance);

		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		FAtomSource::InitCommon();

		if (!ensure(InPlaybackInstance))
		{
			return false;
		}

		bIsPreparingForInit = false;

		if (!bIsPrepareFailed && Player.IsValid() && Status == CRIATOMPLAYER_STATUS_PLAYING)
		{
			if (InPlaybackInstance->ActiveSound->PcmFilterFunction)
			{
				FCriWareApi::criAtomPlayer_DiscardSamples(Player, WaveInfo.NumFrames);
				//FCriWareApi::criAtomPlayer_Pause(Player, CRI_FALSE);
			}

			bIsInitialized = true;
			return true;
		}

		bIsPrepareFailed = false;
		FreeResources();
		return false;
	}

	void FAtomPlayer::Update()
	{
		CSV_SCOPED_TIMING_STAT(Atom, UpdateSources);
		SCOPE_CYCLE_COUNTER(STAT_AtomUpdateSources);

		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		if (!PlaybackInstance || !Player.IsValid() || bIsPaused || (!bIsInitialized && !bIsPreparingForInit))
		{
			return;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomPlayer::Update);

		Status = FCriWareApi::criAtomPlayer_GetStatus(Player);

		FAtomSource::UpdateCommon();

		TickCount++;

		UpdateVolume();

#if ENABLE_ATOM_DEBUG
		Atom::FAtomDebugger::DrawDebugInfo(*this);
#endif // ENABLE_ATOM_DEBUG
	}

	void FAtomPlayer::UpdateVolume()
	{
		check(PlaybackInstance);

		float CurrentVolume = 0.0f;
		if (!AtomRuntime->IsAtomRuntimeMuted())
		{
			FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
			check(ActiveSound);

			// 1. Apply device gain stage(s)
			CurrentVolume = ActiveSound->bIsPreviewSound ? 1.0f : AtomRuntime->GetPrimaryVolume();
			//CurrentVolume *= AudioDevice->GetPlatformAudioHeadroom();

			// 2. Gain stages
			CurrentVolume *= PlaybackInstance->GetVolume();
			CurrentVolume *= PlaybackInstance->GetDynamicVolume();

			// 3. Editor gain stage
			CurrentVolume = FMath::Clamp<float>(GetDebugVolume(CurrentVolume), 0.0f, ATOM_MAX_VOLUME);

			// 4. Modulations
			UAtomSoundBase* Sound = PlaybackInstance->SoundData;
			check(Sound);
			const float ModVolumeBase = AtomModulationUtils::GetRoutedVolume(*PlaybackInstance, *Sound, *ActiveSound);
		}

		FCriWareApi::criAtomPlayer_SetVolume(Player, CurrentVolume);
	}

	void FAtomPlayer::Play()
	{
#ifdef ATOM_SOUND_STATUS_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Play %d (Atom Voice)"), Status);
#endif
		if (!PlaybackInstance)
		{
			return;
		}

		// Don't restart the sound if it was stopping when we paused, just stop it.
		if (bIsPaused && (bIsStopping /*|| IsPlaybackFinished(MainPlaybackID)*/))
		{
#ifdef ATOM_SOUND_STATUS_DEBUG
			UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Stop %d (Atom Voice)"), Status);
#endif
			StopNow();
			return;
		}

		if (bIsStopping)
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Restarting an Atom source player which was stopping. Stopping now."));
			return;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomPlayer::Play);

		if (Player.IsValid() && bIsInitialized)
		{
#ifdef ATOM_SOUND_STATUS_DEBUG
			UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Resume %d (Atom Voice)"), Status);
#endif
			if (Status == CRIATOMPLAYER_STATUS_PLAYING)
			{
				FCriWareApi::criAtomPlayer_Pause(Player, CRI_FALSE);
			}
			else if (IsPlaybackFinished())
			{
				FCriWareApi::criAtomPlayer_Start(Player);
			}
			else
			{
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("AtomPlayer is not ready for playback."));
			}
		}

		bIsPaused = false;
		bIsPlaying = true;
		bIsStopping = false;
	}

	void FAtomPlayer::Pause()
	{
		if (!PlaybackInstance)
		{
			return;
		}

		if (bIsStopping)
		{
			return;
		}

		if (Player.IsValid())
		{
			FCriWareApi::criAtomPlayer_Pause(Player, CRI_TRUE);
		}

		bIsPaused = true;
	}

	void FAtomPlayer::Stop()
	{
		if (!bIsInitialized)
		{
			return;
		}

		if (!Player.IsValid())
		{
			StopNow();
			return;
		}

		UAtomSoundBase* Sound = PlaybackInstance ? PlaybackInstance->SoundData : nullptr;
		// If MarkAsGarbage() was called, Sound can be null
		if (!Sound)
		{
			StopNow();
			return;
		}

		// Stop procedural sounds immediately that don't require fade
		/*if (Sound->bProcedural && !Sound->bRequiresStopFade)
		{
			StopNow();
			return;
		}*/

		if (IsPlaybackFinished())
		{
			StopNow();
			return;
		}

		// Otherwise, we need to do a quick fade-out of the sound and put the state
		// of the sound into "stopping" mode. This prevents this source from
		// being put into the "free" pool and prevents the source from freeing its resources
		// until the sound has finished naturally (i.e. faded all the way out)

		if (!bIsStopping)
		{
			// Let the playback instance know it's stopping
			PlaybackInstance->SetStopping(true);

#ifdef  ATOM_SOUND_DEV_DEBUG
			UE_LOG(LogCriWareAtomMixerDebug, Error, TEXT("call stop (atom voice)"));
#endif

			FCriWareApi::criAtomPlayer_Stop(Player);
			bIsStopping = true;
			bIsPaused = false;

			// FAtomSource::Stop(); // Will be excuted by StopNow()
		}
	}

	void FAtomPlayer::StopNow()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		bIsInitialized = false;
		bIsStopping = false;

		if (PlaybackInstance)
		{
		}

		if (Player.IsValid())
		{
			if (bIsPlaying)
			{
				FCriWareApi::criAtomPlayer_ForceStop(Player);
			}
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("AtomPlayer is not setup."));
		}

		bIsPaused = false;
		bIsPlaying = false;

		// Free/reset Parameters and Resources

		FreeResources();

		FAtomSource::Stop();
	}

	bool FAtomPlayer::IsStopping()
	{
		return bIsStopping;
	}

	bool FAtomPlayer::IsFinished()
	{
		// A paused source is not finished.
		if (bIsPaused)
		{
			return false;
		}

		if (!bIsInitialized)
		{
			return true;
		}

		if (bIsPreparingForInit)
		{
			return false;
		}

		if (PlaybackInstance && Player.IsValid())
		{
			if (IsPlaybackFinished())
			{
				PlaybackInstance->NotifyFinished();
				bIsStopping = false;
				return true;
			}
			//else if (bLoopCallback && WaveInstance->LoopingMode == LOOP_WithNotification)
			//{
			//	WaveInstance->NotifyFinished();
			//	bLoopCallback = false;
			//}
		}

		return false;
	}

	bool FAtomPlayer::IsPlaybackFinished()
	{
		return Status == CRIATOMPLAYER_STATUS_STOP || Status == CRIATOMPLAYER_STATUS_PLAYEND;
	}

	/* FAtomExPlayback implementation
	 *****************************************************************************/

	 // this occur from atom render thread / and can happen from User thread too
	void FAtomExPlaybackListener::OnPlaybackEvent(EPlaybackEvent PlaybackEvent)
	{
		if (Owner)
		{
			if (PlaybackEvent == EPlaybackEvent::AllocateAndVirtualized || PlaybackEvent == EPlaybackEvent::Virtualized)
			{
				// update source status
				Owner->bIsVirtual = true;
			}
			else if (PlaybackEvent == EPlaybackEvent::Realized)
			{
				// update source status
				Owner->bIsVirtual = false;
			}
			
			// Try get playback info if not yet.
			FAtomExPlayer::GetPlaybackInfo(Owner->PlaybackInstance, PlaybackID);
		}
	}

	// this occur from atom render thread
	void FAtomExPlaybackListener::OnCueBlockIndexChanged(int32 BlockIndex)
	{
		if (!IsInAtomThread())
		{
			TWeakPtr<FAtomExPlaybackListener, ESPMode::ThreadSafe> LambdaWeakThis = AsShared();

			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SetCueBlockIndex"), STAT_AtomSetCueBlockIndex, STATGROUP_AtomThreadCommands);
			FAtomThread::RunCommandOnAtomThread([LambdaWeakThis, BlockIndex]()
			{
				if (TSharedPtr<FAtomExPlaybackListener, ESPMode::ThreadSafe> StrongThis = LambdaWeakThis.Pin(); StrongThis.IsValid())
				{
					if (StrongThis->PlaybackInstance)
					{
						if (FAtomActiveSound* ActiveSound = StrongThis->PlaybackInstance->ActiveSound)
						{
							ActiveSound->SetCueBlockIndex(BlockIndex);
						}
					}
				}
			}, GET_STATID(STAT_AtomSetCueBlockIndex));
		}
		else
		{
			if (PlaybackInstance && PlaybackInstance->ActiveSound)
			{
				PlaybackInstance->ActiveSound->SetCueBlockIndex(BlockIndex);
			}
		}
	}

	// this occur from atom render thread
	void FAtomExPlaybackListener::OnFilter(EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
	{
		// this event is not executed on main thread for performance purpose - user code needs to use some CriticalSections and WeakPtrs.
		if (PlaybackInstance && PlaybackInstance->ActiveSound)
		{
			auto ActiveSound = PlaybackInstance->ActiveSound;
			if (ActiveSound->PcmFilterFunction)
			{
				const UPTRINT ChildHash = static_cast<UPTRINT>(PlaybackID);
				ActiveSound->PcmFilterFunction(ChildHash, BitDepth, NumChannels, NumFrames, Data);
			}
		}

		// temp - send audio to voice and source manager for bus mix and quantization 
		if (Owner && Owner->MixerSourceVoice)
		{
			Owner->MixerSourceVoice->SubmitBuffer((float**)Data, NumChannels, NumFrames);
		}
	}

	FAtomExPlayback::FAtomExPlayback(FAtomRuntime* InAtomRuntime)
		: FMixerSource(InAtomRuntime)
		, MixerPlayer(MakeShared<FAtomExPlayer>())
		, PlaybackID(INDEX_NONE)
		, Status(CRIATOMEXPLAYBACK_STATUS_STOP)
		, InitializationState(EAtomExPlaybackInitializationState::NotInitialized)
		, bIsPreparingForInit(false)
		, bIsLoadingResource(false)
		, bIsPreparingPlayer(false)
		, bIsPrepareFailed(false)
		, bIsStopping(false)
		, bIsExternal(false)
		, bIs3D(false)
		, bAttenuationReady(false) //
		, bPreviousSourceBusEnablement(false)
		, bPreviousBaseRackEnablement(false)
		, bSendingAudioToSourceBuses(false)
		, PreviousPlaybackPercent(0.0f)
		, PreviousAzimuth(-1.0f)
		, PreviousCueNextBlockIndex(INDEX_NONE)
	{
		bIsInitialized = false;
	}

	FAtomExPlayback::~FAtomExPlayback()
	{
		FreeResources(EFreeReason::Deleter);

#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("Player destructor: Afer free resources ExPlayer=%d ExSource3d=%d bIs3d=%d bIsPlaying=%d"), MixerPlayer->IsInitialized(), MixerPlayer->ExSource.IsValid(), bIs3D, (bool)bIsPlaying);
#endif
	}

	void FAtomExPlayback::FreeResources(EFreeReason InReason)
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);
#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("FreeResources: %s ExPlayer=%d ExSource3d=%d bIs3d=%d bIsPlaying=%d"), ToString(InReason), MixerPlayer->IsInitialized(), MixerPlayer->ExSource.IsValid(), bIs3D, (bool)bIsPlaying);
#endif
		// remove associations
		//if (bIs3D && (ExSource.IsValid() || ExSourceList.IsValid()))
		//{
		//	RemoveSpatialization();
		//}

		// clear settings
		if (MixerPlayer->ExPlayer.IsValid())
		{
			if (MixerSourceVoice)
			{
				uint32 SourceID = MixerSourceVoice->GetSourceID();

				// Release the source using the propagation interface
				if (AtomRuntime->SourceDataOverridePluginInterface)
				{
					AtomRuntime->SourceDataOverridePluginInterface->OnReleaseSource(SourceID);
				}

				for (auto RuntimePluginInterface : AtomRuntime->RuntimePluginInterfaces)
				{
					RuntimePluginInterface->OnReleaseSource(SourceID);
				}
			}

			// remove listener with source player
			if (PlaybackID != CRIATOMEX_INVALID_PLAYBACK_ID)
			{
				MixerPlayer->UnregisterPlaybackListener(PlaybackID);
			}

			// Apply reset updated before destroying
			FCriWareApi::criAtomExPlayer_Update(MixerPlayer->ExPlayer, PlaybackID);
		}

		// Invalidate the player reference
		MixerPlayer = MakeShared<FAtomExPlayer>();

		// Remove voice
		if (MixerSourceVoice)
		{
			MixerSourceVoice->Release();
			MixerSourceVoice = nullptr;
		}

		// destroy 3d sources
		//ExSource.Reset();
		//ExSources.Reset();
		//ExSourceList.Reset();
		bIs3D = false;
		bAttenuationReady = false; //

		AtomListener.Reset();
		MixerSourcePlayer = {};
		//MixerSourceBuffer.Reset(); // Next STEP use a pcm buffer object and dont send the atomexplayer to manager
		NumTotalFrames = 0;

		InitializationState = EAtomExPlaybackInitializationState::NotInitialized;
		PlaybackID = INDEX_NONE;
		bIsInitialized = false;
		bIsPreparingPlayer = false;
		bIsExternal = false;
	}

	// cri_set_sound, cri_prepare
	bool FAtomExPlayback::PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance)
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		if (!ensure(InPlaybackInstance))
		{
			return false;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomExPlayback::PrepareForInitialization);

		// We are not initialized yet. We won't be until the sound file finishes loading and parsing the header.
		InitializationState = EAtomExPlaybackInitializationState::Initializing;

		const bool bIsSeeking = InPlaybackInstance->StartTime > 0.0f;

		check(InPlaybackInstance);
		check(AtomRuntime);

		if (InPlaybackInstance)
		{
			// SoundData must be valid beyond this point.
			check(InPlaybackInstance->SoundData);
			UAtomSoundBase& Sound = *InPlaybackInstance->SoundData;

			PlaybackInstance = InPlaybackInstance;

			LPFFrequency = ATOM_MAX_FILTER_FREQUENCY;
			LastLPFFrequency = FLT_MAX;

			HPFFrequency = 0.0f;
			LastHPFFrequency = FLT_MAX;

			// setup time
			// Not all wave data types have a non-zero duration
			if (Sound.GetDuration() > 0.0f)
			{
				if (!Sound.bIsSourceBus)
				{
					NumTotalFrames = Sound.WaveInfo.NumFrames;
					//check(NumTotalFrames > 0); // todo handle action cue
				}
				else if (!Sound.IsLooping())
				{
					NumTotalFrames = Sound.Duration * AtomRuntime->GetRuntimeSampleRate();
					check(NumTotalFrames > 0);
				}

				StartFrame = FMath::Clamp<int32>((InPlaybackInstance->StartTime / Sound.Duration) * NumTotalFrames, 0, NumTotalFrames);
			}

			bIsExternal = MixerPlayer->IsExternal();

			if (!bIsExternal)
			{
				check(!MixerPlayer->IsInitialized());

				// Active sound instance ID is the Atom component ID of active sound.
				uint64 InstanceID = 0;
				FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
				if (ActiveSound)
				{
					InstanceID = ActiveSound->GetAtomComponentID();
				}

				// MixerPlayer (MixerBuffer)
				FAtomExPlayerArgs InitPlayerArgs;
				InitPlayerArgs.AtomRuntime = AtomRuntime;
				InitPlayerArgs.ActiveSound = ActiveSound;
				InitPlayerArgs.SoundData = PlaybackInstance->SoundData;
				MixerPlayer = FAtomExPlayer::Create(InitPlayerArgs).ToSharedRef();
				MixerSourcePlayer.Player = MixerPlayer;
				MixerSourcePlayer.PlaybackID = static_cast<uint32>(PlaybackInstance->PlaybackInstanceHash);
			}

			if (!MixerPlayer->IsInitialized())
			{
				FreeResources(EFreeReason::Error);
				return false;
			}

			// Load sound Buffer in Atom (MixerSourceBuffer)
			if (LoadSoundInternal(InPlaybackInstance))
			{
				bIsPreparingForInit = true;

				// loading or ready
				return true;
			}
		}

		FreeResources(EFreeReason::Error);
		return false;
	}

	bool FAtomExPlayback::LoadSoundInternal(FAtomPlaybackInstance* InPlaybackInstance)
	{
		// copy sound wave info
		WaveInfo = InPlaybackInstance->SoundData->WaveInfo;

		// try access or load assets if not available
		UAtomSoundBank* SoundBank = nullptr;
		uint32 SoundID = 0;
		bool bProcedural = InPlaybackInstance->SoundData->bProcedural;
		bool bIsBus = InPlaybackInstance->SoundData->bIsSourceBus;
		
		if (auto SoundCue = Cast<UAtomSoundCue>(InPlaybackInstance->SoundData))
		{
			SoundBank = SoundCue->CueSheet;
			SoundID = SoundCue->CueInfo.ID;
			// -> next atom will have a cueinfo or a function to know. if using awb file - todo: check in case the awb is from memory in a specific case :p 
		}
		else if (auto SoundWave = Cast<UAtomSoundWave>(InPlaybackInstance->SoundData))
		{
			SoundBank = SoundWave->WaveBank;
			SoundID = SoundWave->WaveID;
		}
		else if (auto ExternalSound = Cast<UAtomExternalSound>(InPlaybackInstance->SoundData))
		{
			bIsLoadingResource = false;
			// init externalvoicepool
			InitSourceVoice(InPlaybackInstance);

			// if procedural external sound uses a filter callback, assign here to the source DSP chain.
			if (auto PcmFilter = ExternalSound->GetPcmFilterFunction())
			{
				InPlaybackInstance->ActiveSound->PcmFilterFunction = PcmFilter;
			}

			return true;
		}

		bool bIsLoaded = false;

		if (bProcedural || bIsBus)
		{
			bIsLoadingResource = false;

			// init audioinput			
			InitSourceVoice(InPlaybackInstance);

			void* Port = nullptr;
			if (MixerSourceVoice)
			{
				Port = MixerSourceVoice->GetInputPort();
			}
			if (!Port)
			{
				// error
				return false;
			}

			// set input port
			FCriWareApi::criAtomExPlayer_SetInputPort(MixerPlayer->ExPlayer, Port);
			FCriWareApi::criAtomExPlayer_SetPan3dAngle(MixerPlayer->ExPlayer, 0.0f);

			uint32 PlayOrder = InPlaybackInstance->ActiveSound->GetPlayOrder();
			uint64 InstanceID = InPlaybackInstance->ActiveSound->GetAtomComponentID();
			bool bActiveSoundIsPreviewSound = InPlaybackInstance->ActiveSound->bIsPreviewSound;

			// Retrieve a sound generator if this is a procedural sound wave
			if (bProcedural)
			{
				// todo: may be move this to soucevoice too
				// try to get generator if exist and assign it to ADX pcm filter
				auto SoundWaveProcedural = Cast<UAtomSoundWaveProcedural>(InPlaybackInstance->SoundData);
				FAtomSoundGeneratorInitParams InitParams;
				InitParams.AtomRuntimeID = AtomRuntime->GetAtomRuntimeID();
				InitParams.AtomComponentID = InstanceID;
				InitParams.SampleRate = WaveInfo.SampleRate;
				InitParams.AtomMixerNumOutputFrames = FAtomRuntimeManager::Get()->GetDefaultDSPBufferLength();
				InitParams.NumChannels = WaveInfo.NumChannels;
				InitParams.NumFramesPerCallback = WaveInfo.NumFrames;
				//InitParams.InstanceID = Audio::GetTransmitterID(InitParams.AtomComponentID, PlaybackInstance->PlaybackInstanceHash, PlayOrder);
				InitParams.InstanceID = HashCombineFast(static_cast<uint32>(InstanceID % TNumericLimits<uint32>::Max()), PlayOrder + PlaybackInstance->PlaybackInstanceHash);
				InitParams.bIsPreviewSound = bActiveSoundIsPreviewSound;

				// todo: procedural voice buffer: actually soundgenerator lifespan is held by activesound tru pcmfilter lambda
				IAtomSoundGeneratorPtr SoundGenerator = SoundWaveProcedural->CreateSoundGenerator(InitParams);
				if (SoundGenerator.IsValid())
				{
					InPlaybackInstance->ActiveSound->PcmFilterFunction = [SoundGenerator](UPTRINT, EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
					{
						if (SoundGenerator.IsValid() && !SoundGenerator->IsFinished())
						{
							const int32 NumRequestedSamples = NumFrames * NumChannels;
							int32 NumSampleWritten = SoundGenerator->GetNextBuffer((float**)Data, NumFrames, NumChannels, true);
							if (NumSampleWritten < NumRequestedSamples)
							{
								UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Input Port Buffer Underrun."));
							}
						}
					};
				}
			}
			else // bIsBus
			{
				// todo: bus voice buffer: actually bus lifespan is held by activesound tru pcmfilter lambda
				InPlaybackInstance->ActiveSound->PcmFilterFunction = [InstanceID = InstanceID, RuntimeID = AtomRuntime->GetAtomRuntimeID()](UPTRINT, EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
				{
					if (FAtomRuntimeManager* ARM = FAtomRuntimeManager::Get())
					{
						if (FAtomRuntime* AtomRuntime = ARM->GetAtomRuntimeRaw(RuntimeID))
						{
							UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
							check(AudioBusSubsystem);
							AudioBusSubsystem->ConnectPatches(InstanceID);
						}
					}
				};
			}

			// enable the pcm filter for the listener
			MixerPlayer->EnablePcmFilter(true);

			bIsLoaded = true;
		}
		// set wave bank+index
		else if (UAtomWaveBank* WaveBank = Cast<UAtomWaveBank>(SoundBank))
		{
			bIsLoaded = WaveBank->IsLoaded();

			if (!bIsLoaded)
			{
				if (!bIsPreparingForInit)
				{
					// async load
					WaveBank->RetainResource();
					bIsLoadingResource = true;
				}
			}
			else
			{
				bIsLoadingResource = false;

				if (auto Resource = WaveBank->AtomResource.GetResource())
				{
					auto WaveBankResource = Resource->GetWaveBankResource();

					// set the voice pool to use					
					InitSourceVoice(InPlaybackInstance);

					if (MixerPlayer->ExPlayer.IsValid())
					{
						if (MixerSourceVoice)
						{
							const int32 VoicePoolID = MixerSourceVoice->GetVoicePoolID();
							FCriWareApi::criAtomExPlayer_SetVoicePoolIdentifier(MixerPlayer->ExPlayer, (CriAtomExVoicePoolIdentifier)VoicePoolID);
						}
						else
						{
							// default - should not come here
							FCriWareApi::criAtomExPlayer_SetVoicePoolIdentifier(MixerPlayer->ExPlayer, AtomRuntime->GetAtomDefaultVoicePoolIdentifier());
						}

						auto NativeHandle = WaveBankResource->GetNativeHandle();

						// Set the wave
						FCriWareApi::criAtomExPlayer_SetWaveId(MixerPlayer->ExPlayer, NativeHandle, SoundID);
						FCriWareApi::criAtomExPlayer_SetNumChannels(MixerPlayer->ExPlayer, WaveInfo.NumChannels);
						FCriWareApi::criAtomExPlayer_SetSamplingRate(MixerPlayer->ExPlayer, WaveInfo.SampleRate);

						//FCriWareApi::criAtomExPlayer_SetFile(Player, nullptr, SoundWave->);
					}
					else
					{
						UE_LOG(LogCriWareAtomMixer, Warning, TEXT("The mixer source`s player is in an invalid state."));
					}
				}
				else
				{
					// error
					return false;
				}
			}
		}

		// set data cuesheet+id
		else if (UAtomCueSheet* CueSheet = Cast<UAtomCueSheet>(SoundBank))
		{
			bIsLoaded = CueSheet->IsLoaded();

			if (!bIsLoaded)
			{
				if (!bIsPreparingForInit)
				{
					// async load
					CueSheet->RetainResource();
					bIsLoadingResource = true;
				}
			}
			else
			{
				bIsLoadingResource = false;

				if (auto Resource = CueSheet->AtomResource.GetResource())
				{
					auto CueSheetResource = Resource->GetCueSheetResource();

					// set the voice pool to use					
					InitSourceVoice(InPlaybackInstance);

					if (MixerPlayer->ExPlayer.IsValid())
					{
						if (MixerSourceVoice)
						{
							const int32 VoicePoolID = MixerSourceVoice->GetVoicePoolID();
							FCriWareApi::criAtomExPlayer_SetVoicePoolIdentifier(MixerPlayer->ExPlayer, (CriAtomExVoicePoolIdentifier)VoicePoolID);
						}
						else
						{
							// default - should not come here
							FCriWareApi::criAtomExPlayer_SetVoicePoolIdentifier(MixerPlayer->ExPlayer, AtomRuntime->GetAtomDefaultVoicePoolIdentifier());
						}

						auto NativeHandle = CueSheetResource->GetNativeHandle();

						// Set the cue
						FCriWareApi::criAtomExPlayer_SetCueId(MixerPlayer->ExPlayer, NativeHandle, SoundID);
					}
					else
					{
						UE_LOG(LogCriWareAtomMixer, Warning, TEXT("The mixer source`s player is in an invalid state."));
					}
				}
				else
				{
					// error
					return false;
				}
			}
		}

		return true;
	}

	bool FAtomExPlayback::PrepareSoundInternal()
	{
		// select output if not default or native (uses master rack by default)

		/*
		TArray<FAtomRackSend> Racks;
		PlaybackInstance->ActiveSound->GetAtomRackSends(Racks);
		if (Racks.Num() > 0)
		{
			TArray<CriSint32> ExRackIDs;
			for (UAtomRackBase* AtomRack : Racks)
			{
				if (GCriWare && AtomRack)
				{
					// ExPlayer only accept asr rack, no endpoint.
					if (AtomRack->IsA(UAtomRack::StaticClass()))
					{
						ExRackIDs.Add((CriSint32)AtomRuntime->GetAsrRackId(AtomRack));
					}
				}
			}

			// Set rack id array
			if (ExRackIDs.Num() > 0)
			{
				FCriWareApi::criAtomExPlayer_SetAsrRackIdArray(ExPlayer, ExRackIDs.GetData(), ExRackIDs.Num());
			}
		}*/

		// Actually we only support one rack for the bus send settings so only one rack destination
		
		if (!IsUsingObjectBasedSpatialization())
		{
			UAtomRackWithParentBase* Rack = nullptr;
 			if (PlaybackInstance->BusSendSettings.Num() > 0) // get from 1st bus send
			{
				Rack = Cast<UAtomRackWithParentBase>(PlaybackInstance->BusSendSettings[0].Bus->GetRack());
			}
			if (!Rack && PlaybackInstance->bEnableSoundRack) // else use the sound rack
			{
				Rack = Cast<UAtomRackWithParentBase>(PlaybackInstance->SoundRack);
			}
			if (Rack)
			{
				// ExPlayer only accept ASR rack (or master rack), no endpoints. 
				if (Rack->IsA(UAtomRackWithParentBase::StaticClass()))
				{
					CriSint32 ExRackID = (CriSint32)AtomRuntime->GetAsrRackId(Rack);
					if (ExRackID != INDEX_NONE)
					{
						FCriWareApi::criAtomExPlayer_SetAsrRackId(MixerPlayer->ExPlayer, ExRackID);
					}
				}
			}
		}
		else // Object Based
		{
			// Warn about sending a source marked as Binaural directly to a soundfield rack:
			// This is a bit of a gray area as soundfield racks are intended to be their own spatial format
			// So to send a source to this, and also flagging the source as Binaural are probably conflicting forms of spatialization.
			UAtomRackBase* Rack = PlaybackInstance->bEnableSoundRack ? Cast<UAtomRackBase>(PlaybackInstance->SoundRack) : nullptr;

			if (Rack)
			{
				//if (Rack->IsA(UAtomSoundfieldRack::StaticClass()) || Rack->IsA(UAtomSoundfieldEndpointRack::StaticClass()))
				//{
				//	UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Ignoring soundfield Base Rack destination being set on SoundWave (%s) because spatializaition method is set to Binaural.")
				//		, *PlaybackInstance->GetName());
				//}
				
				// multi rack no yet available - alway sow a warning
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Ignoring Base Rack destination being set on Sound (%s) because spatialization method is set to 'Object Based / Binaural'.")
					, *PlaybackInstance->GetName());
			}

			// get the spatialization rack and use it as destination
			UAtomRackBase* SpatializationRack = AtomRuntime->GetSpatializationRack();
			CriSint32 ExRackID = (CriSint32)AtomRuntime->GetAsrRackId(SpatializationRack);
			if (ExRackID != INDEX_NONE)
			{
				FCriWareApi::criAtomExPlayer_SetAsrRackId(MixerPlayer->ExPlayer, ExRackID);
			}

			if (AtomRuntime->IsAtomMixerPluginEnabled())
			{
				auto SpatializationBusIndex = AtomRuntime->GetSpatializationBusIndex();
				if (SpatializationBusIndex < 0)
				{
					if (UAtomRack* AtomRack = Cast<UAtomRack>(SpatializationRack))
					{
						if (AtomRack->Buses.IsValidIndex(SpatializationBusIndex))
						{
							// add bus send
							FCriWareApi::criAtomExPlayer_SetBusSendLevel(MixerPlayer->ExPlayer, SpatializationBusIndex, 1.0f);
						}
					}
				}
			}
		}

		// voices
		if (PlaybackInstance->SoundData->bOverrideVirtualizationMode)
		{
			// virtualization / silent mode (TODO: can be changed while playing with updateall())
			CriAtomExSilentMode SilentMode = CRIATOMEX_SILENT_MODE_NORMAL;
			switch (PlaybackInstance->SoundData->GetVirtualizationMode())
			{
			case EAtomVirtualizationMode::Normal:
				// no virtualization, continue to use a voice and play in silent.
				SilentMode = CRIATOMEX_SILENT_MODE_NORMAL;
				break;
			case EAtomVirtualizationMode::StopWhenSilent:
				// no virtualization and don't play in silent.
				SilentMode = CRIATOMEX_SILENT_MODE_STOP;
				break;
			case EAtomVirtualizationMode::PlayWhenSilent:
				// virtualize in silent (use a virtual voice), continue to play when silent. 
				SilentMode = CRIATOMEX_SILENT_MODE_VIRTUAL;
				break;
			case EAtomVirtualizationMode::Retrigger:
				// virtualize in silent (use a virtual voice), retrigger when back from virtual.
				SilentMode = CRIATOMEX_SILENT_MODE_VIRTUAL_RETRIGGER;
				break;
				/*case EAtomVirtualizationMode::Restart:
					// no virtalization, restrat from unreal.
					SilentMode = CRIATOMEX_SILENT_MODE_STOP;
					break;*/
			}

			FCriWareApi::criAtomExPlayer_SetSilentMode(MixerPlayer->ExPlayer, SilentMode);
		}

		// Priorties
		// WARNING -> Lost of information 32 bit to 9 bits only -> the layer that ADD to data based value of priority in ATOM with this function is limited to 9bits values (512 values only!)
		// priority with Atom Limiters can go wrong depending the Blueprint script logic that user made, in this case may need to control priority by AISAC.
		const float Priority = FMath::GetMappedRangeValueUnclamped(FVector2f{ -100.0f, 100.0f }, FVector2f{ -255.0f, 255.0f }, PlaybackInstance->Priority);
		FCriWareApi::criAtomExPlayer_SetVoicePriority(MixerPlayer->ExPlayer, (CriSint32)Priority); // control limiters and virtualization BY atom (can be culled by concurrency sytem before)

		// Categories
		SetCategories();

		// AISAC patches
		AttachAisacPatches();

		// Cue Params
		SetCueSelectorLabels();
		SetCueFirstBlockIndex();

		// Setup max pitch value to prepare internal buffer to accept fast pitch change at startup.
		const CriFloat32 Cents = Atom::GetSemitones(AtomRuntime->GetGlobalPitchRange().GetUpperBoundValue()) * 100;
		FCriWareApi::criAtomExPlayer_SetMaxPitch(MixerPlayer->ExPlayer, Cents);

		// Set startup values
		LPFFrequency = ATOM_MAX_FILTER_FREQUENCY;
		LastLPFFrequency = FLT_MAX;

		HPFFrequency = 0.0f;
		LastHPFFrequency = FLT_MAX;

		// Looping
		SetLooping();

		// Start Time
		CriSint64 StartTimeMs = 0;
		if (PlaybackInstance->StartTime > 0.0f)
		{
			if (PlaybackInstance->IsSeekable())
			{
				StartTimeMs = (CriSint64)(PlaybackInstance->StartTime * 1000.0);
			}
		}
		FCriWareApi::criAtomExPlayer_SetStartTime(MixerPlayer->ExPlayer, StartTimeMs);

		if (!bIsExternal)
		{
			PlaybackID = MixerSourceVoice ? MixerSourceVoice->InitMixerPlayer() : CRIATOMEX_INVALID_PLAYBACK_ID;

			/// ---

			if (PlaybackID == CRIATOMEX_INVALID_PLAYBACK_ID)
			{
				UE_LOG(LogCriWareAtomMixer, Error, TEXT("Cannot prepare AtomExPlayer for AtomSoundBase '%s'."), *PlaybackInstance->SoundData->GetName());
				return false;
			}

			// attach this as listener for events
			AtomListener = MakeShared<FAtomExPlaybackListener>(PlaybackID, PlaybackInstance, this);
			MixerPlayer->RegisterPlaybackListener(AtomListener.ToSharedRef(), PlaybackID);
		}
		else
		{
			// try to get fisrt playback ID if started else will be -1 (CRIATOMEX_INVALID_PLAYBACK_ID)
			// in case it is not prepared, we will try to get it when a voice is allocated in atom.
			PlaybackID = FCriWareApi::criAtomExPlayer_GetLastPlaybackId(MixerPlayer->ExPlayer); // extern

			if (PlaybackID != CRIATOMEX_INVALID_PLAYBACK_ID)
			{
				// attach this as listener for events
				AtomListener = MakeShared<FAtomExPlaybackListener>(PlaybackID, PlaybackInstance, this);
				MixerPlayer->RegisterPlaybackListener(AtomListener.ToSharedRef(), PlaybackID);
			}

			// for test - (multiple playbackids are not supported for external explayers attachement.)
			/*struct FCaptureParams
			{
				FAtomActiveSound* ActiveSound;
				const UPTRINT ParentHash;
				const UPTRINT NodeHash;
			};

			FCaptureParams Params = FCaptureParams{
				PlaybackInstance->ActiveSound,
				PlaybackInstance->PlaybackInstanceHash,
				reinterpret_cast<const UPTRINT>(MixerPlayer->ExPlayer.Get())
			};

			FCriWareApi::criAtomExPlayer_EnumeratePlaybacks(MixerPlayer->ExPlayer, [](void* Capture, CriAtomExPlaybackId PlaybackID) -> CriBool
			{
				FCaptureParams& Params = *static_cast<FCaptureParams*>(Capture);

				const UPTRINT ChildHash = static_cast<UPTRINT>(PlaybackID);

				if (!Params.ActiveSound->FindPlaybackInstance(ChildHash))
				{
					FAtomPlaybackInstance& NewPlaybackInstance = Params.ActiveSound->AddPlaybackInstance(ChildHash);
				}

				return CRI_TRUE;
			}, &Params);*/
		}

		return true;
	}

	bool FAtomExPlayback::IsPreparedToInit()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);
		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomExPlayback::IsPreparedToInit);
#ifdef ATOM_SOUND_INIT_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("IsPreparedToInit '%p', %i."), PlaybackInstance, MixerPlayer->ExPlayer.IsValid());
#endif
		FScopeLock Lock(&MixerPlayer->PlayerLock);

		if (bIsPreparingForInit && MixerPlayer->ExPlayer.IsValid())
		{
			// check if resource is ready
			if (PlaybackInstance)
			{
				bool bIsLoaded = true;
				if (bIsLoadingResource)
				{
					// try access ressource
					bIsLoaded = false;
					if (LoadSoundInternal(PlaybackInstance))
					{
						if (bIsLoadingResource)
						{
							// not yet loaded - wait
							return false;
						}

						bIsLoaded = true;
					}
					else
					{
						// loading failed
						bIsPrepareFailed = true;
						return true;
					}
				}

				if (bIsLoaded && !bIsPreparingPlayer)
				{
					// loaded - setup and call prepare of native atom player
					if (!PrepareSoundInternal())
					{
						// prepare failed
						bIsPreparingPlayer = false;
						bIsPrepareFailed = true;
						return true;
					}

					// atom is preparing the playback
					bIsPreparingPlayer = true;
				}
			}

			if (bIsPreparingPlayer && PlaybackID == CRIATOMEX_INVALID_PLAYBACK_ID)
			{
				if (bIsExternal)
				{
					// wait until 1st PlaybackID is ready.
					/// SHOULD MOVE THIS to a function in explayer or event....
					PlaybackID = FCriWareApi::criAtomExPlayer_GetLastPlaybackId(MixerPlayer->ExPlayer); // extern
					// ----
					if (PlaybackID != CRIATOMEX_INVALID_PLAYBACK_ID)
					{
						// attach this as listener for events
						AtomListener = MakeShared<FAtomExPlaybackListener>(PlaybackID, PlaybackInstance, this);
						MixerPlayer->RegisterPlaybackListener(AtomListener.ToSharedRef(), PlaybackID);
					}
				}
				else
				{
					// check for error at prepare time
					bIsPreparingPlayer = false;
					bIsPrepareFailed = true;
					return true;
				}
			}

			if (PlaybackID != CRIATOMEX_INVALID_PLAYBACK_ID)
			{
				// check the prepare status of native atom player after call to prepare
				Status = FCriWareApi::criAtomExPlayback_GetStatus(PlaybackID);
				if (Status == CRIATOMEXPLAYBACK_STATUS_PLAYING
					|| Status == CRIATOMEXPLAYBACK_STATUS_REMOVED)
				{
					bIsPreparingPlayer = false;
					return true;
				}
			}
		}

		return false;
	}

	bool FAtomExPlayback::Init(FAtomPlaybackInstance* InPlaybackInstance)
	{
		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomExPlayback::Init);
		//AUDIO_MIXER_CHECK(MixerBuffer);
		//AUDIO_MIXER_CHECK(MixerBuffer->IsRealTimeSourceReady());
#ifdef ATOM_SOUND_INIT_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Init. %d"), Status);
#endif
		// We've already been passed the wave instance in PrepareForInitialization, make sure we have the same one
		ATOM_MIXER_CHECK(PlaybackInstance && PlaybackInstance == InPlaybackInstance);

		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		FAtomSource::InitCommon();

		if (!ensure(InPlaybackInstance))
		{
			return false;
		}

		// Reset all 'previous' state.
		bPreviousBaseRackEnablement = false;
		PreviousAzimuth = -1.f;
		PreviousPlaybackPercent = 0.f;
		PreviousBusSendSettings.Reset();
		PreviousCueSelectorParams.Reset();
		PreviousCueNextBlockIndex = INDEX_NONE;

		bPreviousSourceBusEnablement = PlaybackInstance->bEnableSourceBusSends;
		DynamicSourceBusSendInfos.Reset();

		MixerPlayer->ResetSpatialization(*PlaybackInstance);

		bIsPreparingForInit = false;

		{
			FScopeLock Lock(&MixerPlayer->PlayerLock);

			if (!bIsPrepareFailed && MixerPlayer->ExPlayer.IsValid())
			{
				if (Status == CRIATOMEXPLAYBACK_STATUS_PLAYING)
				{
					// init other stuff and reset dynamic params

					FAtomRuntimeId RuntimeID = AtomRuntime->GetAtomRuntimeID();

					check(MixerSourceVoice != nullptr);
					uint32 SourceID = MixerSourceVoice->GetSourceID();

					// Initialize the propagation interface as soon as we have a valid source id
					if (AtomRuntime->SourceDataOverridePluginInterface)
					{
						AtomRuntime->SourceDataOverridePluginInterface->OnInitSource(SourceID, PlaybackInstance->ActiveSound->GetAtomComponentUserID(), PlaybackInstance->SourceDataOverridePluginSettings);
					}

					for (auto RuntimePluginInterface : AtomRuntime->RuntimePluginInterfaces)
					{
						RuntimePluginInterface->OnInitSource(SourceID, this);
					}

					bIsInitialized = true;
					return true;
				}
				else if (Status == CRIATOMEXPLAYBACK_STATUS_REMOVED)
				{
					bIsInitialized = true;
					PlaybackInstance->NotifyFinished();
					return true;
				}
			}
		}

		bIsPrepareFailed = false;
		FreeResources(EFreeReason::Error);
		return false;
	}

	void FAtomExPlayback::SetupSourceBusData(TArray<FInitAudioBusSend>* OutAudioBusSends, bool bEnableBusSends)
	{
		for (int32 BusSendStage = 0; BusSendStage < (int32)EAtomBusSendStage::Count; ++BusSendStage)
		{
			// And add all the source bus sends
			for (FAtomSoundSourceBusSendInfo& SendInfo : PlaybackInstance->SourceBusSends[BusSendStage])
			{
				// Avoid redoing duplicate code for sending audio to source bus or audio bus. Most of it is the same other than the bus id.
				auto SetupBusSend = [this](TArray<FInitAudioBusSend>* AudioBusSends, const FAtomSoundSourceBusSendInfo& InSendInfo, int32 InBusSendStage, uint32 InBusID, bool bEnableBusSends, int32 InBusChannels, const FString& InBusName)
					{
						FInitAudioBusSend BusSend;
						BusSend.AudioBusID = InBusID;
						BusSend.BusChannels = InBusChannels;
#if ENABLE_ATOM_DEBUG
						BusSend.AudioBusName = InBusName;
#endif // ENABLE_ATOM_DEBUG

						if (bEnableBusSends)
						{
							BusSend.SendLevel = InSendInfo.SendLevel;
						}
						else
						{
							BusSend.SendLevel = 0;
						}

						if (AudioBusSends)
						{
							AudioBusSends[InBusSendStage].Add(BusSend);
						}

						FDynamicSourceBusSendInfo NewDynamicBusSendInfo;
						NewDynamicBusSendInfo.SendLevel = InSendInfo.SendLevel;
						NewDynamicBusSendInfo.BusID = BusSend.AudioBusID;
#if ENABLE_ATOM_DEBUG
						NewDynamicBusSendInfo.BusName = BusSend.AudioBusName;
#endif // ENABLE_ATOM_DEBUG
						NewDynamicBusSendInfo.BusSendLevelControlMethod = InSendInfo.SourceBusSendLevelControlMethod;
						NewDynamicBusSendInfo.BusSendStage = (EAtomBusSendStage)InBusSendStage;
						NewDynamicBusSendInfo.MinSendLevel = InSendInfo.MinSendLevel;
						NewDynamicBusSendInfo.MaxSendLevel = InSendInfo.MaxSendLevel;
						NewDynamicBusSendInfo.MinSendDistance = InSendInfo.MinSendDistance;
						NewDynamicBusSendInfo.MaxSendDistance = InSendInfo.MaxSendDistance;
						NewDynamicBusSendInfo.CustomSendLevelCurve = InSendInfo.CustomSendLevelCurve;

						// Copy the bus SourceBusSendInfo structs to a local copy so we can update it in the update tick
						bool bIsNew = true;
						for (FDynamicSourceBusSendInfo& BusSendInfo : DynamicSourceBusSendInfos)
						{
							if (BusSendInfo.BusID == NewDynamicBusSendInfo.BusID)
							{
								BusSendInfo = NewDynamicBusSendInfo;
								BusSendInfo.bIsInit = false;
								bIsNew = false;
								break;
							}
						}

						if (bIsNew)
						{
							DynamicSourceBusSendInfos.Add(NewDynamicBusSendInfo);
						}

						// Flag that we're sending audio to buses so we can check for updates to send levels
						bSendingAudioToSourceBuses = true;
					};

				// Retrieve bus id of the audio bus to use
				if (SendInfo.SoundSourceBus)
				{
					uint32 BusID;
					int32 BusChannels;
					FString BusName;

					// Either use the bus id of the source bus's audio bus id if it was specified
					if (SendInfo.SoundSourceBus->AudioBus)
					{
						BusID = SendInfo.SoundSourceBus->AudioBus->GetUniqueID();
						BusChannels = (int32)SendInfo.SoundSourceBus->AudioBus->GetNumChannels();
#if ENABLE_ATOM_DEBUG
						BusName = SendInfo.SoundSourceBus->AudioBus.GetPathName();
#endif // ENABLE_ATOM_DEBUG
					}
					else
					{
						// otherwise, use the id of the source bus itself (for an automatic source bus)
						BusID = SendInfo.SoundSourceBus->GetUniqueID();
						BusChannels = SendInfo.SoundSourceBus->WaveInfo.NumChannels;
#if ENABLE_ATOM_DEBUG
						BusName = SendInfo.SoundSourceBus->GetPathName();
#endif // ENABLE_ATOM_DEBUG
					}

					// Call lambda w/ the correctly derived bus id
					SetupBusSend(OutAudioBusSends, SendInfo, BusSendStage, BusID, bEnableBusSends, BusChannels, BusName);
				}

				if (SendInfo.AudioBus)
				{
					// Only need to send audio to just the specified audio bus
					uint32 BusID = SendInfo.AudioBus->GetUniqueID();
					int32 BusChannels = (int32)SendInfo.AudioBus->AudioBusChannels + 1;
					FString BusName;

#if ENABLE_ATOM_DEBUG
					BusName = SendInfo.AudioBus->GetPathName();
#endif // ENABLE_ATOM_DEBUG

					// Note we will be sending audio to both the specified source bus and the audio bus with the same send level
					SetupBusSend(OutAudioBusSends, SendInfo, BusSendStage, BusID, bEnableBusSends, BusChannels, BusName);
				}
			}
		}
	}

	void FAtomExPlayback::Update()
	{
		CSV_SCOPED_TIMING_STAT(Atom, UpdateSources);
		SCOPE_CYCLE_COUNTER(STAT_AtomUpdateSources);

		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		MixerPlayer->PlayerLock.Lock();
		
		if (!PlaybackInstance || !MixerSourceVoice || !MixerPlayer->ExPlayer.IsValid() || bIsPaused || (!bIsInitialized && !bIsPreparingForInit)) // || InitializationState == EMixerSourceInitializationState::NotInitialized)
		{
			MixerPlayer->PlayerLock.Unlock();
			return;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomExPlayback::Update);

		// if MarkAsGarbage() was called, PlaybackInstance->SoundData is null
		if (!PlaybackInstance->SoundData)
		{
			MixerPlayer->PlayerLock.Unlock();
			StopNow();
			return;
		}

		//Status = FCriWareApi::criAtomExPlayer_GetStatus(ExPlayer);
		Status = FCriWareApi::criAtomExPlayback_GetStatus(PlaybackID);
#ifdef ATOM_SOUND_INIT_DEBUG
		//UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Update. %d"), Status);
#endif
		FAtomSource::UpdateCommon();

		TickCount++;

		// Allow plugins to override any data in a PlaybackInstance
		if (AtomRuntime->SourceDataOverridePluginInterface && PlaybackInstance->bEnableSourceDataOverride)
		{
			uint32 SourceID = MixerSourceVoice->GetSourceID();
			int32 ListenerIndex = PlaybackInstance->ActiveSound->GetClosestListenerIndex();

			FTransform ListenerTransform;
			AtomRuntime->GetListenerTransform(ListenerIndex, ListenerTransform);

			AtomRuntime->SourceDataOverridePluginInterface->GetSourceDataOverrides(SourceID, ListenerTransform, PlaybackInstance);
		}

		UpdateModulation();

		UpdatePitch();

		UpdateVolume();

		UpdateSpatialization();

		UpdateEffects();

		UpdateBusSends();

		UpdateSourceBusSends();

		UpdateChannelMap();

		UpdateCueParameters();

		FCriWareApi::criAtomExPlayer_Update(MixerPlayer->ExPlayer, PlaybackID);

#if ENABLE_ATOM_DEBUG
		Atom::FAtomDebugger::DrawDebugInfo(*this);
#endif // ENABLE_ATOM_DEBUG

		MixerPlayer->PlayerLock.Unlock();
	}

	void FAtomExPlayback::UpdatePitch()
	{
		check(PlaybackInstance);

		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);

		Pitch = PlaybackInstance->GetPitch();
		//UE_LOG(LogCriWareAtomMixer, Display, TEXT("Update pitch. %f"), Pitch);

		// Don't apply global pitch scale to UI sounds
		if (!PlaybackInstance->bIsUISound)
		{
			Pitch *= AtomRuntime->GetGlobalPitchScale().GetValue();
		}

		// Modulations
		UAtomSoundBase* Sound = PlaybackInstance->SoundData;
		check(Sound);
		// realtime change base value
		const float ModPitchBase = AtomModulationUtils::GetRoutedPitch(*PlaybackInstance, *Sound, *ActiveSound);
		
		if (MixerSourceVoice)
		{
			MixerSourceVoice->SetModPitch(ModPitchBase);
			const float ModPitchEnd = Atom::GetFrequencyMultiplier(MixerSourceVoice->GetModPitchEnd());
			Pitch *= ModPitchEnd;
		}

		Pitch = AtomRuntime->ClampPitch(Pitch);

		// Semitones = 12.0f * FMath::Log2(PitchRatio)
		// 100 Cents = 1 Semitone
		const CriFloat32 Cents = Atom::GetSemitones(Pitch) * 100.0f;
		FCriWareApi::criAtomExPlayer_SetPitch(MixerPlayer->ExPlayer, Cents);

#if ATOM_PROFILERTRACE_ENABLED
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
		if (bChannelEnabled)
		{
			UE_TRACE_LOG(CriWareAtom, MixerSourcePitch, AtomMixerChannel)
				<< MixerSourcePitch.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< MixerSourcePitch.Timestamp(FPlatformTime::Cycles64())
				<< MixerSourcePitch.PlayOrder(PlaybackInstance->GetPlayOrder()) // in source
				<< MixerSourcePitch.ActiveSoundPlayOrder(ActiveSound->GetPlayOrder()) // in source
				<< MixerSourcePitch.Pitch(Pitch);
		}
#endif
	}

	void FAtomExPlayback::UpdateVolume()
	{
		check(PlaybackInstance);

		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);

		float CurrentVolume = 0.0f;
		if (!AtomRuntime->IsAtomRuntimeMuted())
		{
			// Apply virtualization volume to ADX -> silent
			// todo: move virtulaization stuff to PlaybackInstance
			if (ActiveSound->bVirtualizedDueToMaxConcurrency
				 && ActiveSound->FadeOut != FAtomActiveSound::EFadeOut::Concurrency)
			{
				CurrentVolume = 0.0f;
			}
			else
			{
				// 1. Apply runtime gain stage(s)
				CurrentVolume = ActiveSound->bIsPreviewSound ? 1.0f : AtomRuntime->GetPrimaryVolume();
				//CurrentVolume *= AudioDevice->GetPlatformAudioHeadroom();

				// 2. Apply instance gain stages
				CurrentVolume *= PlaybackInstance->GetVolume();
				CurrentVolume *= PlaybackInstance->GetDynamicVolume();

				// 3. Apply editor gain stage
				CurrentVolume = FMath::Clamp<float>(GetDebugVolume(CurrentVolume), 0.0f, ATOM_MAX_VOLUME);

				// 4. Apply Modulations
				UAtomSoundBase* Sound = PlaybackInstance->SoundData;
				check(Sound);

				if (MixerSourceVoice)
				{
					// realtime change base value
					const float ModVolumeBase = AtomModulationUtils::GetRoutedVolume(*PlaybackInstance, *Sound, *ActiveSound);
					MixerSourceVoice->SetModVolume(ModVolumeBase);

					// Note: can execute this in real time with cri atom server callback
					const float ModVolumeEnd = MixerSourceVoice->GetModVolumeEnd();

					CurrentVolume *= ModVolumeEnd;
				}

				// 5. Apply occlusion attenuation that is not handled by Atom
				CurrentVolume *= PlaybackInstance->GetOcclusionAttenuation();
			}
		}

#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Error, TEXT("volume %f"), CurrentVolume);
#endif

		FCriWareApi::criAtomExPlayer_SetVolume(MixerPlayer->ExPlayer, CurrentVolume);

#if ATOM_PROFILERTRACE_ENABLED
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
		if (bChannelEnabled)
		{
			UE_TRACE_LOG(CriWareAtom, MixerSourceVolume, AtomMixerChannel)
				<< MixerSourceVolume.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< MixerSourceVolume.Timestamp(FPlatformTime::Cycles64())
				<< MixerSourceVolume.PlayOrder(PlaybackInstance->GetPlayOrder())
				<< MixerSourceVolume.ActiveSoundPlayOrder(ActiveSound->GetPlayOrder()) // InSource
				<< MixerSourceVolume.Volume(CurrentVolume); // need to get the base volume too !

			UE_TRACE_LOG(CriWareAtom, MixerSourceDistanceAttenuation, AtomMixerChannel)
				<< MixerSourceDistanceAttenuation.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< MixerSourceDistanceAttenuation.Timestamp(FPlatformTime::Cycles64())
				<< MixerSourceDistanceAttenuation.PlayOrder(PlaybackInstance->GetPlayOrder())
				<< MixerSourceDistanceAttenuation.DistanceAttenuation(0.0f); // how get this ?
		}
#endif

		// amplitude envelope
		if (PlaybackInstance->bEnableAmplitudeEnvelope)
		{
			auto& Envelope = PlaybackInstance->AmplitudeEnvelope;
			FCriWareApi::criAtomExPlayer_SetEnvelopeAttackTime(MixerPlayer->ExPlayer, Envelope.AttackTime);
			FCriWareApi::criAtomExPlayer_SetEnvelopeHoldTime(MixerPlayer->ExPlayer, Envelope.HoldTime);
			FCriWareApi::criAtomExPlayer_SetEnvelopeDecayTime(MixerPlayer->ExPlayer, Envelope.DecayTime);
			FCriWareApi::criAtomExPlayer_SetEnvelopeSustainLevel(MixerPlayer->ExPlayer, Envelope.SustainLevel);
			FCriWareApi::criAtomExPlayer_SetEnvelopeReleaseTime(MixerPlayer->ExPlayer, Envelope.ReleaseTime);
			FCriWareApi::criAtomExPlayer_SetEnvelopeAttackCurve(MixerPlayer->ExPlayer, (CriAtomExCurveType)Envelope.AttackCurve, Envelope.AttackCurveStrength);
			FCriWareApi::criAtomExPlayer_SetEnvelopeDecayCurve(MixerPlayer->ExPlayer, (CriAtomExCurveType)Envelope.DecayCurve, Envelope.DecayCurveStrength);
			FCriWareApi::criAtomExPlayer_SetEnvelopeReleaseCurve(MixerPlayer->ExPlayer, (CriAtomExCurveType)Envelope.ReleaseCurve, Envelope.ReleaseCurveStrength);
		}
	}

	// debug
#if UE_BUILD_DEBUG
	extern "C" CriBool CRIAPI criAtomExPlayback_DebugGetDistanceFactor(CriAtomExPlaybackId id, CriFloat32 * distance_factor);
#endif

	void FAtomExPlayback::UpdateSpatialization()
	{
		check(PlaybackInstance);

		bIs3D = MixerPlayer->UpdateSpatialization(*PlaybackInstance);

		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);

		if (bIs3D)
		{
			// debug test - try to get the applyied volume attenuation
			if (bIsPlaying && bAttenuationReady)
			{
				CriFloat32 OutDistanceFactor = 0.0f;
#if UE_BUILD_DEBUG
				criAtomExPlayback_DebugGetDistanceFactor(PlaybackID, &OutDistanceFactor);
#endif
				PlaybackInstance->SetDistanceAttenuation(1.0f - OutDistanceFactor);
#ifdef ATOM_SOUND_DEV_DEBUG
				UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("GetDistanceFactor for [%d]: factor %f"), PlaybackID, OutDistanceFactor);
#endif
			}
			else
			{
				bAttenuationReady = true;
			}
		}
	}

	void FAtomExPlayback::UpdateEffects()
	{
		// Update the default LPF/HPF filter frequency
		SetFilterFrequency();

		// Modulations
		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);
		UAtomSoundBase* Sound = PlaybackInstance->SoundData;
		check(Sound);

		if (MixerSourceVoice)
		{
			// realtime change base value
			float ModHighpassBase = AtomModulationUtils::GetRoutedHighpass(*PlaybackInstance, *Sound, *ActiveSound);
			MixerSourceVoice->SetModHPFFrequency(ModHighpassBase);

			float ModLowpassBase = AtomModulationUtils::GetRoutedLowpass(*PlaybackInstance, *Sound, *ActiveSound);
			MixerSourceVoice->SetModLPFFrequency(ModLowpassBase);

			const float ModHighpassEnd = MixerSourceVoice->GetModHPFEnd();
			const float ModLowpassEnd = MixerSourceVoice->GetModLPFEnd();
		
			// mix with main carrier
			HPFFrequency = FMath::Max(ModHighpassEnd, HPFFrequency);
			LPFFrequency = FMath::Min(ModLowpassEnd, LPFFrequency);
		}

		// apply to atom 
		// TODO: better biquad/bandpass filter control to apply only one 
		if (LastLPFFrequency != LPFFrequency || LastHPFFrequency != HPFFrequency)
		{
			//UE_LOG(LogTemp, Warning, TEXT("LPF%f HPF%f"), SpatializationParams.AttenuationLowpassFilterFrequency, SpatializationParams.AttenuationHighpassFilterFrequency);
			CriFloat32 CutoffLow = Atom::GetLinearFrequencyClamped(HPFFrequency, FVector2D(0.0f, 1.0f), FVector2D(ATOM_MIN_FILTER_FREQUENCY, ATOM_MAX_FILTER_FREQUENCY));
			CriFloat32 CutoffHigh = Atom::GetLinearFrequencyClamped(LPFFrequency, FVector2D(0.0f, 1.0f), FVector2D(ATOM_MIN_FILTER_FREQUENCY, ATOM_MAX_FILTER_FREQUENCY));

			//UE_LOG(LogTemp, Warning, TEXT("LPF%f HPF%f"), CutoffLow, CutoffHigh);
			FCriWareApi::criAtomExPlayer_SetBandpassFilterParameters(MixerPlayer->ExPlayer, CutoffLow, CutoffHigh);

			LastLPFFrequency = LPFFrequency;
			LastHPFFrequency = HPFFrequency;
		}

#if ATOM_PROFILERTRACE_ENABLED
		const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
		if (bChannelEnabled)
		{
			UE_TRACE_LOG(CriWareAtom, MixerSourceFilters, AtomMixerChannel)
				<< MixerSourceFilters.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< MixerSourceFilters.Timestamp(FPlatformTime::Cycles64())
				<< MixerSourceFilters.PlayOrder(PlaybackInstance->GetPlayOrder()) // in source
				<< MixerSourceFilters.HPFFrequency(HPFFrequency)
				<< MixerSourceFilters.LPFFrequency(LPFFrequency);
			UE_TRACE_LOG(CriWareAtom, MixerSourceEnvelope, AtomMixerChannel)
				<< MixerSourceEnvelope.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< MixerSourceEnvelope.Timestamp(FPlatformTime::Cycles64())
				<< MixerSourceEnvelope.PlayOrder(PlaybackInstance->GetPlayOrder()) // in source
				<< MixerSourceEnvelope.ActiveSoundPlayOrder(ActiveSound->GetPlayOrder()) // InSource
				<< MixerSourceEnvelope.Envelope(GetEnvelopeValue());
		}
#endif // ATOM_PROFILERTRACE_ENABLED

		// WARNING -> Lost of information 32 bit to 9 bits only -> the layer that ADD to data based value of priority in ATOM with this function is limited to 9bits values (512 values only!)
		// priority with Atom Limiters can go wrong depending the Blueprint script logic that user made, in this case may need to control priority by AISAC.
		const float Priority = FMath::GetMappedRangeValueUnclamped(FVector2f{-100.0f, 100.0f}, FVector2f{-255.0f, 255.0f}, PlaybackInstance->Priority);
		FCriWareApi::criAtomExPlayer_SetVoicePriority(MixerPlayer->ExPlayer, (CriSint32)Priority); // control limiters and virtualization BY atom (can be culled by concurrency sytem before)
		
		//CriAtomExFaderConfig cfg;
		//FCriWareApi::criAtomExPlayer_AttachFader(ExPlayer, &cfg, nullptr, 0);

		// function to by pass attenuation
		//FCriWareApi::criAtomExPlayer_SetDrySendLevel();

		// bus sends from attenuation settings
		if (PlaybackInstance->BusSendSettings.Num() > 0)
		{
			bool bIsUsingMasterBusSend = false;
			auto DestRack = (PlaybackInstance->bEnableSoundRack && PlaybackInstance->SoundRack) ? PlaybackInstance->SoundRack : AtomRuntime->GetMasterRack();

			for (auto& SendSettings : PlaybackInstance->BusSendSettings)
			{
				if (SendSettings.Bus)
				{
					//if (SendSettings.Bus->GetRack() != DestRack)
					//{
					//	UE_LOG(LogCriWareAtomMixer, Warning, TEXT("A bus send from a different rack than the destination rack is set for sound %s."), *ActiveSound->GetSound()->GetName());
					//	continue;
					//}

					// Get bus name
					const FString& BusName = SendSettings.Bus->GetBusName();
#if WITH_EDITOR
					if (BusName.IsEmpty())
					{
						UE_LOG(LogCriWareAtomMixerDebug, Error, TEXT("Failed to set bus send value of bus '%s' because it is not initialized with AtomRack '%s'."), *SendSettings.Bus->GetFullName(), *DestRack->GetFullName());
						continue;
					}
#endif
					check(!BusName.IsEmpty());

					if (SendSettings.Bus->IsMainBus())
					{
						bIsUsingMasterBusSend = true;
					}

					// compute attenuation value

					float BusSendLevel = 0.0f;

					if (SendSettings.BusSendMethod == EAtomBusSendMethod::Manual)
					{
						BusSendLevel = FMath::Clamp(SendSettings.ManualBusSendLevel, 0.0f, 1.0f);
					}
					else
					{
						// The alpha value is determined identically between manual and custom curve methods
						const float Denom = FMath::Max(SendSettings.BusSendDistanceMax - SendSettings.BusSendDistanceMin, 1.0f);
						const float Alpha = FMath::Clamp((PlaybackInstance->ListenerToSoundDistance - SendSettings.BusSendDistanceMin) / Denom, 0.0f, 1.0f);

						if (SendSettings.BusSendMethod == EAtomBusSendMethod::Linear)
						{
							BusSendLevel = FMath::Clamp(FMath::Lerp(SendSettings.BusSendLevelMin, SendSettings.BusSendLevelMax, Alpha), 0.0f, 1.0f);
						}
						else
						{
							BusSendLevel = FMath::Clamp(SendSettings.CustomBusSendCurve.GetRichCurveConst()->Eval(Alpha), 0.0f, 1.0f);
						}
					}

					// actualy it modulate the seetings that is set in acb data.
					FCriWareApi::criAtomExPlayer_SetBusSendLevel(MixerPlayer->ExPlayer, SendSettings.Bus->GetBusIndex(), BusSendLevel);
					//FCriWareApi::criAtomExPlayer_SetBusSendLevelByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*BusName), BusSendLevel);
					//FCriWareApi::criAtomExPlayer_SetBusSendLevelOffsetByName(ExPlayer, TCHAR_TO_UTF8(*BusName), 0.0f);
				}
			}

			// remove default master bus send, if not set (Atom sets by default a send to master bus.)
			if (!bIsUsingMasterBusSend)
			{
				FCriWareApi::criAtomExPlayer_SetBusSendLevel(MixerPlayer->ExPlayer, 0, 0.0f);
				//FCriWareApi::criAtomExPlayer_SetBusSendLevelByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*UAtomRack::MainBusName), 0.0f);
			}
		}

		if (MixerSourceVoice)
		{

			// AISACs modulations
			MixerSourceVoice->IterateOverAisacModulations([this, Sound, ActiveSound](FAtomAisacControl& Control, FModulationDestination& Modulation, float ModulationBase)
			{
				auto FindPredicate = [&](const FAtomAisacParameter& ControlParam) { return Control == ControlParam.Control; };

				float AisacValue = 1.0f;
				if (auto* DefaultAisac = PlaybackInstance->AisacControlParams.FindByPredicate(FindPredicate))
				{
					AisacValue = DefaultAisac->Value;
				}

				// realtime change base value
				const float ModVolumeBase = AtomModulationUtils::GetRoutedAisac(Control, *PlaybackInstance, *Sound, *ActiveSound);
				ModulationBase = ModVolumeBase;

				// Note: can execute this in real time with cri atom server callback
				//const bool bHasProcessed = SourceInfo.VolumeModulation.GetHasProcessed();
				//const float ModVolumeStart = SourceInfo.VolumeModulation.GetValue();
				Modulation.ProcessControl(ModulationBase);
				const float ModValueEnd = Modulation.GetValue();
				//VolumeStart *= bHasProcessed ? ModVolumeStart : ModVolumeEnd;
				AisacValue *= ModValueEnd;

				const CriAtomExAisacControlId ID = Control.ID;
				CriFloat32 Value = AisacValue;
				FCriWareApi::criAtomExPlayer_SetAisacControlById(MixerPlayer->ExPlayer, ID, Value);

				return true;
			});

			// add aisac not in modulation
			for (auto& ControlMod : PlaybackInstance->AisacControlParams)
			{
				//if (!SourceInfo.AisacControlModulations.Contains(ControlMod.Control))
				if (MixerSourceVoice->IterateOverAisacModulations([Ctrl = ControlMod.Control](FAtomAisacControl& Control, FModulationDestination&, float) { return Ctrl != Control; }))
				{
					CriFloat32 Value = ControlMod.Value;
					if (ControlMod.Control.ID >= 0)
					{
						const CriAtomExAisacControlId ID = ControlMod.Control.ID;
						FCriWareApi::criAtomExPlayer_SetAisacControlById(MixerPlayer->ExPlayer, ID, Value);
					}
					else if (!ControlMod.Control.Name.IsNone())
					{
						FCriWareApi::criAtomExPlayer_SetAisacControlByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*ControlMod.Control.Name.ToString()), Value);
					}
				}
			}

			// source effects
		
			// update the voice effect
			const FAtomSourceVoiceEffect& VoiceEffect = PlaybackInstance->SourceVoiceEffect;
			MixerSourceVoice->SetVoiceEffect(VoiceEffect);
			for (int32 Index = 0; Index < VoiceEffect.Params.Num(); ++Index)
			{
				FCriWareApi::criAtomExPlayer_SetDspParameter(MixerPlayer->ExPlayer, Index, VoiceEffect.Params[Index]);
			}
		}

		MixerSourceVoice->SetEnablement(PlaybackInstance->bEnableSourceBusSends, PlaybackInstance->bEnableSoundRack, PlaybackInstance->bEnableSoundBusSends);
	}

	void FAtomExPlayback::UpdateModulation()
	{
		check(PlaybackInstance);

		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);

		if (ActiveSound->bModulationRoutingUpdated)
		{
			if (MixerSourceVoice)
			{
				MixerSourceVoice->SetModulationRouting(ActiveSound->ModulationRouting);
			}
		}

		ActiveSound->bModulationRoutingUpdated = false;
	}

	void FAtomExPlayback::UpdateBusSends()
	{
		bool bIsUsingMasterBusSend = false;
		auto DestRack = (PlaybackInstance->bEnableSoundRack && PlaybackInstance->SoundRack) ? PlaybackInstance->SoundRack : AtomRuntime->GetMasterRack();

		// Clear bus sends if they need clearing.
		if (PreviousBusSendSettings.Num() > 0)
		{
			// Loop through every previous send setting
			for (FAtomSoundToBusSend& PreviousSendSetting : PreviousBusSendSettings)
			{
				bool bFound = false;

				if (PlaybackInstance->bEnableSoundBusSends)
				{
					// See if it's in the current send list
					for (const FAtomSoundToBusSend& CurrentSendSettings : PlaybackInstance->SoundBusSends)
					{
						if (CurrentSendSettings.Bus == PreviousSendSetting.Bus)
						{
							bFound = true;
							break;
						}
					}
				}

				// If it's not in the current send list, add to submixes to clear
				if (!bFound && PreviousSendSetting.Bus)
				{
					//FCriWareApi::criAtomExPlayer_SetBusSendLevelByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*PreviousSendSetting.Bus->GetBusName().ToString()), 0.0f);

					// Atom resets all buses, but we will reapply them. 
					// it will not change things since Update() is not called between the ResetBuses() and SetBusLevel()
					FCriWareApi::criAtomExPlayer_ResetBusSends(MixerPlayer->ExPlayer);
					break;
				}
			}
		}
		
		if (PlaybackInstance->bEnableSoundBusSends)
		{
			PreviousBusSendSettings = PlaybackInstance->SoundBusSends;

			// Update bus send levels
			for (FAtomSoundToBusSend& SendInfo : PlaybackInstance->SoundBusSends)
			{
				if (SendInfo.Bus)
				{
					//if (SendInfo.Bus->GetRack() != DestRack)
					//{
					//	UE_LOG(LogCriWareAtomMixer, Warning, TEXT("A bus send from a different rack than the destination rack is set for sound %s."), *ActiveSound->GetSound()->GetName());
					//	continue;
					//}

					// Get bus name
					const FString& BusName = SendInfo.Bus->GetBusName();
#if WITH_EDITOR
					if (BusName.IsEmpty())
					{
						UE_LOG(LogCriWareAtomMixerDebug, Error, TEXT("Failed to set bus send value of bus '%s' because it is not initialized with AtomRack '%s'."), *SendInfo.Bus->GetFullName(), *DestRack->GetFullName());
						continue;
					}
#endif
					check(!BusName.IsEmpty());

					if (SendInfo.Bus->IsMainBus())
					{
						bIsUsingMasterBusSend = true;
					}

					float SendLevel = 1.0f;

					// calculate send level based on distance if that method is enabled
					/*if (!WaveInstance->bEnableSubmixSends)
					{
						SendLevel = 0.0f;
					}
					else*/ if (SendInfo.SendLevelControlMethod == EAtomSendLevelControlMethod::Manual)
					{
						if (SendInfo.DisableManualSendClamp)
						{
							SendLevel = SendInfo.SendLevel;
						}
						else
						{
							SendLevel = FMath::Clamp(SendInfo.SendLevel, 0.0f, 1.0f);
						}
					}
					else
					{
						// The alpha value is determined identically between manual and custom curve methods
						const float Denom = FMath::Max(SendInfo.MaxSendDistance - SendInfo.MinSendDistance, 1.0f);
						const float Alpha = FMath::Clamp((PlaybackInstance->ListenerToSoundDistance - SendInfo.MinSendDistance) / Denom, 0.0f, 1.0f);

						if (SendInfo.SendLevelControlMethod == EAtomSendLevelControlMethod::Linear)
						{
							SendLevel = FMath::Clamp(FMath::Lerp(SendInfo.MinSendLevel, SendInfo.MaxSendLevel, Alpha), 0.0f, 1.0f);
						}
						else // use curve
						{
							SendLevel = FMath::Clamp(SendInfo.CustomSendLevelCurve.GetRichCurveConst()->Eval(Alpha), 0.0f, 1.0f);
						}
					}

					// actualy it modulate the settings that is set in acb data.
					FCriWareApi::criAtomExPlayer_SetBusSendLevel(MixerPlayer->ExPlayer, SendInfo.Bus->GetBusIndex(), SendLevel);
					//FCriWareApi::criAtomExPlayer_SetBusSendLevelByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*BusName.ToString()), SendLevel);
					//FCriWareApi::criAtomExPlayer_SetBusSendLevelOffsetByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*BusName.ToString()), SendLevel);
				}
			}
		}
		else
		{
			PreviousBusSendSettings.Reset();
		}
	}

	void FAtomExPlayback::UpdateSourceBusSends()
	{
		// 1) loop through all source bus sends
		// 2) check for any source bus sends that are set to update non-manually
		// 3) Cache previous send level and only do update if it's changed in any significant amount

		SetupSourceBusData();

		FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound;
		check(ActiveSound);

		// Check if the user actively called a function that alters bus sends since the last update
		bool bHasNewBusSends = ActiveSound->HasNewSourceBusSends();

		if (!bSendingAudioToSourceBuses && !bHasNewBusSends && !DynamicSourceBusSendInfos.Num())
		{
			return;
		}

		if (bHasNewBusSends)
		{
			TArray<TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>> NewBusSends = ActiveSound->GetNewSourceBusSends();
			for (TTuple<EAtomBusSendStage, FAtomSoundSourceBusSendInfo>& NewSend : NewBusSends)
			{
				if (NewSend.Value.SoundSourceBus)
				{
					FString BusName;

#if ENABLE_ATOM_DEBUG
					BusName = NewSend.Value.SoundSourceBus->GetPathName();
#endif // ENABLE_ATOM_DEBUG

					MixerSourceVoice->SetAudioBusSendInfo(NewSend.Key, NewSend.Value.SoundSourceBus->GetUniqueID(), NewSend.Value.SendLevel, BusName);
					bSendingAudioToSourceBuses = true;
				}

				if (NewSend.Value.AudioBus)
				{
					FString BusName;

#if ENABLE_ATOM_DEBUG
					BusName = NewSend.Value.AudioBus->GetPathName();
#endif // ENABLE_ATOM_DEBUG

					MixerSourceVoice->SetAudioBusSendInfo(NewSend.Key, NewSend.Value.AudioBus->GetUniqueID(), NewSend.Value.SendLevel, BusName);
					bSendingAudioToSourceBuses = true;
				}
			}

			ActiveSound->ResetNewSourceBusSends();
		}

		// If this source is sending its audio to a bus, we need to check if it needs to be updated
		for (FDynamicSourceBusSendInfo& DynamicBusSendInfo : DynamicSourceBusSendInfos)
		{
			float SendLevel = 0.0f;

			if (DynamicBusSendInfo.BusSendLevelControlMethod == EAtomSourceBusSendLevelControlMethod::Manual)
			{
				SendLevel = FMath::Clamp(DynamicBusSendInfo.SendLevel, 0.0f, 1.0f);
			}
			else
			{
				// The alpha value is determined identically between linear and custom curve methods
				const FVector2D SendRadialRange = { DynamicBusSendInfo.MinSendDistance, DynamicBusSendInfo.MaxSendDistance };
				const FVector2D SendLevelRange = { DynamicBusSendInfo.MinSendLevel, DynamicBusSendInfo.MaxSendLevel };
				const float Denom = FMath::Max(SendRadialRange.Y - SendRadialRange.X, 1.0f);
				const float Alpha = FMath::Clamp((PlaybackInstance->ListenerToSoundDistance - SendRadialRange.X) / Denom, 0.0f, 1.0f);

				if (DynamicBusSendInfo.BusSendLevelControlMethod == EAtomSourceBusSendLevelControlMethod::Linear)
				{
					SendLevel = FMath::Clamp(FMath::Lerp(SendLevelRange.X, SendLevelRange.Y, Alpha), 0.0f, 1.0f);
				}
				else // use curve
				{
					SendLevel = FMath::Clamp(DynamicBusSendInfo.CustomSendLevelCurve.GetRichCurveConst()->Eval(Alpha), 0.0f, 1.0f);
				}
			}

			// If the send level changed, then we need to send an update to the audio render thread
			const bool bSendLevelChanged = !FMath::IsNearlyEqual(SendLevel, DynamicBusSendInfo.SendLevel);
			const bool bBusEnablementChanged = bPreviousSourceBusEnablement != PlaybackInstance->bEnableSourceBusSends;

			if (bSendLevelChanged || bBusEnablementChanged)
			{
				DynamicBusSendInfo.SendLevel = SendLevel;
				DynamicBusSendInfo.bIsInit = false;

				FString BusName;

#if ENABLE_ATOM_DEBUG
				BusName = DynamicBusSendInfo.BusName;
#endif // ENABLE_ATOM_DEBUG

				MixerSourceVoice->SetAudioBusSendInfo(DynamicBusSendInfo.BusSendStage, DynamicBusSendInfo.BusID, SendLevel, BusName);

				bPreviousSourceBusEnablement = PlaybackInstance->bEnableSourceBusSends;
			}

		}
	}

	void FAtomExPlayback::UpdateChannelMap()
	{
		//SetLFEBleed();

		// Compute a new speaker map for each possible output channel mapping for the source
		bool bShouldSetMap = false;
		//{
		//	FRWScopeLock Lock(ChannelMapLock, SLT_Write);
			bShouldSetMap = ComputeChannelMap(WaveInfo.NumChannels);
		//}
		//if (bShouldSetMap)
		//{
		//	FRWScopeLock Lock(ChannelMapLock, SLT_ReadOnly);
		//	MixerSourceVoice->SetChannelMap(NumChannels, ChannelMap, bIs3D, WaveInstance->bCenterChannelOnly);
		//}

		//bPrevAllowedSpatializationSetting = IsSpatializationCVarEnabled();
	}

	float FAtomExPlayback::GetEnvelopeValue() const
	{
		if (MixerSourceVoice)
		{
			return MixerSourceVoice->GetEnvelopeValue();
		}
		return 0.0f;
	}

	bool FAtomExPlayback::ComputeChannelMap(const int32 NumSourceChannels)
	{
		if (NumSourceChannels == 1)
		{
			return ComputeMonoChannelMap();
		}
		else if (NumSourceChannels >= 2)
		{
			return ComputeStereoChannelMap();
		}
		//else if (!OutChannelMap.Num())
		//{
		//	MixerDevice->Get2DChannelMap(bIsVorbis, NumSourceChannels, WaveInstance->bCenterChannelOnly, OutChannelMap);
		//	return true;
		//}
		return false;
	}

	bool FAtomExPlayback::ComputeMonoChannelMap()
	{
		if (IsUsingObjectBasedSpatialization())
		{

			// Treat the source as if it is a 2D stereo source:
			//return ComputeStereoChannelMap();
			return true; // adx object based
		}
		else if (PlaybackInstance->GetUseSpatialization() && (!FMath::IsNearlyEqual(PlaybackInstance->AbsoluteAzimuth, PreviousAzimuth, 0.01f) /* || MixerSourceVoice->NeedsSpeakerMap()*/))
		{
			// Don't need to compute the source channel map if the absolute azimuth hasn't changed much
			PreviousAzimuth = PlaybackInstance->AbsoluteAzimuth;

			return true;
		}

		return false;
	}

	bool FAtomExPlayback::ComputeStereoChannelMap()
	{
		if (PlaybackInstance->GetUseSpatialization() && (!FMath::IsNearlyEqual(PlaybackInstance->AbsoluteAzimuth, PreviousAzimuth, 0.01f)/* || MixerSourceVoice->NeedsSpeakerMap()*/))
		{
			// Make sure our stereo emitter positions are updated relative to the sound emitter position
			if (WaveInfo.NumChannels == 2)
			{
				// Make sure our stereo emitter positions are updated relative to the sound emitter position
				UpdateStereoEmitterPositions();
			}

			// Check whether voice is currently using 
			if (!IsUsingObjectBasedSpatialization())
			{
				float AzimuthOffset = 0.0f;

				const float DistanceToUse = AtomUseListenerOverrideForSpreadCVar ? PlaybackInstance->ListenerToSoundDistance : PlaybackInstance->ListenerToSoundDistanceForPanning;

				if (DistanceToUse > UE_KINDA_SMALL_NUMBER)
				{
					AzimuthOffset = FMath::Atan(0.5f * PlaybackInstance->StereoSpread / DistanceToUse);
					AzimuthOffset = FMath::RadiansToDegrees(AzimuthOffset);
				}

				// Atom panning is only multichannel so stereo is limited to a wideness angle of 30 degre.
				// panspread => {0.0, 1.0} => {0 degree, 30 degrees} 
				const float Wideness = AzimuthOffset * 0.033333f;
				FCriWareApi::criAtomExPlayer_SetWideness(MixerPlayer->ExPlayer, (CriFloat32)Wideness);

				return true;
			}
		}

		return true; // channel mapping is controlled by Atom SDK
	}

	void FAtomExPlayback::UpdateCueParameters()
	{
		// Selector Labels

		// Clear selector labels if they need clearing.
		if (PreviousCueSelectorParams.Num() > 0)
		{
			// Loop through every previous label setting
			for (FAtomSelectorParam& PreviousSelectorParam : PreviousCueSelectorParams)
			{
				// See if it's in the current label list
				if (!PlaybackInstance->CueSelectorParams.FindByPredicate([PreviousSelectorParam](const FAtomSelectorParam& Param)
					{
						return PreviousSelectorParam.Name == Param.Name;
					}))
				{
					// If it's not in the current label list, ask for a complete clear.
					if (!PreviousSelectorParam.Name.IsNone())
					{
						// Atom clear all labels, but we will reapply all current labels.
						// it will not change things since Update() is not called between the ClearSelectorLabels() and SetSelectorLabel()
						FCriWareApi::criAtomExPlayer_ClearSelectorLabels(MixerPlayer->ExPlayer);
						break;
					}
				}
			}
		}
		PreviousCueSelectorParams = PlaybackInstance->CueSelectorParams;

		// Apply the current selector labels
		SetCueSelectorLabels();

		// Block Index
		int32 BlockIndex = PlaybackInstance->CueNextBlockIndex;
		if (BlockIndex != PreviousCueNextBlockIndex)
		{
			FCriWareApi::criAtomExPlayback_SetNextBlockIndex(PlaybackID, (CriAtomExBlockIndex)BlockIndex);
			PreviousCueNextBlockIndex = BlockIndex;
		}

		// BeatSync
		CriAtomExBeatSyncInfo ExBeatSyncInfo;
		CriBool bIsBeatSyncPresent = FCriWareApi::criAtomExPlayback_GetBeatSyncInfo(PlaybackID, &ExBeatSyncInfo);
		if (bIsBeatSyncPresent == CRI_TRUE)
		{
			// sync info
			if (!CueBeatSyncInfo.IsValid())
			{
				CueBeatSyncInfo = MakeShareable(new FAtomBeatSyncInfo{});
			}
			FAtomBeatSyncInfo& BeatSyncInfo = *CueBeatSyncInfo.Get();
			BeatSyncInfo.BarCount = (int32)ExBeatSyncInfo.bar_count;
			BeatSyncInfo.BeatCount = (int32)ExBeatSyncInfo.beat_count;
			BeatSyncInfo.BeatProgress = (float)ExBeatSyncInfo.beat_progress;
			BeatSyncInfo.BPM = (float)ExBeatSyncInfo.bar_count;
			BeatSyncInfo.Offset = (int32)ExBeatSyncInfo.offset;
			BeatSyncInfo.NumBeatsPerBar = (int32)ExBeatSyncInfo.num_beats;
			PlaybackInstance->ActiveSound->SetCueBeatSyncInfo(CueBeatSyncInfo);

			// set offset prameter (no-op if not changed)
			int32 BeatSyncOffset = PlaybackInstance->CueBeatSyncOffset;
			FCriWareApi::criAtomExPlayback_SetBeatSyncOffset(PlaybackID, (CriSint16)BeatSyncOffset);
		}
	}

	void FAtomExPlayback::SetLooping()
	{
		// A cue controls various track and block looping, so behavior should never be overwritten in player.
		if (Cast<UAtomSoundCue>(PlaybackInstance->SoundData) == nullptr)
		{
			// Applies the looping mode to player
			if (PlaybackInstance->LoopingMode == EAtomLoopingMode::LoopForever
				|| PlaybackInstance->LoopingMode == EAtomLoopingMode::LoopWithNotification)
			{
				FCriWareApi::criAtomExPlayer_LimitLoopCount(MixerPlayer->ExPlayer, CRIATOMEXPLAYER_FORCE_LOOP);
			}
			else
			{
				FCriWareApi::criAtomExPlayer_LimitLoopCount(MixerPlayer->ExPlayer, CRIATOMEXPLAYER_IGNORE_LOOP);
			}
		}
	}

	void FAtomExPlayback::SetCategories()
	{
		for (auto CategoryName : PlaybackInstance->CategoryNames)
		{
			FCriWareApi::criAtomExPlayer_SetCategoryByName(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*CategoryName.ToString()));
		}
	}

	void FAtomExPlayback::UnsetCategories()
	{
		// note: ResetParameters also flush
		FCriWareApi::criAtomExPlayer_UnsetCategory(MixerPlayer->ExPlayer);
	}

	void FAtomExPlayback::AttachAisacPatches()
	{
		for (auto PatchName : PlaybackInstance->AdditionalAisacPatchNames)
		{
			FCriWareApi::criAtomExPlayer_AttachAisac(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*PatchName.ToString()));
		}
	}

	void FAtomExPlayback::DetachAisacPatches()
	{
		// note: ResetParameters also flush
		FCriWareApi::criAtomExPlayer_DetachAisacAll(MixerPlayer->ExPlayer);
	}

	void FAtomExPlayback::SetCueSelectorLabels()
	{
		for (const FAtomSelectorParam& SelectorParam : PlaybackInstance->CueSelectorParams)
		{

#if WITH_EDITOR
			if (SelectorParam.Name.IsNone() || SelectorParam.Label.IsNone())
			{
				UE_LOG(LogCriWareAtomMixerDebug, Error, TEXT("Failed to set selector label for sound '%s' because it is not valid."), *PlaybackInstance->SoundData->GetFullName());
				continue;
			}
#endif
			check(!SelectorParam.Name.IsNone());
			check(!SelectorParam.Label.IsNone());

			FCriWareApi::criAtomExPlayer_SetSelectorLabel(MixerPlayer->ExPlayer, TCHAR_TO_UTF8(*SelectorParam.Name.ToString()), TCHAR_TO_UTF8(*SelectorParam.Label.ToString()));
		}
	}

	void FAtomExPlayback::SetCueFirstBlockIndex()
	{
		if (!MixerPlayer->ExPlayer.IsValid())
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("The first block number to play could not be set because the Atom player is not valid."));
			return;
		}

		int32 BlockIndex = PlaybackInstance->ActiveSound->CueFirstBlockIndex;
		if (BlockIndex > INDEX_NONE)
		{
			FCriWareApi::criAtomExPlayer_SetFirstBlockIndex(MixerPlayer->ExPlayer, (CriAtomExBlockIndex)BlockIndex);
		}
	}

	void FAtomExPlayback::Play()
	{
#ifdef ATOM_SOUND_STATUS_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Play %d"), Status);
#endif
		if (!PlaybackInstance)
		{
			return;
		}

		// Don't restart the sound if it was stopping when we paused, just stop it.
		if (bIsPaused && (bIsStopping /* || IsPlaybackFinished() */))
		{
#ifdef ATOM_SOUND_STATUS_DEBUG
			UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Stop %d"), Status);
#endif
			StopNow();
			return;
		}

		if (bIsStopping)
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Restarting an Atom source player which was stopping. Stopping now."));
			return;
		}

		ATOM_MIXER_TRACE_CPUPROFILER_EVENT_SCOPE(FAtomExPlayback::Play);

		if (MixerPlayer->ExPlayer.IsValid() && bIsInitialized)
		{
			if (!bIsExternal && !PlaybackInstance->WasCreatedByMixer())
			{
#ifdef ATOM_SOUND_STATUS_DEBUG
				UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("Resume %d"), Status);
#endif
				MixerSourceVoice->Play();
				//FCriWareApi::criAtomExPlayback_Resume(PlaybackID, CriAtomExResumeMode::CRIATOMEX_RESUME_ALL_PLAYBACK);
			}

#if ATOM_PROFILERTRACE_ENABLED
			const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
			if (bChannelEnabled && PlaybackInstance)
			{
				if (const FAtomActiveSound* ActiveSound = PlaybackInstance->ActiveSound)
				{
					int32 TraceSourceID = INDEX_NONE;
					if (MixerSourceVoice)
					{
						TraceSourceID = MixerSourceVoice->GetSourceID();
					}

					UE_TRACE_LOG(CriWareAtom, MixerSourceStart, AtomMixerChannel)
						<< MixerSourceStart.RuntimeID(AtomRuntime->GetAtomRuntimeID())
						<< MixerSourceStart.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceStart.PlayOrder(PlaybackInstance->GetPlayOrder())
						<< MixerSourceStart.SourceID(TraceSourceID)
						<< MixerSourceStart.ComponentID(ActiveSound->GetAtomComponentID())
						<< MixerSourceStart.Name(*PlaybackInstance->SoundData->GetPathName());
				}
			}
#endif // Atom_PROFILERTRACE_ENABLED
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("AtomExPlayback is not ready."));
		}

		bIsPaused = false;
		bIsPlaying = true;
		bIsStopping = false;
	}

	void FAtomExPlayback::Pause()
	{
		if (!PlaybackInstance)
		{
			return;
		}

		if (bIsStopping)
		{
			return;
		}

		if (MixerPlayer->ExPlayer.IsValid() && PlaybackID != CRIATOMEX_INVALID_PLAYBACK_ID)
		{
			MixerSourceVoice->Pause();
			//FCriWareApi::criAtomExPlayback_Pause(PlaybackID, CRI_TRUE);
		}

		bIsPaused = true;
	}

	void FAtomExPlayback::Stop()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		if (!bIsInitialized)
		{
			return;
		}

		if (!MixerPlayer->ExPlayer.IsValid())
		{
			StopNow();
			return;
		}

		UAtomSoundBase* Sound = PlaybackInstance ? PlaybackInstance->SoundData : nullptr;
		// If MarkAsGarbage() was called, Sound can be null
		if (!Sound)
		{
			StopNow();
			return;
		}

		// Stop procedural sounds immediately that don't require fade
		/*if (Sound->bProcedural && !Sound->bRequiresStopFade)
		{
			StopNow();
			return;
		}*/

		if (IsPlaybackFinished())
		{
			StopNow();
			return;
		}

		// Otherwise, we need to do a quick fade-out of the sound and put the state
		// of the sound into "stopping" mode. This prevents this source from
		// being put into the "free" pool and prevents the source from freeing its resources
		// until the sound has finished naturally (i.e. faded all the way out)

		if (!bIsStopping)
		{
			// Let the playback instance know it's stopping
			PlaybackInstance->SetStopping(true);

			if (!bIsExternal)
			{
#ifdef ATOM_SOUND_DEV_DEBUG
				UE_LOG(LogCriWareAtomMixerDebug, Warning, TEXT("call stop until end... <%p> <%p> [%d]"), MixerPlayer->ExPlayer.Get(), MixerPlayer->ExSource.Get(), PlaybackID);
#endif
				MixerSourceVoice->StopFade();
				//FCriWareApi::criAtomExPlayback_Stop(PlaybackID);
			}
			bIsStopping = true;
			bIsPaused = false;

			// FAtomSource::Stop() Will be excuted by StopNow()
		}
	}

	void FAtomExPlayback::StopNow()
	{
		LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMixer);

		// Immediately stop the sound source

		InitializationState = EAtomExPlaybackInitializationState::NotInitialized;
		bIsInitialized = false; // maybe replace by InitializationState
		bIsStopping = false;

		if (MixerPlayer->ExPlayer.IsValid())
		{
			if (bIsPlaying && !bIsExternal)
			{
#ifdef ATOM_SOUND_DEV_DEBUG
				UE_LOG(LogCriWareAtomMixerDebug, Log, TEXT("call stop without release <%p> <%p> [%d]"), MixerPlayer->ExPlayer.Get(), MixerPlayer->ExSource.Get(), PlaybackID);
#endif
#if ATOM_PROFILERTRACE_ENABLED
				const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomMixerChannel);
				if (bChannelEnabled)
				{
					int32 TraceSourceID = INDEX_NONE;
					if (MixerSourceVoice)
					{
						TraceSourceID = MixerSourceVoice->GetSourceID();
					}

					UE_TRACE_LOG(CriWareAtom, MixerSourceStop, AtomMixerChannel)
						<< MixerSourceStop.RuntimeID(AtomRuntime->GetAtomRuntimeID())
						<< MixerSourceStop.Timestamp(FPlatformTime::Cycles64())
						<< MixerSourceStop.PlayOrder(PlaybackInstance->GetPlayOrder());
				}
#endif // ATOM_PROFILERTRACE_ENABLED

				MixerSourceVoice->Stop();
				//FCriWareApi::criAtomExPlayback_StopWithoutReleaseTime(PlaybackID);
			}

			if (PlaybackInstance)
			{
				if (PlaybackInstance->AdditionalAisacPatchNames.Num() > 0)
				{
					DetachAisacPatches();
				}
			}
		}

		bIsPaused = false;
		bIsPlaying = false;

		// Free/reset Parameters and Resources

		FreeResources(EFreeReason::Stop);

		FAtomSource::Stop();
	}

	bool FAtomExPlayback::IsStopping()
	{
		return bIsStopping;
	}

	bool FAtomExPlayback::IsFinished()
	{
		// A paused source is not finished.
		if (bIsPaused)
		{
			return false;
		}

		if (!bIsInitialized) // EAtomPLayerExInitializationState::NotInitialized)
		{
			return true;
		}

		if (bIsPreparingForInit) // EAtomPLayerExInitializationState::Initializing
		{
			return false;
		}

		if (PlaybackInstance && MixerPlayer->ExPlayer.IsValid())
		{
			if (IsPlaybackFinished())
			{
				PlaybackInstance->NotifyFinished();
				bIsStopping = false;
				return true;
			}
			//else if (bLoopCallback && WaveInstance->LoopingMode == LOOP_WithNotification)
			//{
			//	WaveInstance->NotifyFinished();
			//	bLoopCallback = false;
			//}
		}

		return false;
	}

	bool FAtomExPlayback::IsPlaybackFinished()
	{
		// This check now if the playback is finished to avoid calls on removed playback.
		// Remove event from PlaybackListener::OnPlaybackEvent() will come after this.
		// To avoid a prematured stop with some cues that trigger other sounds and create new playback instances, 
		// ActiveSound checks if the MixerPlayer is still alive trought the Update() function before marks as fully finished.
		const bool bIsFinished = FCriWareApi::criAtomExPlayback_GetStatus(PlaybackID) == CRIATOMEXPLAYBACK_STATUS_REMOVED;
#ifdef ATOM_SOUND_DEV_DEBUG
		UE_LOG(LogCriWareAtomMixerDebug, Display, TEXT("IsPlaybackFinished: %s."), bIsFinished ? TEXT("YES") : TEXT("NO"));
#endif
		return bIsFinished;
	}

	float FAtomExPlayback::GetPlaybackPercent() const
	{
		if (!bIsInitialized)// InitializationState != EMixerSourceInitializationState::Initialized)
		{
			//return PreviousPlaybackPercent; // from epic -> bugged
			return 0.f;
		}

		if (MixerSourceVoice && NumTotalFrames > 0)
		{
			// StartFrames is included, SampleRate is the rate of the source same as NumTotalFrames.
			CriSint64 NumSamplesPlayed = 0;
			CriSint32 SampleRate = 0;
			FCriWareApi::criAtomExPlayback_GetNumPlayedSamples(PlaybackID, &NumSamplesPlayed, &SampleRate);

			PreviousPlaybackPercent = (float)NumSamplesPlayed / NumTotalFrames;
			if (PlaybackInstance->LoopingMode == EAtomLoopingMode::LoopNever)
			{
				PreviousPlaybackPercent = FMath::Min(PreviousPlaybackPercent, 1.0f);
			}
			return PreviousPlaybackPercent;
		}
		else
		{
			// If we don't have any frames, that means it's some procedural sound, which means
			// that we're never going to have a playback percentage.
			return 1.0f;
		}
	}

	/* FAtomExternalExPlayback implementation
	 *****************************************************************************/

	FAtomExternalExPlayback::FAtomExternalExPlayback(FAtomRuntime* InAtomRuntime, FCriAtomExPlayerPtr&& InExternalExPlayer)
		: FAtomExPlayback(InAtomRuntime)
		, TempExternalExPlayer(MoveTemp(InExternalExPlayer))
	{
	}

	FAtomExternalExPlayback::~FAtomExternalExPlayback()
	{
	}

	bool FAtomExternalExPlayback::PrepareForInitialization(FAtomPlaybackInstance* InPlaybackInstance)
	{
		if (!ensure(InPlaybackInstance))
		{
			return false;
		}

		if (InPlaybackInstance)
		{
			// SoundData must be valid beyond this point.
			check(InPlaybackInstance->SoundData);

			// MixerPlayer (MixerBuffer)
			FAtomExPlayerArgs InitPlayerArgs;
			InitPlayerArgs.AtomRuntime = AtomRuntime;
			InitPlayerArgs.ActiveSound = InPlaybackInstance->ActiveSound;
			InitPlayerArgs.SoundData = InPlaybackInstance->SoundData;

			if (TempExternalExPlayer.IsValid())
			{
				MixerPlayer = FAtomExPlayer::CreateWithExternalPlayer(InitPlayerArgs, MoveTemp(TempExternalExPlayer)).ToSharedRef();
				MixerSourcePlayer.Player = MixerPlayer;
			}
			//else
			//{
			//	UE_LOG(LogCriWareAtomMixer, Error, TEXT("Failed to create external AtomExPlayer for playback instance %s."), *InPlaybackInstance->GetName());
			//	return false;
			//}
		}

		return FAtomExPlayback::PrepareForInitialization(InPlaybackInstance);
	}
} // namespace
