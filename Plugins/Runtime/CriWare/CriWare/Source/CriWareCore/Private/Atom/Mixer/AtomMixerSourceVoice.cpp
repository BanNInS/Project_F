
#include "Atom/Mixer/AtomMixerSourceVoice.h"

#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerSource.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/AtomRuntime.h"

namespace Atom
{
	/**
	* FMixerSourceVoice Implementation
	*/

	FMixerSourceVoice::FMixerSourceVoice()
	{
		Reset(nullptr);
	}

	FMixerSourceVoice::~FMixerSourceVoice()
	{
	}

	void FMixerSourceVoice::Reset(FAtomRuntime* InAtomRuntime)
	{
		if (InAtomRuntime)
		{
			AtomRuntime = InAtomRuntime;
			SourceManager = AtomRuntime->GetSourceManager();
		}
		else
		{
			AtomRuntime = nullptr;
			SourceManager = nullptr;
		}

		//Pitch = -1.0f;
		//Volume = -1.0f;
		//DistanceAttenuation = -1.0f;
		//Distance = -1.0f;
		//LPFFrequency = -1.0f;
		//HPFFrequency = -1.0f;
		SourceID = INDEX_NONE;
		bIsPlaying = false;
		bIsPaused = false;
		bIsActive = false;
		bIsSourceBus = false;
		bEnableSourceBusSends = false;
		bEnableBaseSubmix = false;
		bEnableSubmixSends = false;
		bStopFadedOut = false;

		PitchModBase = TNumericLimits<float>::Max();
		VolumeModBase = TNumericLimits<float>::Max();
		LPFFrequencyModBase = TNumericLimits<float>::Max();
		HPFFrequencyModBase = TNumericLimits<float>::Max();

		//SubmixSends.Reset();

		// destroy the pool/port
		VoicePool = nullptr;
		CodecType = EMixerSourceVoiceCodecType::Undefined;
		StreamingType = EMixerSourceVoiceStreamingType::Mixed;
		PoolID = INDEX_NONE;
		MaxInputVoices = 0;
		MaxInputChannels = 0;
		MaxInputSampleRate = 0;

		VoiceEffect.Reset();
	}

	bool FMixerSourceVoice::Init(const FMixerSourceVoiceInitParams& InitParams)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceManager->GetFreeSourceID(SourceID))
		{
			ATOM_MIXER_CHECK(InitParams.SourceListener != nullptr);
			ATOM_MIXER_CHECK(InitParams.NumInputChannels > 0);

			//bEnableBusSends = InitParams.bEnableBusSends;
			//bEnableBaseSubmix = InitParams.bEnableBaseSubmix;
			//bEnableSubmixSends = InitParams.bEnableSubmixSends;

			bIsSourceBus = InitParams.AudioBusID != INDEX_NONE;

			//for (int32 i = 0; i < InitParams.SubmixSends.Num(); ++i)
			//{
			//	FMixerSubmixPtr SubmixPtr = InitParams.SubmixSends[i].Submix.Pin();
			//	if (SubmixPtr.IsValid())
			//	{
			//		SubmixSends.Add(SubmixPtr->GetId(), InitParams.SubmixSends[i]);
			//	}
			//}

			bStopFadedOut = false;
			SourceManager->InitSource(SourceID, InitParams);
			return true;
		}

		return false;
	}

	void FMixerSourceVoice::Release()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		SourceManager->ReleaseSourceID(SourceID);
	}

	/*void FMixerSourceVoice::SetPitch(const float InPitch)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(Pitch, InPitch, SourceManager->GetFloatCompareTolerance()))
		{
			Pitch = InPitch;
			SourceManager->SetPitch(SourceID, InPitch);
		}
	}*/

	/*void FMixerSourceVoice::SetVolume(const float InVolume)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(Volume, InVolume, SourceManager->GetFloatCompareTolerance()))
		{
			Volume = InVolume;
			SourceManager->SetVolume(SourceID, InVolume);
		}
	}*/

	/*void FMixerSourceVoice::SetDistanceAttenuation(const float InDistanceAttenuation)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(DistanceAttenuation, InDistanceAttenuation, SourceManager->GetFloatCompareTolerance()))
		{
			DistanceAttenuation = InDistanceAttenuation;
			SourceManager->SetDistanceAttenuation(SourceID, InDistanceAttenuation);
		}
	}*/

	/*void FMixerSourceVoice::SetLPFFrequency(const float InLPFFrequency)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(LPFFrequency, InLPFFrequency, SourceManager->GetFloatCompareTolerance()))
		{
			LPFFrequency = InLPFFrequency;
			SourceManager->SetLPFFrequency(SourceID, LPFFrequency);
		}
	}*/

	/*void FMixerSourceVoice::SetHPFFrequency(const float InHPFFrequency)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(HPFFrequency, InHPFFrequency, SourceManager->GetFloatCompareTolerance()))
		{
			HPFFrequency = InHPFFrequency;
			SourceManager->SetHPFFrequency(SourceID, HPFFrequency);
		}
	}*/

	void FMixerSourceVoice::SetVoiceEffect(const FAtomSourceVoiceEffect& Params)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// update the attached dsp effect to the voice pool
		if (VoiceEffect.Type != Params.Type && VoicePool.IsValid())
		{
			// always call detach if something is set or not
			FCriWareApi::criAtomExVoicePool_DetachDsp(VoicePool);

			switch (Params.Type)
			{
			case EAtomSourceVoiceEffectType::PitchShifter:
			{
				CriAtomExDspPitchShifterConfig Config;
				criAtomExVoicePool_SetDefaultConfigForDspPitchShifter(&Config);
				Config.max_channels = MaxInputChannels;
				Config.max_sampling_rate = MaxInputSampleRate;
				Config.num_dsp = MaxInputVoices;
				FCriWareApi::criAtomExVoicePool_AttachDspPitchShifter(VoicePool, &Config, nullptr, 0);
			}
			break;
			case EAtomSourceVoiceEffectType::TimeStretch:
			{
				CriAtomExDspTimeStretchConfig Config;
				criAtomExVoicePool_SetDefaultConfigForDspTimeStretch(&Config);
				Config.max_channels = MaxInputChannels;
				Config.max_sampling_rate = MaxInputSampleRate;
				Config.num_dsp = MaxInputVoices;
				FCriWareApi::criAtomExVoicePool_AttachDspTimeStretch(VoicePool, &Config, nullptr, 0);
			}
			break;
			case EAtomSourceVoiceEffectType::Disabled:
			default:
			{
				// nothing to do
			}
			}

			// params at attach time
			VoiceEffect = Params;
		}

		SourceManager->SetVoiceEffect(SourceID, Params);
	}

	void FMixerSourceVoice::SetModVolume(const float InVolumeModBase)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(VolumeModBase, InVolumeModBase, SourceManager->GetFloatCompareTolerance()))
		{
			VolumeModBase = InVolumeModBase;
			SourceManager->SetModVolume(SourceID, VolumeModBase);
		}
	}

	void FMixerSourceVoice::SetModPitch(const float InPitchModBase)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(PitchModBase, InPitchModBase, SourceManager->GetFloatCompareTolerance()))
		{
			PitchModBase = InPitchModBase;
			SourceManager->SetModPitch(SourceID, InPitchModBase);
		}
	}

	void FMixerSourceVoice::SetModHPFFrequency(const float InHPFFrequencyModBase)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(HPFFrequencyModBase, InHPFFrequencyModBase, SourceManager->GetFloatCompareTolerance()))
		{
			HPFFrequencyModBase = InHPFFrequencyModBase;
			SourceManager->SetModHPFFrequency(SourceID, InHPFFrequencyModBase);
		}
	}

	void FMixerSourceVoice::SetModLPFFrequency(const float InLPFFrequencyModBase)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!FMath::IsNearlyEqual(LPFFrequencyModBase, InLPFFrequencyModBase, SourceManager->GetFloatCompareTolerance()))
		{
			LPFFrequencyModBase = InLPFFrequencyModBase;
			SourceManager->SetModLPFFrequency(SourceID, InLPFFrequencyModBase);
		}
	}

	void FMixerSourceVoice::SetModulationRouting(FAtomSoundModulationDefaultSettings& RoutingSettings)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		SourceManager->SetModulationRouting(SourceID, RoutingSettings);
	}

	void FMixerSourceVoice::Play()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		bIsPlaying = true;
		bIsPaused = false;
		bIsActive = true;

		SourceManager->Play(SourceID);
	}

	void FMixerSourceVoice::Pause()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		bIsPaused = true;
		bIsActive = false;
		SourceManager->Pause(SourceID);
	}

	void FMixerSourceVoice::Stop()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		bIsPlaying = false;
		bIsPaused = false;
		bIsActive = false;
		// We are instantly fading out with this stop command
		bStopFadedOut = true;
		SourceManager->Stop(SourceID);
	}

	void FMixerSourceVoice::StopFade(/*int32 NumFrames*/)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		bIsPaused = false;
		SourceManager->StopFade(SourceID/*, NumFrames */);
	}

	bool FMixerSourceVoice::IsPlaying() const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		return bIsPlaying;
	}

	bool FMixerSourceVoice::IsPaused() const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		return bIsPaused;
	}

	bool FMixerSourceVoice::IsActive() const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		return bIsActive;
	}

	bool FMixerSourceVoice::IsUsingHRTFSpatializer(bool bDefaultValue) const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			return SourceManager->IsUsingHRTFSpatializer(SourceID);
		}

		return bDefaultValue;
	}

	float FMixerSourceVoice::GetEnvelopeValue() const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		return SourceManager->GetEnvelopeValue(SourceID);
	}

	void FMixerSourceVoice::SetEnablement(bool bInEnableSourceBusSendRouting, bool bInEnableMainSubmixOutput, bool bInEnableSubmixSendRouting)
	{
		bEnableSourceBusSends = bInEnableSourceBusSendRouting;
		bEnableBaseSubmix = bInEnableMainSubmixOutput;
		bEnableSubmixSends = bInEnableSubmixSendRouting;
	}

	void FMixerSourceVoice::SetAudioBusSendInfo(EAtomBusSendStage InBusSendStage, uint32 AudioBusID, float BusSendLevel, const FString& InBusName)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (!bEnableSourceBusSends)
		{
			BusSendLevel = 0.0f;
		}

		SourceManager->SetBusSendInfo(SourceID, InBusSendStage, AudioBusID, BusSendLevel, InBusName);
	}

	// temp -> connect atomexplayer directy to sourcemanager !
	void FMixerSourceVoice::SubmitBuffer(float** Data, int NumChannels, int NumFrames)
	{
		SourceManager->SubmitBuffer(SourceID, Data, NumChannels, NumFrames);
	}

	/*void FMixerSourceVoice::OnMixBus(FMixerSourceVoiceBuffer* OutMixerSourceBuffer)
	{
		ATOM_MIXER_CHECK_AUDIO_PLAT_THREAD(AtomRuntime);

		check(OutMixerSourceBuffer->AudioData.Num() > 0);

		for (int32 i = 0; i < OutMixerSourceBuffer->AudioData.Num(); ++i)
		{
			OutMixerSourceBuffer->AudioData[i] = 0.0f;
		}
	}*/

	int64 FMixerSourceVoice::GetNumFramesPlayed() const
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			return SourceManager->GetNumFramesPlayed(SourceID);
		}

		return 0;
	}

	FAtomPlaybackId FMixerSourceVoice::InitMixerPlayer()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			return SourceManager->InitSourcePlayer(SourceID);
		}

		return CRIATOMEX_INVALID_PLAYBACK_ID;
	}


	float FMixerSourceVoice::GetModVolumeEnd()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			auto& SourceInfo = SourceManager->SourceInfos[SourceID];

			//const bool bHasProcessed = SourceInfo.VolumeModulation.GetHasProcessed();
			//const float ModVolumeStart = SourceInfo.VolumeModulation.GetValue();
			SourceInfo.VolumeModulation.ProcessControl(SourceInfo.VolumeModulationBase);
			const float ModVolumeEnd = SourceInfo.VolumeModulation.GetValue();
			//VolumeStart *= bHasProcessed ? ModVolumeStart : ModVolumeEnd;

			return ModVolumeEnd;
		}

		return 1.0f;
	}

	float FMixerSourceVoice::GetModPitchEnd()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			auto& SourceInfo = SourceManager->SourceInfos[SourceID];

			SourceInfo.PitchModulation.ProcessControl(SourceInfo.PitchModulationBase);
			const float ModPitchEnd = SourceInfo.PitchModulation.GetValue();

			return ModPitchEnd;
		}

		return 1.0f;
	}

	float FMixerSourceVoice::GetModLPFEnd()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			auto& SourceInfo = SourceManager->SourceInfos[SourceID];

			SourceInfo.LowpassModulation.ProcessControl(SourceInfo.LowpassModulationBase);
			const float ModLPFEnd = SourceInfo.LowpassModulation.GetValue();

			return ModLPFEnd;
		}

		return 1.0f;
	}

	float FMixerSourceVoice::GetModHPFEnd()
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			auto& SourceInfo = SourceManager->SourceInfos[SourceID];

			SourceInfo.HighpassModulation.ProcessControl(SourceInfo.HighpassModulationBase);
			const float ModHPFEnd = SourceInfo.HighpassModulation.GetValue();

			return ModHPFEnd;
		}

		return 1.0f;
	}

	bool FMixerSourceVoice::IterateOverAisacModulations(TFunction<bool(FAtomAisacControl& Control, FModulationDestination& Modulation, float ModulationBase)> Func)
	{
		ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		if (SourceID != INDEX_NONE)
		{
			for (auto& AisacControlModulation : SourceManager->SourceInfos[SourceID].AisacControlModulations)
			{
				if (!Func(AisacControlModulation.Key, AisacControlModulation.Value.Modulation, AisacControlModulation.Value.ModulationBase))
				{
					return false;
				}
			}
		}

		return true;
	}
} // namespace
