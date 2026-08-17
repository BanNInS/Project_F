/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBlueprintLibrary.cpp
 *
 ****************************************************************************/

#include "Atom/AtomBlueprintLibrary.h"

#include <string>

#include "DSP/ConstantQ.h"

#include "CriWarePlatformUtils.h"
#include "Atom/Atom.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomResourceManager.h"

#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerPlayer.h"
#include "Atom/Mixer/AtomMixerSubmix.h"

 /*
  * UAtomRuntimeBlueprintLibrary
  *****************************************************************************/

void UAtomRuntimeBlueprintLibrary::PrimeSoundForPlayback(UAtomSoundBase* Sound, const FOnAtomSoundLoadComplete OnLoadCompletion)
{
	if (!Sound)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Prime Sound For Playback called with a null Sound pointer."));
	}
	else if (IAtomSoundResource* Source = Sound->GetSoundResource())
	{
		FAtomLoadParams LoadParams;
		LoadParams.Source = Source;
		LoadParams.Scope = EAtomResourceScope::Shared;
		LoadParams.bIsStreamed = true;
		LoadParams.bAsyncLoad = true;
		LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::GameThread;
		LoadParams.OnLoadCompleted = [OnLoadCompletion, Sound](EAtomResourceLoadResult InResult)
		{
			if (InResult == EAtomResourceLoadResult::Completed || InResult == EAtomResourceLoadResult::AlreadyLoaded)
			{
				OnLoadCompletion.ExecuteIfBound(Sound, false);
			}
			else
			{
				OnLoadCompletion.ExecuteIfBound(Sound, true);
			}
		};

		FAtomResourceHandle AtomResource = FAtomRuntimeManager::Get()->GetAtomResourceManager().RequestSoundResource(LoadParams);
		if (auto SoundBank = Cast<UAtomSoundBank>(Source))
		{
			SoundBank->AtomResource = AtomResource;
		}
	}
}

void UAtomRuntimeBlueprintLibrary::PrimeCueSheetForPlayback(UAtomCueSheet* CueSheet, const FOnAtomCueSheetLoadComplete OnLoadCompletion)
{
	if (!CueSheet)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Prime Cue Sheet For Playback called with a null CueSheet pointer."));
	}
	else
	{
		FAtomLoadParams LoadParams;
		LoadParams.Source = CueSheet;
		LoadParams.Scope = EAtomResourceScope::Shared;
		LoadParams.bIsStreamed = true;
		LoadParams.bAsyncLoad = true;
		LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::GameThread;
		LoadParams.OnLoadCompleted = [OnLoadCompletion, CueSheet](EAtomResourceLoadResult InResult)
		{
			if (InResult == EAtomResourceLoadResult::Completed || InResult == EAtomResourceLoadResult::AlreadyLoaded)
			{
				OnLoadCompletion.ExecuteIfBound(CueSheet, false);
			}
			else
			{
				OnLoadCompletion.ExecuteIfBound(CueSheet, true);
			}
		};

		CueSheet->AtomResource = FAtomRuntimeManager::Get()->GetAtomResourceManager().RequestSoundResource(LoadParams);
	}
}

void UAtomRuntimeBlueprintLibrary::PrimeWaveBankForPlayback(UAtomWaveBank* WaveBank, const FOnAtomWaveBankLoadComplete OnLoadCompletion)
{
	if (!WaveBank)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("Prime Wave Bank For Playback called with a null WaveBank pointer."));
	}
	else
	{
		FAtomLoadParams LoadParams;
		LoadParams.Source = WaveBank;
		LoadParams.Scope = EAtomResourceScope::Shared;
		LoadParams.bIsStreamed = true;
		LoadParams.bAsyncLoad = true;
		LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::GameThread;
		LoadParams.OnLoadCompleted = [OnLoadCompletion, WaveBank](EAtomResourceLoadResult InResult)
		{
			if (InResult == EAtomResourceLoadResult::Completed || InResult == EAtomResourceLoadResult::AlreadyLoaded)
			{
				OnLoadCompletion.ExecuteIfBound(WaveBank, false);
			}
			else
			{
				OnLoadCompletion.ExecuteIfBound(WaveBank, true);
			}
		};
		WaveBank->AtomResource = FAtomRuntimeManager::Get()->GetAtomResourceManager().RequestSoundResource(LoadParams);
	}
}

/** Set a bus effect preset to the master rack, master out bus. */
void UAtomRuntimeBlueprintLibrary::SetMasterEffectPreset(const UObject* WorldContextObject, UAtomBusEffectPreset* BusEffectPreset)
{
	if (!BusEffectPreset)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("SetMasterEffectPreset was passed invalid bus effect preset"));
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		if (UAtomRack* MasterRack = AtomRuntime->GetMasterRack())
		{
			UAtomBus* MasterBus = MasterRack->MasterBus;
			check(MasterBus);

			MasterBus->SetEffectPreset(BusEffectPreset);
		}
	}
}

/** Removes a bus effect preset from the master rack, master out bus. */
void UAtomRuntimeBlueprintLibrary::RemoveMasterEffectPreset(const UObject* WorldContextObject, UAtomBusEffectPreset* BusEffectPreset)
{
	if (!BusEffectPreset)
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("RemoveMasterEffectPreset was passed invalid bus effect preset"));
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		if (UAtomRack* MasterRack = AtomRuntime->GetMasterRack())
		{
			UAtomBus* MasterBus = MasterRack->MasterBus;
			check(MasterBus);

			MasterBus->RemoveEffectPreset(BusEffectPreset);
		}
	}
}

/** Clears all effect presets from the master rack, master out bus. */
void UAtomRuntimeBlueprintLibrary::ClearMasterEffectPresets(const UObject* WorldContextObject)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		if (UAtomRack* MasterRack = AtomRuntime->GetMasterRack())
		{
			UAtomBus* MasterBus = MasterRack->MasterBus;
			check(MasterBus);

			for (int32 EffectIndex = 0; EffectIndex < MasterBus->GetNumEffectPresets(); EffectIndex++)
			{
				MasterBus->SetDefaultEffectPreset(EffectIndex);
			}
		}
	}
}

/** Set a bus effect preset to the given bus. Returns the index of the effect in the bus or index none if not found. */
int32 UAtomRuntimeBlueprintLibrary::SetBusEffectPreset(const UObject* WorldContextObject, UAtomBus* AtomBus, UAtomBusEffectPreset* BusEffectPreset)
{
	if (!BusEffectPreset || !AtomBus)
	{
		return 0;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		// todo: check if bus's rack is in this runtime

		int32 EffectIndex = AtomBus->FindEffectPreset(BusEffectPreset);
		if (EffectIndex != INDEX_NONE && AtomBus->SetEffectPreset(BusEffectPreset))
		{
			return EffectIndex;
		}
	}

	return INDEX_NONE;
}

/** Removes the bus effect preset at the given bus, given effect index, if there is an effect preset set at that index. Defaulting to original bus settings. */
void UAtomRuntimeBlueprintLibrary::RemoveBusEffectPreset(const UObject* WorldContextObject, UAtomBus* AtomBus, int32 EffectIndex)
{
	if (!AtomBus || EffectIndex < 0)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		// todo: check if bus's rack is in this runtime

		if (UAtomBusEffectPreset* EffectPreset = AtomBus->GetEffectPreset(EffectIndex))
		{
			AtomBus->RemoveEffectPreset(EffectPreset);
		}
	}
}

/** Clears all submix effect presets on the given bus. Defaulting to original bus settings. */
void UAtomRuntimeBlueprintLibrary::ClearBusEffectPresets(const UObject* WorldContextObject, UAtomBus* AtomBus)
{
	if (!AtomBus)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		// todo: check if bus's rack is in this runtime

		for (int32 EffectIndex = 0; EffectIndex < AtomBus->GetNumEffectPresets(); EffectIndex++)
		{
			AtomBus->SetDefaultEffectPreset(EffectIndex);
		}
	}
}

/** Removes all instances of a bus effect preset from any buses of the given Atom rack. */
void UAtomRuntimeBlueprintLibrary::RemoveRackEffectPreset(const UObject* WorldContextObject, UAtomRack* AtomRack, UAtomBusEffectPreset* BusEffectPreset)
{
	if (!AtomRack || !BusEffectPreset)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		// todo: check if rack is in this runtime

		for (UAtomBus* Bus : AtomRack->Buses)
		{
			Bus->RemoveEffectPreset(BusEffectPreset);
		}
	}
}

/** Clears all effect presets from any buses of the given rack. */
void UAtomRuntimeBlueprintLibrary::ClearRackEffectPresets(const UObject* WorldContextObject, UAtomRack* AtomRack)
{
	if (!AtomRack)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		// todo: check if rack is in this runtime

		for (UAtomBus* AtomBus : AtomRack->Buses)
		{
			for (int32 EffectIndex = 0; EffectIndex < AtomBus->GetNumEffectPresets(); EffectIndex++)
			{
				AtomBus->SetDefaultEffectPreset(EffectIndex);
			}
		}
	}
}

void UAtomRuntimeBlueprintLibrary::EnableBinauralizer(const UObject* WorldContextObject, bool bEnable)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->SetHRTFDisabled(!bEnable);
	}
}

bool UAtomRuntimeBlueprintLibrary::IsBinauralizerEnabled(const UObject* WorldContextObject)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		return !AtomRuntime->IsHRTFDisabled();
	}

	return false;
}

void UAtomRuntimeBlueprintLibrary::StartAnalyzingOutput(const UObject* WorldContextObject, UAtomBus* BusToAnalyze, EAtomFFTSize FFTSize, EAtomFFTPeakInterpolationMethod InterpolationMethod, EAtomFFTWindowType WindowType, float HopSize, EAtomSpectrumType AtomSpectrumType)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		FAtomSoundSpectrumAnalyzerSettings Settings = UAtomBus::GetSpectrumAnalyzerSettings(FFTSize, InterpolationMethod, WindowType, HopSize, AtomSpectrumType);
		AtomRuntime->StartSpectrumAnalysis(BusToAnalyze, Settings);
	}
}

void UAtomRuntimeBlueprintLibrary::StopAnalyzingOutput(const UObject* WorldContextObject, UAtomBus* BusToStopAnalyzing)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->StopSpectrumAnalysis(BusToStopAnalyzing);
	}
}

TArray<FAtomBusSpectralAnalysisBandSettings> UAtomRuntimeBlueprintLibrary::MakeMusicalSpectralAnalysisBandSettings(int32 InNumNotes, EAtomMusicalNoteName InStartingMusicalNote, int32 InStartingOctave, int32 InAttackTimeMsec, int32 InReleaseTimeMsec)
{
	// Make values sane.
	InNumNotes = FMath::Clamp(InNumNotes, 0, 10000);
	InStartingOctave = FMath::Clamp(InStartingOctave, -1, 10);
	InAttackTimeMsec = FMath::Clamp(InAttackTimeMsec, 0, 10000);
	InReleaseTimeMsec = FMath::Clamp(InReleaseTimeMsec, 0, 10000);

	// Some assumptions here on what constitutes "music". 12 notes, equal temperament.
	const float BandsPerOctave = 12.f;
	// This QFactor makes the bandwidth equal to the difference in frequency between adjacent notes.
	const float QFactor = 1.f / (FMath::Pow(2.f, 1.f / BandsPerOctave) - 1.f);

	// Base note index off of A4 which we know to be 440 hz.
	// Make note relative to A
	int32 NoteIndex = static_cast<int32>(InStartingMusicalNote) - static_cast<int32>(EAtomMusicalNoteName::A);
	// Make relative to 4th octave of A
	NoteIndex += 12 * (InStartingOctave - 4);

	const float StartingFrequency = 440.f * FMath::Pow(2.f, static_cast<float>(NoteIndex) / 12.f);

	TArray<FAtomBusSpectralAnalysisBandSettings> BandSettingsArray;
	for (int32 i = 0; i < InNumNotes; i++)
	{
		FAtomBusSpectralAnalysisBandSettings BandSettings;

		BandSettings.BandFrequency = Audio::FPseudoConstantQ::GetConstantQCenterFrequency(i, StartingFrequency, BandsPerOctave);

		BandSettings.QFactor = QFactor;
		BandSettings.AttackTimeMsec = InAttackTimeMsec;
		BandSettings.ReleaseTimeMsec = InReleaseTimeMsec;

		BandSettingsArray.Add(BandSettings);
	}

	return BandSettingsArray;
}

TArray<FAtomBusSpectralAnalysisBandSettings> UAtomRuntimeBlueprintLibrary::MakeFullSpectrumSpectralAnalysisBandSettings(int32 InNumBands, float InMinimumFrequency, float InMaximumFrequency, int32 InAttackTimeMsec, int32 InReleaseTimeMsec)
{
	// Make inputs sane.
	InNumBands = FMath::Clamp(InNumBands, 0, 10000);
	InMinimumFrequency = FMath::Clamp(InMinimumFrequency, 20.0f, 20000.0f);
	InMaximumFrequency = FMath::Clamp(InMaximumFrequency, InMinimumFrequency, 20000.0f);
	InAttackTimeMsec = FMath::Clamp(InAttackTimeMsec, 0, 10000);
	InReleaseTimeMsec = FMath::Clamp(InReleaseTimeMsec, 0, 10000);

	// Calculate CQT settings needed to space bands.
	const float NumOctaves = FMath::Loge(InMaximumFrequency / InMinimumFrequency) / FMath::Loge(2.f);
	const float BandsPerOctave = static_cast<float>(InNumBands) / FMath::Max(NumOctaves, 0.01f);
	const float QFactor = 1.f / (FMath::Pow(2.f, 1.f / FMath::Max(BandsPerOctave, 0.01f)) - 1.f);

	TArray<FAtomBusSpectralAnalysisBandSettings> BandSettingsArray;
	for (int32 i = 0; i < InNumBands; i++)
	{
		FAtomBusSpectralAnalysisBandSettings BandSettings;

		BandSettings.BandFrequency = Audio::FPseudoConstantQ::GetConstantQCenterFrequency(i, InMinimumFrequency, BandsPerOctave);

		BandSettings.QFactor = QFactor;
		BandSettings.AttackTimeMsec = InAttackTimeMsec;
		BandSettings.ReleaseTimeMsec = InReleaseTimeMsec;

		BandSettingsArray.Add(BandSettings);
	}

	return BandSettingsArray;
}

TArray<FAtomBusSpectralAnalysisBandSettings> UAtomRuntimeBlueprintLibrary::MakePresetSpectralAnalysisBandSettings(EAtomSpectrumBandPresetType InBandPresetType, int32 InNumBands, int32 InAttackTimeMsec, int32 InReleaseTimeMsec)
{
	float MinimumFrequency = 20.f;
	float MaximumFrequency = 20000.f;

	// Likely all these are debatable. What we are shooting for is the most active frequency
	// ranges, so when an instrument plays a significant amount of spectral energy from that
	// instrument will show up in the frequency range. 
	switch (InBandPresetType)
	{
	case EAtomSpectrumBandPresetType::KickDrum:
		MinimumFrequency = 40.f;
		MaximumFrequency = 100.f;
		break;

	case EAtomSpectrumBandPresetType::SnareDrum:
		MinimumFrequency = 150.f;
		MaximumFrequency = 4500.f;
		break;

	case EAtomSpectrumBandPresetType::Voice:
		MinimumFrequency = 300.f;
		MaximumFrequency = 3000.f;
		break;

	case EAtomSpectrumBandPresetType::Cymbals:
		MinimumFrequency = 6000.f;
		MaximumFrequency = 16000.f;
		break;

		// More presets can be added. The possibilities are endless.
	}

	return MakeFullSpectrumSpectralAnalysisBandSettings(InNumBands, MinimumFrequency, MaximumFrequency, InAttackTimeMsec, InReleaseTimeMsec);
}

void UAtomRuntimeBlueprintLibrary::GetMagnitudeForFrequencies(const UObject* WorldContextObject, const TArray<float>& Frequencies, TArray<float>& Magnitudes, UAtomBus* BusToAnalyze /*= nullptr*/)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->GetMagnitudesForFrequencies(BusToAnalyze, Frequencies, Magnitudes);
	}
}

void UAtomRuntimeBlueprintLibrary::GetPhaseForFrequencies(const UObject* WorldContextObject, const TArray<float>& Frequencies, TArray<float>& Phases, UAtomBus* BusToAnalyze /*= nullptr*/)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->GetPhasesForFrequencies(BusToAnalyze, Frequencies, Phases);
	}
}

void UAtomRuntimeBlueprintLibrary::StartAudioBus(const UObject* WorldContextObject, UAtomAudioBus* AudioBus)
{
	if (!AudioBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Start Audio Bus called with an invalid Audio Bus."));
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		uint32 AudioBusID = AudioBus->GetUniqueID();
		int32 NumChannels = (int32)AudioBus->AudioBusChannels + 1;

		UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
		check(AudioBusSubsystem);
		AudioBusSubsystem->StartAudioBus(Atom::FAudioBusKey(AudioBusID), AudioBus->GetPathName(), NumChannels, false);
	}
}

void UAtomRuntimeBlueprintLibrary::StopAudioBus(const UObject* WorldContextObject, UAtomAudioBus* AudioBus)
{
	if (!AudioBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Stop Audio Bus called with an invalid Audio Bus."));
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		uint32 AudioBusID = AudioBus->GetUniqueID();
		UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
		check(AudioBusSubsystem);
		AudioBusSubsystem->StopAudioBus(Atom::FAudioBusKey(AudioBusID));
	}
}

bool UAtomRuntimeBlueprintLibrary::IsAudioBusActive(const UObject* WorldContextObject, UAtomAudioBus* AudioBus)
{
	if (!AudioBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Is Audio Bus Active called with an invalid Audio Bus."));
		return false;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		uint32 AudioBusID = AudioBus->GetUniqueID();
		UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
		check(AudioBusSubsystem);
		return AudioBusSubsystem->IsAudioBusActive(Atom::FAudioBusKey(AudioBusID));
	}

	return false;
}

void UAtomRuntimeBlueprintLibrary::RegisterAudioBusToBus(const UObject* WorldContextObject, UAtomBus* AtomDspBus, UAtomAudioBus* AudioBus)
{
	if (!AtomDspBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("RegisterAudioBusToBus called with an invalid Atom DSP Bus."));
		return;
	}

	if (!AudioBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("RegisterAudioBusToBus called with an invalid Audio Bus."));
		return;
	}

	if (!IsInAtomThread())
	{
		//Send this over to the Atom thread, with the same settings
		FAtomThread::RunCommandOnAtomThread([WorldContextObject, AtomDspBus, AudioBus]()
		{
			RegisterAudioBusToBus(WorldContextObject, AtomDspBus, AudioBus);
		});

		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		Atom::FMixerSubmixPtr MixerSubmixPtr = AtomRuntime->GetSubmixInstance(AtomDspBus).Pin();

		if (MixerSubmixPtr.IsValid())
		{
			UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
			check(AudioBusSubsystem);

			const Atom::FAudioBusKey AudioBusKey = Atom::FAudioBusKey(AudioBus->GetUniqueID());
			MixerSubmixPtr->RegisterAudioBus(AudioBusKey, AudioBusSubsystem->AddPatchInputForAudioBus(AudioBusKey, AtomRuntime->GetRuntimeNumOutputFrames(), AudioBus->GetNumChannels()));
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("Submix not found in audio mixer."));
		}
	}
	else
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Audio mixer device not found."));
	}
}

void UAtomRuntimeBlueprintLibrary::UnregisterAudioBusFromBus(const UObject* WorldContextObject, UAtomBus* AtomDspBus, UAtomAudioBus* AudioBus)
{
	if (!AtomDspBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("UnregisterAudioBusToSubmix called with an invalid Submix."));
		return;
	}

	if (!AudioBus)
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("UnregisterAudioBusToSubmix called with an invalid Audio Bus."));
		return;
	}

	if (!IsInAtomThread())
	{
		//Send this over to the audio thread, with the same settings
		FAtomThread::RunCommandOnAtomThread([WorldContextObject, AtomDspBus, AudioBus]()
		{
			UnregisterAudioBusFromBus(WorldContextObject, AtomDspBus, AudioBus);
		});

		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		Atom::FMixerSubmixPtr MixerSubmixPtr = AtomRuntime->GetSubmixInstance(AtomDspBus).Pin();

		if (MixerSubmixPtr.IsValid())
		{
			const Atom::FAudioBusKey AudioBusKey(AudioBus->GetUniqueID());
			MixerSubmixPtr->UnregisterAudioBus(AudioBusKey);
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("Submix not found in audio mixer."));
		}
	}
	else
	{
		UE_LOG(LogCriWareAtomMixer, Error, TEXT("Audio mixer device not found."));
	}
}

void UAtomRuntimeBlueprintLibrary::AddGlobalVoicePool(const UObject* WorldContextObject,
	FName PoolName,
	FAtomVoicesSettings VoicesSettings,
	EAtomVoicePoolCodecType CodecType,
	EAtomVoicePoolStreamingType StreamingType)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->AddUserAtomVoicePool(PoolName, VoicesSettings, CodecType, StreamingType);
	}
}

void UAtomRuntimeBlueprintLibrary::RemoveGlobalVoicePool(const UObject* WorldContextObject, FName PoolName)
{
	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		AtomRuntime->RemoveUserAtomVoicePool(PoolName);
	}
}

void UAtomRuntimeBlueprintLibrary::StopCategoryByName(const UObject* WorldContextObject, FName CategoryName)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.StopCategoryByName"), STAT_AtomStopCategoryByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName]()
		{
			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				FCriWareApi::criAtomExCategory_StopByName(TCHAR_TO_UTF8(*CategoryName.ToString()));
			}
		}, GET_STATID(STAT_AtomStopCategoryByName));
	}
}

void UAtomRuntimeBlueprintLibrary::PauseCategoryByName(const UObject* WorldContextObject, FName CategoryName, bool bPause)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.PauseCategoryByName"), STAT_AtomPauseCategoryByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName, bPause]()
		{
			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				FCriWareApi::criAtomExCategory_PauseByName(TCHAR_TO_UTF8(*CategoryName.ToString()), (CriBool)bPause);
			}
		}, GET_STATID(STAT_AtomPauseCategoryByName));
	}
}

void UAtomRuntimeBlueprintLibrary::MuteCategoryByName(const UObject* WorldContextObject, FName CategoryName, bool bMute)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.MuteCategoryByName"), STAT_AtomMuteCategoryByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName, bMute]()
		{
			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				FCriWareApi::criAtomExCategory_MuteByName(TCHAR_TO_UTF8(*CategoryName.ToString()), (CriBool)bMute);
			}
		}, GET_STATID(STAT_AtomMuteCategoryByName));
	}
}

void UAtomRuntimeBlueprintLibrary::SoloCategoryByName(const UObject* WorldContextObject, FName CategoryName, bool bSolo, float MuteVolume)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SoloCategoryByName"), STAT_AtomSoloCategoryByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName, bSolo, MuteVolume]()
		{
			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				FCriWareApi::criAtomExCategory_SoloByName(TCHAR_TO_UTF8(*CategoryName.ToString()), (CriBool)bSolo, (CriFloat32)MuteVolume);
			}
		}, GET_STATID(STAT_AtomSoloCategoryByName));
	}
}

void UAtomRuntimeBlueprintLibrary::SetCategoryVolumeByName(const UObject* WorldContextObject, FName CategoryName, float VolumeMultiplier)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SetVolumeToCategoryByName"), STAT_AtomSetVolumeToCategoryByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName, VolumeMultiplier]()
		{
			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				FCriWareApi::criAtomExCategory_SetVolumeByName(TCHAR_TO_UTF8(*CategoryName.ToString()), (CriFloat32)VolumeMultiplier);
			}
		}, GET_STATID(STAT_AtomSetVolumeToCategoryByName));
	}
}

void UAtomRuntimeBlueprintLibrary::GetCategoryStatusByName(const UObject* WorldContextObject, FName CategoryName, const FOnAtomGetCategoryStatus OnGetStatus)
{
	if (CategoryName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.GetCategoryStatusByName"), STAT_AtomGetCategoryStatusByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, CategoryName, OnGetStatus]()
		{
			CriBool bIsPaused = false;
			CriBool bIsMuted = false;
			CriBool bIsSoloed = false;
			CriFloat32 Volume = 0.0f;
			CriSint32 NumCues = 0;

			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				std::string CategoryStr(TCHAR_TO_UTF8(*CategoryName.ToString()));
				bIsPaused = FCriWareApi::criAtomExCategory_IsPausedByName(CategoryStr.c_str());
				bIsMuted = FCriWareApi::criAtomExCategory_IsMutedByName(CategoryStr.c_str());
				bIsSoloed = FCriWareApi::criAtomExCategory_IsSoloedByName(CategoryStr.c_str());
				Volume = FCriWareApi::criAtomExCategory_GetVolumeByName(CategoryStr.c_str());
				NumCues = FCriWareApi::criAtomExCategory_GetNumCuePlayingCountByName(CategoryStr.c_str());
			}

			FAtomThread::RunCommandOnGameThread([=]()
			{
				OnGetStatus.ExecuteIfBound(CategoryName, (bool)bIsPaused, (bool)bIsMuted, (bool)bIsSoloed, (float)Volume, (int32)NumCues);
			});
		}, GET_STATID(STAT_AtomGetCategoryStatusByName));
	}
}

void UAtomRuntimeBlueprintLibrary::SetGameVariableValueByName(const UObject* WorldContextObject, FName GameVariableName, float Value)
{
	if (GameVariableName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SetGameVariableValueByName"), STAT_AtomSetGameVariableValueByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, GameVariableName, Value]()
		{
			// todo call a function of the runtime 
			// todo use IDs from current config to check if variable is valid
			FCriWareApi::criAtomEx_SetGameVariableByName(TCHAR_TO_UTF8(*GameVariableName.ToString()), Value);
		}, GET_STATID(STAT_AtomSetGameVariableValueByName));
	}
}

void UAtomRuntimeBlueprintLibrary::GetGameVariableValueByName(const UObject* WorldContextObject, FName GameVariableName, const FOnAtomGetGameVariableValue OnGetValue)
{
	if (GameVariableName.IsNone())
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.GetGameVariableValueByName"), STAT_AtomGetGameVariableValueByName, STATGROUP_AtomThreadCommands);

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, GameVariableName, OnGetValue]()
		{
			CriFloat32 Value = 0.0f;

			// todo call a function of the runtime
			if (AtomRuntime->IsActive())
			{
				Value = FCriWareApi::criAtomEx_GetGameVariableByName(TCHAR_TO_UTF8(*GameVariableName.ToString()));	
			}

			FAtomThread::RunCommandOnGameThread([=]()
			{
				OnGetValue.ExecuteIfBound(GameVariableName, (float)Value);
			});
		}, GET_STATID(STAT_AtomGetGameVariableValueByName));
	}
}

bool UAtomRuntimeBlueprintLibrary::MapOutputPortToRack(const UObject* WorldContextObject, FName OutputPortName, UAtomRackWithParentBase* AtomRack)
{
	if (OutputPortName.IsNone())
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("OutputPort name for mapping to rack is invalid."));
		return false;
	}

	if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		if (!AtomRuntime->GetAtomConfiguration())
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Atom configuration is not set for output port mapping."));
			return false;
		}

		return AtomRuntime->MapOutputPortToRack(OutputPortName, AtomRack);
	}

	return false;
}

/*
 * Atom playback based API (Experimental)
 *****************************************************************************/

void UAtomRuntimeBlueprintLibrary::GetAllPlaybacks(const UAtomComponent* AtomComponent, TArray<FAtomSoundPlayback>& Playbacks)
{
	if (!AtomComponent)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = AtomComponent->GetAtomRuntime())
	{
		const TArray<FAtomActiveSound*>& ActiveSounds = AtomRuntime->GetActiveSounds();
		for (auto& ActiveSound : ActiveSounds)
		{
			if (ActiveSound->GetAtomComponentID() == AtomComponent->GetAtomComponentID())
			{
				uint32 InstanceID = ActiveSound->GetInstanceID();
				for (auto& Pair : ActiveSound->GetPlaybackInstances())
				{
					if (auto PlaybackInstance = Pair.Value)
					{
						auto& Info = PlaybackInstance->SoundInfo;
						Playbacks.Add(FAtomSoundPlayback{
							Info.Type,
							Info.SoundID,
							Info.SoundName,
							Info.StartTrackIndex,
							Info.StartBlockIndex,
							ActiveSound->GetInstanceID(),
							Pair.Key
						});
					}
				}
			}
		}
	}
}

void UAtomRuntimeBlueprintLibrary::StopPlayback(const UAtomComponent* AtomComponent, FAtomSoundPlayback PlaybackToStop)
{
	if (!AtomComponent)
	{
		return;
	}

	if (FAtomRuntime* AtomRuntime = AtomComponent->GetAtomRuntime())
	{
		AtomRuntime->SendCommandToActiveSounds(AtomComponent->GetAtomComponentID(), [AtomRuntime, PlaybackToStop](FAtomActiveSound& ActiveSound)
		{
			if (ActiveSound.GetInstanceID() == PlaybackToStop.InstanceID)
			{
				if (auto PlaybackInstance = ActiveSound.FindPlaybackInstance(PlaybackToStop.PlaybackInstanceHash))
				{
					if (auto Source = AtomRuntime->GetSoundSource(PlaybackInstance))
					{
						Source->Stop();
					}
				}
			}
		});
	}
}

/*
 * UAtomUtilitiesBlueprintLibrary
 *****************************************************************************/

float UAtomUtilitiesBlueprintLibrary::ConvertLinearToDecibels(float InLinear, const FVector2D& InRange, float InFloor /* = SMALL_NUMBER */)
{
	const float Decibels = 20.0f * FMath::LogX(10.0f, FMath::Max(InLinear, InFloor));
	return FMath::GetMappedRangeValueClamped({ -100.0f , 0.0f }, InRange, Decibels);
}

float UAtomUtilitiesBlueprintLibrary::ConvertDecibelsToLinear(float InDecibels, const FVector2D& InRange)
{
	const float Decibels = FMath::GetMappedRangeValueClamped(InRange, { -100.0f , 0.0f }, InDecibels);
	return FMath::Pow(10.0f, Decibels * 0.05f);
}

float UAtomUtilitiesBlueprintLibrary::ConvertLinearToDecibelsSlower(float InLinear, const FVector2D& InRange)
{
	const float DecibelScale = FMath::Loge(InLinear * 1000.0f) * 0.144765f; // ln(x * 1000) / ln(1000) with 10^(60db / 20) = 1000
	return FMath::GetRangeValue(InRange, FMath::Max(DecibelScale, SMALL_NUMBER));
}

float UAtomUtilitiesBlueprintLibrary::ConvertDecibelsToLinearSlower(float InDecibels, const FVector2D& InRange)
{
	const float DecibelScale = FMath::GetRangePct(InRange, InDecibels);
	return FMath::Exp(6.907755f * DecibelScale) * 0.001f; // exp(x * ln(1000)) / 1000
}

float UAtomUtilitiesBlueprintLibrary::ConvertLinearToDecibelsSlowerFromFloor(float InLinear, const FVector2D& InRange, float InFloor /* = SMALL_NUMBER */)
{
	const float DecibelScale = FMath::Loge(InLinear / InFloor) / FMath::Loge(1.0f / InFloor);
	return FMath::GetRangeValue(InRange, FMath::Max(DecibelScale, SMALL_NUMBER));
}

float UAtomUtilitiesBlueprintLibrary::ConvertDecibelsToLinearSlowerFromFloor(float InDecibels, const FVector2D& InRange, float InFloor /* = SMALL_NUMBER */)
{
	const float DecibelScale = FMath::GetRangePct(InRange, InDecibels);
	return FMath::Exp(FMath::Loge(1.0f / InFloor) * DecibelScale) * InFloor;
}

float UAtomUtilitiesBlueprintLibrary::GetLogFrequencyClamped(const float InValue, const FVector2D& InDomain, const FVector2D& InRange)
{
	FVector2D ClampedRange = InRange.ClampAxes(SMALL_NUMBER, BIG_NUMBER);
	return Atom::GetLogFrequencyClamped(InValue, InDomain, ClampedRange);
}

float UAtomUtilitiesBlueprintLibrary::GetLinearFrequencyClamped(const float InValue, const FVector2D& InDomain, const FVector2D& InRange)
{
	FVector2D ClampedRange = InRange.ClampAxes(SMALL_NUMBER, BIG_NUMBER);
	return Atom::GetLinearFrequencyClamped(InValue, InDomain, InRange);
}
