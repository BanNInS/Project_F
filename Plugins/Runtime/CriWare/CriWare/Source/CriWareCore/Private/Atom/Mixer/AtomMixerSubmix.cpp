
#include "Atom/Mixer/AtomMixerSubmix.h"

#include "Async/Async.h"
#include "Misc/ScopeTryLock.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "DSP/FloatArrayMath.h"
#include "Algo/Accumulate.h"

#include "Atom/Mixer/AtomMixer.h"
#include "Atom/Mixer/AtomMixerTrace.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomThread.h"
#include "Atom/AtomEffectPreset.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"

/*static int32 AtomBypassAllBusEffectsCVar = 0;
FAutoConsoleVariableRef CVarAtomBypassAllBusEffects(
	TEXT("atom.BypassAllBusEffects"),
	AtomBypassAllBusEffectsCVar,
	TEXT("When set to 1, all bus effects will be bypassed.\n")
	TEXT("1: Bus Effects are disabled."),
	ECVF_Default);

static int32 AtomLogBusEnablementCVar = 0;
FAutoConsoleVariableRef CVarAtomLogBusEnablement(
	TEXT("atom.LogBusAutoDisable"),
	AtomLogBusEnablementCVar,
	TEXT("Enables logging of bus disable and enable state.\n")
	TEXT("1: Bus enablement logging is on. 0: Bus enablement/disablement logging is off."),
	ECVF_Default);
*/
// Define profiling categories for submixes. 
DEFINE_STAT(STAT_AtomMixerSubmixes);

#if ATOM_PROFILERTRACE_ENABLED

UE_TRACE_EVENT_BEGIN(CriWareAtom, SubmixHasActivity)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SubmixID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(bool, HasActivity)
UE_TRACE_EVENT_END()

#endif // ATOM_PROFILERTRACE_ENABLED

namespace Atom
{
	namespace MixerSubmixIntrinsics
	{
		using namespace Audio;

		static FSpectrumAnalyzerSettings::EFFTSize GetSpectrumAnalyzerFFTSize(EAtomFFTSize InFFTSize)
		{
			switch (InFFTSize)
			{
			case EAtomFFTSize::DefaultSize:	return FSpectrumAnalyzerSettings::EFFTSize::Default;
			case EAtomFFTSize::Min:			return FSpectrumAnalyzerSettings::EFFTSize::Min_64;
			case EAtomFFTSize::Small:		return FSpectrumAnalyzerSettings::EFFTSize::Small_256;
			case EAtomFFTSize::Medium:		return FSpectrumAnalyzerSettings::EFFTSize::Medium_512;
			case EAtomFFTSize::Large:		return FSpectrumAnalyzerSettings::EFFTSize::Large_1024;
			case EAtomFFTSize::VeryLarge:	return FSpectrumAnalyzerSettings::EFFTSize::VeryLarge_2048;
			case EAtomFFTSize::Max:			return FSpectrumAnalyzerSettings::EFFTSize::TestLarge_4096;
			default:						return FSpectrumAnalyzerSettings::EFFTSize::Default;
			}
		}

		static Audio::EWindowType GetWindowType(EAtomFFTWindowType InWindowType)
		{
			switch (InWindowType)
			{
			case EAtomFFTWindowType::None:		return Audio::EWindowType::None;
			case EAtomFFTWindowType::Hamming:	return Audio::EWindowType::Hamming;
			case EAtomFFTWindowType::Hann:		return Audio::EWindowType::Hann;
			case EAtomFFTWindowType::Blackman:	return Audio::EWindowType::Blackman;
			default:							return Audio::EWindowType::None;
			}
		}

		static FSpectrumBandExtractorSettings::EMetric GetExtractorMetric(EAtomSpectrumType InSpectrumType)
		{
			using EMetric = FSpectrumBandExtractorSettings::EMetric;

			switch (InSpectrumType)
			{
			case EAtomSpectrumType::MagnitudeSpectrum:	return EMetric::Magnitude;
			case EAtomSpectrumType::PowerSpectrum:		return EMetric::Power;
			case EAtomSpectrumType::Decibel:
			default:									return EMetric::Decibel;
			}
		}

		static ISpectrumBandExtractor::EBandType GetExtractorBandType(EAtomFFTPeakInterpolationMethod InMethod)
		{
			using EBandType = ISpectrumBandExtractor::EBandType;

			switch (InMethod)
			{
			case EAtomFFTPeakInterpolationMethod::NearestNeighbor:	return EBandType::NearestNeighbor;
			case EAtomFFTPeakInterpolationMethod::Linear:			return EBandType::Lerp;
			case EAtomFFTPeakInterpolationMethod::Quadratic:		return EBandType::Quadratic;
			case EAtomFFTPeakInterpolationMethod::ConstantQ:
			default:												return EBandType::ConstantQ;
			}
		}
	}

	namespace FMixerSubmix_NativeCallbacks
	{
		extern "C" void CRIAPI OnPreEffectFilter(void* Obj, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[])
		{
			// ADX do not call this function if bus is destroyed
			if (FMixerSubmix* Self = static_cast<FMixerSubmix*>(Obj))
			{
				Self->HandleNativeOnPreEffectFilter(Format, NumChannels, NumSamples, Data);
			}
		}

		extern "C" void CRIAPI OnPostEffectFilter(void* Obj, CriAtomPcmFormat Format, CriSint32 NumChannels, CriSint32 NumSamples, void* Data[])
		{
			// ADX do not call this function if bus is destroyed
			if (FMixerSubmix* Self = static_cast<FMixerSubmix*>(Obj))
			{
				Self->HandleNativeOnPostEffectFilter(Format, NumChannels, NumSamples, Data);
			}
		}
	}

	// Unique IDs for mixer submixes
	static uint32 GAtomSubmixMixerIDs = 0;

	FMixerSubmix::FMixerSubmix(FAtomRuntime* InAtomRuntime)
		: ID(GAtomSubmixMixerIDs++)
		, RackID(INDEX_NONE)
		, FilterBusIndex(INDEX_NONE)
		, AtomRuntime(InAtomRuntime)
		, NumChannels(0)
		, SampleRate(0)
		//, NumSamples(0)
		, CurrentOutputVolume(1.0f)
		, TargetOutputVolume(1.0f)
		, CurrentWetLevel(1.0f)
		, TargetWetLevel(1.0f)
		, CurrentDryLevel(0.0f)
		, TargetDryLevel(0.0f)
		, EnvelopeNumChannels(0)
		//, NumSubmixEffects(0)
		, bIsMainDspBus(false)
		//, bIsRecording(false)
		, bIsBackgroundMuted(false)
		//, bAutoDisable(true)
		//, bIsSilent(false)
		//, bIsCurrentlyDisabled(false)
		//, AutoDisableTime(0.1)
		//, SilenceTimeStartSeconds(-1.0)
		, bIsSpectrumAnalyzing(false)
	{
	}

	FMixerSubmix::~FMixerSubmix()
	{
		if (OwningAtomBusObject.IsValid())
		{
			if (const UAtomBus* AtomBus = Cast<const UAtomBus>(OwningAtomBusObject))
			{
				if (RackID != INDEX_NONE)
				{
					//FString BusName = AtomBus->GetBusName();
					//FCriWareApi::criAtomExAsrRack_SetBusFilterCallbackByName(RackID, TCHAR_TO_UTF8(*BusName), nullptr, nullptr, nullptr);

					int BusIndex = AtomBus->GetBusIndex();
					FCriWareApi::criAtomExAsrRack_SetBusFilterCallback(RackID, BusIndex, nullptr, nullptr, nullptr);
				}
			}
		}
		else if (RackID != INDEX_NONE && FilterBusIndex != INDEX_NONE)
		{
			FCriWareApi::criAtomExAsrRack_SetBusFilterCallback(RackID, FilterBusIndex, nullptr, nullptr, nullptr);
		}

		FilterBusIndex = INDEX_NONE;

		//ClearSoundEffectSubmixes();
		
		//if (RecoverRecordingOnShutdownCVar && OwningSubmixObject.IsValid() && bIsRecording)
		//{
		//	FString InterruptedFileName = TEXT("InterruptedRecording.wav");
		//	UE_LOG(LogAudioMixer, Warning, TEXT("Recording of Submix %s was interrupted. Saving interrupted recording as %s."), *(OwningSubmixObject->GetName()), *InterruptedFileName);
		//	if (const USoundSubmix* SoundSubmix = Cast<const USoundSubmix>(OwningSubmixObject))
		//	{
		//		USoundSubmix* MutableSubmix = const_cast<USoundSubmix*>(SoundSubmix);
		//		MutableSubmix->StopRecordingOutput(MixerDevice, EAudioRecordingExportType::WavFile, InterruptedFileName, FString());
		//	}
		//}
	}

	void FMixerSubmix::Init(const UAtomBus* InAtomBus, bool bAllowReInit)
	{
		check(IsInAtomThread());
		if (InAtomBus != nullptr)
		{
			SubmixName = InAtomBus->GetName();
			bIsMainDspBus = InAtomBus->IsMainBus();

			SampleRate = AtomRuntime->GetRackSampleRate(InAtomBus->GetRack());
			NumChannels = AtomRuntime->GetRackNumOutputChannels(InAtomBus->GetRack());

			// This is a first init and needs to be synchronous
			if (!OwningAtomBusObject.IsValid())
			{
				OwningAtomBusObject = InAtomBus;
				InitInternal();
			}
			// This is a re-init and needs to be thread safe
			else if (bAllowReInit)
			{
				check(OwningAtomBusObject == InAtomBus);
				SubmixCommand([this]()
				{
					InitInternal();
				});
			}
			/*else if (const USoundfieldSubmix* SoundfieldSubmix = Cast<const USoundfieldSubmix>(OwningSubmixObject))
			{
				ISoundfieldFactory* SoundfieldFactory = SoundfieldSubmix->GetSoundfieldFactoryForSubmix();
				const USoundfieldEncodingSettingsBase* EncodingSettings = SoundfieldSubmix->GetSoundfieldEncodingSettings();

				TArray<USoundfieldEffectBase*> Effects = SoundfieldSubmix->GetSoundfieldProcessors();
				SetupSoundfieldStreams(EncodingSettings, Effects, SoundfieldFactory);
			}*/
		}
	}

	void FMixerSubmix::InitInternal()
	{
		// Loop through the submix's presets and make new instances of effects in the same order as the presets
		//ClearSoundEffectSubmixes();

		// Copy any base data
		//bAutoDisable = OwningAtomBusObject->bAutoDisable;
		//AutoDisableTime = (double)OwningAtomBusObject->AutoDisableTime;
		//bIsSilent = false;
		//bIsCurrentlyDisabled = false;
		//SilenceTimeStartSeconds = -1.0;

		if (AtomRuntime->GetAtomModulationSystem())
		{
			VolumeMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
			WetLevelMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
			DryLevelMod.Init(AtomRuntime->GetAtomRuntimeID(), FName("Volume"), false /* bInIsBuffered */, true /* bInValueLinear */);
		}

		if (const UAtomBus* AtomBus = Cast<const UAtomBus>(OwningAtomBusObject))
		{
			VolumeModBaseDb = FMath::Clamp(AtomBus->OutputVolumeModulation.Value, ATOM_MIN_VOLUME_DECIBELS, 0.0f);
			WetModBaseDb = FMath::Clamp(AtomBus->WetLevelModulation.Value, ATOM_MIN_VOLUME_DECIBELS, 0.0f);
			DryModBaseDb = FMath::Clamp(AtomBus->DryLevelModulation.Value, ATOM_MIN_VOLUME_DECIBELS, 0.0f);

			TargetOutputVolume = Atom::ConvertToLinear(VolumeModBaseDb);
			TargetWetLevel = Atom::ConvertToLinear(WetModBaseDb);
			TargetDryLevel = Atom::ConvertToLinear(DryModBaseDb);

			CurrentOutputVolume = TargetOutputVolume;
			CurrentDryLevel = TargetDryLevel;
			CurrentWetLevel = TargetWetLevel;

			{
				TSet<TObjectPtr<UAtomModulatorBase>> VolumeModulator = AtomBus->OutputVolumeModulation.Modulators;
				TSet<TObjectPtr<UAtomModulatorBase>> WetLevelModulator = AtomBus->WetLevelModulation.Modulators;
				TSet<TObjectPtr<UAtomModulatorBase>> DryLevelModulator = AtomBus->DryLevelModulation.Modulators;

				// Queue this up to happen after submix init, when the mixer device has finished being added to the device manager
				SubmixCommand([this, VolMod = MoveTemp(VolumeModulator), WetMod = MoveTemp(WetLevelModulator), DryMod = MoveTemp(DryLevelModulator)]() mutable
				{
					UpdateModulationSettings(VolMod, WetMod, DryMod);
				});

				//UpdateModulationSettings(MoveTemp(VolumeModulator), MoveTemp(WetLevelModulator), MoveTemp(DryLevelModulator));
			}

			// TEST DSP
			// 
			// Attach the Filter Functions that permit custom DSP operations over Atom DSP buses.
			//Audio::FPlateReverbFastSettings NewSettings;

			//NewSettings.bEnableEarlyReflections = true;
			//NewSettings.bEnableLateReflections = true;

			//// Early Reflections
			//NewSettings.EarlyReflections.Gain = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 3.16f }, FVector2f{ 0.0f, 1.0f }, 0.03f);
			//NewSettings.EarlyReflections.PreDelayMsec = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 0.3f }, FVector2f{ 0.0f, 300.0f }, 0.007f);
			//NewSettings.EarlyReflections.Bandwidth = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 1.0f }, FVector2f{ 0.0f, 1.0f }, 1.0f - 0.89f);

			//// LateReflections
			//NewSettings.LateReflections.LateDelayMsec = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 0.1f }, FVector2f{ 0.0f, 100.0f }, 0.011f);
			//NewSettings.LateReflections.LateGainDB = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 1.0f }, FVector2f{ 0.0f, 1.0f }, 0.32f);
			//NewSettings.LateReflections.Bandwidth = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 1.0f }, FVector2f{ 0.1f, 0.6f }, 0.994f);
			//NewSettings.LateReflections.Diffusion = FMath::GetMappedRangeValueClamped(FVector2f{ 0.05f, 1.0f }, FVector2f{ 0.0f, 0.95f }, 1.0f);
			//NewSettings.LateReflections.Dampening = FMath::GetMappedRangeValueClamped(FVector2f{ 0.05f, 1.95f }, FVector2f{ 0.0f, 0.999f }, 0.83f);
			//NewSettings.LateReflections.Density = FMath::GetMappedRangeValueClamped(FVector2f{ 0.0f, 0.95f }, FVector2f{ 0.06f, 1.0f }, 1.0f);

			//// Use mapping function to get decay time in seconds to internal linear decay scale value
			//const float DecayValue = 1.49f;
			//NewSettings.LateReflections.Decay = DecayValue;

			//// Convert to db
			//NewSettings.LateReflections.LateGainDB = Audio::ConvertToDecibels(NewSettings.LateReflections.LateGainDB);
			//Reverb = MakeShared<Audio::FPlateReverbFast, ESPMode::ThreadSafe>(GetSampleRate(), 512, NewSettings);

			if (UAtomRackBase* Rack = Cast<UAtomRackBase>(AtomBus->GetRack()))
			{
				RackID = AtomRuntime->GetAsrRackId(Rack);
				if (RackID != INDEX_NONE)
				{
					// want to check format and disable DSP processing if not Float32.
					//if (FCriWareApi::)
					//{
					//	UE_LOG(LogCriWareAtomMixer, Warning, TEXT("DSP operations are only available for float32 PCM"));
					//	return;
					//}

					//FString BusName = AtomBus->GetBusName();
					//FCriWareApi::criAtomExAsrRack_SetBusFilterCallbackByName(RackID, TCHAR_TO_UTF8(*BusName), FMixerSubmix_NativeCallbacks::OnPreEffectFilter, FMixerSubmix_NativeCallbacks::OnPostEffectFilter, this);

					int BusIndex = AtomBus->GetBusIndex();
					FCriWareApi::criAtomExAsrRack_SetBusFilterCallback(RackID, BusIndex, FMixerSubmix_NativeCallbacks::OnPreEffectFilter, FMixerSubmix_NativeCallbacks::OnPostEffectFilter, this);
					FilterBusIndex = BusIndex;
				}
				else
				{
					UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Invalid Rack '%s' for Bus '%s'."), *Rack->GetFName().ToString(), *AtomBus->GetFName().ToString());
				}
			}
			else
			{
				UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Invalid Bus '%s'."), *AtomBus->GetFName().ToString());
			}

		}
		else
		{
			// If we've arrived here, we couldn't identify the type of the submix we're initializing.
			checkNoEntry();
		}
	}

	void FMixerSubmix::DownmixBuffer(const int32 InChannels, const FAlignedFloatBuffer& InBuffer, const int32 OutChannels, FAlignedFloatBuffer& OutNewBuffer)
	{
		FAlignedFloatBuffer MixdownGainsMap;
		FAtomRuntime::Get2DChannelMap(false, InChannels, OutChannels, false, MixdownGainsMap);
		Audio::DownmixBuffer(InChannels, OutChannels, InBuffer, OutNewBuffer, MixdownGainsMap.GetData());
	}

	void FMixerSubmix::RegisterAudioBus(const Atom::FAudioBusKey& InAudioBusKey, Audio::FPatchInput&& InPatchInput)
	{
		check(IsInAtomThread());

		SubmixCommand([this, InAudioBusKey, InPatchInput = MoveTemp(InPatchInput)]()
		{
			int32 NumBusChannels = AtomRuntime->GetSourceManager()->GetAudioBusNumChannels(InAudioBusKey);

			if (!AudioBuses.Contains(InAudioBusKey))
			{
				AudioBuses.Emplace(InAudioBusKey, { InPatchInput, NumBusChannels });
			}
			else
			{
				auto& [Patch, PatchNumChannels] = AudioBuses[InAudioBusKey];
				Patch = InPatchInput;
				PatchNumChannels = NumBusChannels;
			}
		});
	}

	void FMixerSubmix::UnregisterAudioBus(const Atom::FAudioBusKey& InAudioBusKey)
	{
		check(IsInAtomThread());

		SubmixCommand([this, InAudioBusKey]()
		{
			if (AudioBuses.Contains(InAudioBusKey))
			{
				AudioBuses.Remove(InAudioBusKey);
			}
		});
	}

	void FMixerSubmix::SetBackgroundMuted(bool bInMuted)
	{
		SubmixCommand([this, bInMuted]()
		{
			bIsBackgroundMuted = bInMuted;
		});
	}

	void FMixerSubmix::MixBufferDownToMono(const FAlignedFloatBuffer& InBuffer, int32 NumInputChannels, FAlignedFloatBuffer& OutBuffer)
	{
		check(NumInputChannels > 0);

		int32 NumFrames = InBuffer.Num() / NumInputChannels;
		OutBuffer.Reset();
		OutBuffer.AddZeroed(NumFrames);

		const float* InData = InBuffer.GetData();
		float* OutData = OutBuffer.GetData();

		const float GainFactor = 1.0f / FMath::Sqrt((float)NumInputChannels);

		for (int32 FrameIndex = 0; FrameIndex < NumFrames; FrameIndex++)
		{
			for (int32 ChannelIndex = 0; ChannelIndex < NumInputChannels; ChannelIndex++)
			{
				const int32 InputIndex = FrameIndex * NumInputChannels + ChannelIndex;
				OutData[FrameIndex] += InData[InputIndex] * GainFactor;
			}
		}
	}

	void FMixerSubmix::PumpCommandQueue()
	{
		TFunction<void()> Command;
		while (CommandQueue.Dequeue(Command))
		{
			Command();
		}
	}

	void FMixerSubmix::SubmixCommand(TFunction<void()> Command)
	{
		CommandQueue.Enqueue(MoveTemp(Command));
	}

	bool FMixerSubmix::IsValid() const
	{
		return OwningAtomBusObject.IsValid();
	}

	// this is from server thread
	void FMixerSubmix::ProcessAudio()
	{
		ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		PumpCommandQueue();
	}

	//this from AtomRuntime game thread

	void FMixerSubmix::Update()
	{
		//ATOM_MIXER_CHECK_GAME_THREAD(AtomRuntime);

		// this is originally on render thread

		// Update Wet Level using modulator
		if (AtomRuntime->GetAtomModulationSystem())
		{
			WetLevelMod.ProcessControl(WetModBaseDb);
			TargetWetLevel = WetLevelMod.GetValue();
		}
		else
		{
			TargetWetLevel = Audio::ConvertToLinear(WetModBaseDb);
		}

		TargetWetLevel *= WetLevelModifier;

		// Update Dry Level using modulator
		if (AtomRuntime->GetAtomModulationSystem())
		{
			DryLevelMod.ProcessControl(DryModBaseDb);
			TargetDryLevel = DryLevelMod.GetValue();
		}
		else
		{
			TargetDryLevel = Atom::ConvertToLinear(DryModBaseDb);
		}

		TargetDryLevel *= DryLevelModifier;

		// this is originally on render thread

		// Update output volume using modulator
		if (AtomRuntime->GetAtomModulationSystem())
		{
			VolumeMod.ProcessControl(VolumeModBaseDb);
			TargetOutputVolume = VolumeMod.GetValue();
		}
		else
		{
			TargetOutputVolume = Atom::ConvertToLinear(VolumeModBaseDb);
		}

		TargetOutputVolume *= VolumeModifier;

		// Now apply the output volume (directly to Atom SDK volume)
		if (!FMath::IsNearlyEqual(TargetOutputVolume, CurrentOutputVolume) || !FMath::IsNearlyEqual(CurrentOutputVolume, 1.0f))
		{
			CurrentOutputVolume = TargetOutputVolume;

			if (const UAtomBus* AtomBus = Cast<const UAtomBus>(OwningAtomBusObject))
			{
				AtomRuntime->SetBusVolume(AtomBus, CurrentOutputVolume);
			}
		}

		// Get rendered samples count from ASR rack if requested
		if (bIsMainDspBus && RackID != INDEX_NONE)
		{
			if (OnAtomRackRenderedSamples.IsBound())
			{
				CriSint64 OutNumSample = 0;
				CriSint32 OutSampleRate = 0;
				FCriWareApi::criAtomExAsrRack_GetNumRenderedSamples(RackID, &OutNumSample, &OutSampleRate);

				OnAtomRackRenderedSamples.Broadcast((int32)RackID, (int64)OutNumSample, (int32)OutSampleRate);
			}

			if (OnAtomRackPerformanceMonitorResult.IsBound())
			{
				CriAtomExAsrRackPerformanceInfo PerformanceInfo;
				FCriWareApi::criAtomExAsrRack_GetPerformanceInfo(RackID, &PerformanceInfo);

				PerformanceMonitorResult = {
					(int32)PerformanceInfo.process_count,
					(int32)PerformanceInfo.last_process_time,
					(int32)PerformanceInfo.max_process_time,
					(int32)PerformanceInfo.average_process_time,
					(int32)PerformanceInfo.last_process_interval,
					(int32)PerformanceInfo.max_process_interval,
					(int32)PerformanceInfo.average_process_interval,
					(int32)PerformanceInfo.last_process_samples,
					(int32)PerformanceInfo.max_process_samples,
					(int32)PerformanceInfo.average_process_samples
				};

				OnAtomRackPerformanceMonitorResult.Broadcast(PerformanceMonitorResult);
			}
		}
	}

	int32 FMixerSubmix::GetSampleRate() const
	{
		return SampleRate;
	}

	int32 FMixerSubmix::GetNumOutputChannels() const
	{
		return NumChannels;
	}

	// from pre-effects filter on ASR threads
	void FMixerSubmix::ProcessAudioPreEffects(FAlignedFloatBuffer& InOutBuffer, int InNumChannels)
	{
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		DryChannelBuffer.Reset();

		// update dry level modulator -> moved to ProcessAudio

		// Check if we need to allocate a dry buffer. This is stored here before effects processing. We mix in with wet buffer after effects processing.
		if (!FMath::IsNearlyEqual(TargetDryLevel, CurrentDryLevel) || !FMath::IsNearlyZero(CurrentDryLevel))
		{
			DryChannelBuffer.Append(InOutBuffer);
		}

		// TEST DSP
		//if (Reverb.IsValid())
		//{
		//	Reverb->ProcessAudio(InOutBuffer, 2, InOutBuffer, 2);
		//}
	}

	// from post-effects filter on ASR thread
	void FMixerSubmix::ProcessAudioPostEffects(FAlignedFloatBuffer& InOutBuffer, int InNumChannels)
	{
		//ATOM_MIXER_CHECK_AUDIO_THREAD(AtomRuntime);

		float* BufferPtr = InOutBuffer.GetData();

		// Apply the wet level here after processing effects. 
		if (!FMath::IsNearlyEqual(TargetWetLevel, CurrentWetLevel) || !FMath::IsNearlyEqual(CurrentWetLevel, 1.0f))
		{
			if (FMath::IsNearlyEqual(TargetWetLevel, CurrentWetLevel))
			{
				Audio::ArrayMultiplyByConstantInPlace(InOutBuffer, TargetWetLevel);
			}
			else
			{
				Audio::ArrayFade(InOutBuffer, CurrentWetLevel, TargetWetLevel);
				CurrentWetLevel = TargetWetLevel;
			}
		}

		// Mix in the dry channel buffer
		if (DryChannelBuffer.Num() > 0)
		{
			// If we've already set the volume, only need to multiply by constant
			if (FMath::IsNearlyEqual(TargetDryLevel, CurrentDryLevel))
			{
				Audio::ArrayMultiplyByConstantInPlace(DryChannelBuffer, TargetDryLevel);
			}
			else
			{
				// To avoid popping, we do a fade on the buffer to the target volume
				Audio::ArrayFade(DryChannelBuffer, CurrentDryLevel, TargetDryLevel);
				CurrentDryLevel = TargetDryLevel;
			}
			Audio::ArrayMixIn(DryChannelBuffer, InOutBuffer);
		}

		// If we're muted, memzero the buffer. Note we are still doing all the work to maintain buffer state between mutings.
		if (bIsBackgroundMuted)
		{
			FMemory::Memzero((void*)BufferPtr, sizeof(float) * InOutBuffer.Num());
		}

		// If we are recording, Add out buffer to the RecordingData buffer:
		//{
		//	FScopeLock ScopedLock(&RecordingCriticalSection);
		//	if (bIsRecording)
		//	{
		//		// TODO: Consider a scope lock between here and OnStopRecordingOutput.
		//		RecordingData.Append((float*)BufferPtr, NumSamples);
		//	}
		//}

		// If spectrum analysis is enabled for this submix, downmix the resulting audio
		// and push it to the spectrum analyzer.
		{
			FScopeTryLock TryLock(&SpectrumAnalyzerCriticalSection);

			if (TryLock.IsLocked() && SpectrumAnalyzer.IsValid())
			{
				MixBufferDownToMono(InOutBuffer, InNumChannels, MonoMixBuffer);
				SpectrumAnalyzer->PushAudio(MonoMixBuffer.GetData(), MonoMixBuffer.Num());
				SpectrumAnalyzer->PerformAsyncAnalysisIfPossible(true);
			}
		}

		// Perform any envelope following if we're told to do so
		if (bIsEnvelopeFollowing)
		{
			const int32 BufferSamples = InOutBuffer.Num();
			const float* AudioBufferPtr = InOutBuffer.GetData();

			// Perform envelope following per channel
			FScopeLock EnvelopeScopeLock(&EnvelopeCriticalSection);
			FMemory::Memset(EnvelopeValues, sizeof(float) * AUDIO_MIXER_MAX_OUTPUT_CHANNELS);

			if (InNumChannels > 0)
			{
				const int32 NumFrames = BufferSamples / InNumChannels;
				if (EnvelopeFollower.GetNumChannels() != InNumChannels)
				{
					EnvelopeFollower.SetNumChannels(InNumChannels);
				}

				EnvelopeFollower.ProcessAudio(AudioBufferPtr, NumFrames);
				const TArray<float>& EnvValues = EnvelopeFollower.GetEnvelopeValues();

				check(EnvValues.Num() == InNumChannels);

				FMemory::Memcpy(EnvelopeValues, EnvValues.GetData(), sizeof(float) * InNumChannels);
				Audio::ArrayClampInPlace(MakeArrayView(EnvelopeValues, InNumChannels), 0.f, 1.f);
			}

			EnvelopeNumChannels = InNumChannels;
		}

		// update output volume modulator -> moved to ProcessAudio

		// Now apply the output volume 
		// -> moved to ProcessAudio: the fader control is in AtomEx sdk
		/*if (!FMath::IsNearlyEqual(TargetOutputVolume, CurrentOutputVolume) || !FMath::IsNearlyEqual(CurrentOutputVolume, 1.0f))
		{
			// If we've already set the output volume, only need to multiply by constant
			if (FMath::IsNearlyEqual(TargetOutputVolume, CurrentOutputVolume))
			{
				Audio::ArrayMultiplyByConstantInPlace(InputBuffer, TargetOutputVolume);
			}
			else
			{
				// To avoid popping, we do a fade on the buffer to the target volume
				Audio::ArrayFade(InputBuffer, CurrentOutputVolume, TargetOutputVolume);
				CurrentOutputVolume = TargetOutputVolume;
			}
		}*/

		// send audio for post fader audio bus sends 
		// TODO: we are not post fader so we need to get volume applyed in Atom (asrrack_GetVolume) of the bus and apply to a buffer copy.
		SendAudioToRegisteredAudioBuses(InOutBuffer);

		// Mix the audio buffer of this submix with the audio buffer of the output buffer (i.e. with other submixes)
		//Audio::ArrayMixIn(InputBuffer, OutAudioBuffer);

#if ATOM_PROFILERTRACE_ENABLED
		if (OwningAtomBusObject.IsValid())
		{
			// Extremely cheap silent buffer detection: as soon as we hit a sample which isn't silent, we flag we're not silent
			bool bIsNowSilent = true;
			int i = 0;
#if PLATFORM_ENABLE_VECTORINTRINSICS
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			int SimdNum = InOutBuffer.Num() & 0xFFFFFFF0;
			for (; i < SimdNum; i += 16)
			{
				VectorRegister4x4Float Samples = VectorLoad16(&InOutBuffer[i]);
				if (   VectorAnyGreaterThan(VectorAbs(Samples.val[0]), GlobalVectorConstants::SmallNumber)
					|| VectorAnyGreaterThan(VectorAbs(Samples.val[1]), GlobalVectorConstants::SmallNumber)
					|| VectorAnyGreaterThan(VectorAbs(Samples.val[2]), GlobalVectorConstants::SmallNumber)
					|| VectorAnyGreaterThan(VectorAbs(Samples.val[3]), GlobalVectorConstants::SmallNumber))
				{
					bIsNowSilent = false;
					i = INT_MAX;
					break;
				}
			}
#else
			int SimdNum = InOutBuffer.Num() & 0xFFFFFFF0;
			for (; i < SimdNum; i += 16)
			{
				const VectorRegister4Float Samples0 = VectorLoad(&InOutBuffer[i]);
				const VectorRegister4Float Samples1 = VectorLoad(&InOutBuffer[i + 4]);
				const VectorRegister4Float Samples2 = VectorLoad(&InOutBuffer[i + 8]);
				const VectorRegister4Float Samples3 = VectorLoad(&InOutBuffer[i + 12]);

				if (VectorAnyGreaterThan(VectorAbs(Samples0), GlobalVectorConstants::SmallNumber) ||
					VectorAnyGreaterThan(VectorAbs(Samples1), GlobalVectorConstants::SmallNumber) ||
					VectorAnyGreaterThan(VectorAbs(Samples2), GlobalVectorConstants::SmallNumber) ||
					VectorAnyGreaterThan(VectorAbs(Samples3), GlobalVectorConstants::SmallNumber))
				{
					bIsNowSilent = false;
					i = INT_MAX;
					break;
				}
			}
#endif
#endif
			// Finish to the end of the buffer or check each sample if vector intrinsics are disabled
			for (; i < InOutBuffer.Num(); ++i)
			{
				// As soon as we hit a non-silent sample, we're not silent
				if (FMath::Abs(InOutBuffer[i]) > SMALL_NUMBER)
				{
					bIsNowSilent = false;
					break;
				}
			}

			UE_TRACE_LOG(CriWareAtom, SubmixHasActivity, AtomChannel)
				<< SubmixHasActivity.RuntimeID(AtomRuntime->GetAtomRuntimeID())
				<< SubmixHasActivity.SubmixID(GetTypeHash(OwningAtomBusObject->GetPathName()))
				<< SubmixHasActivity.Timestamp(FPlatformTime::Cycles64())
				<< SubmixHasActivity.HasActivity(!bIsNowSilent);

			// Also send an event for main bus as rack
			if (bIsMainDspBus && RackID != INDEX_NONE)
			{
				UE_TRACE_LOG(CriWareAtom, SubmixHasActivity, AtomChannel)
					<< SubmixHasActivity.RuntimeID(AtomRuntime->GetAtomRuntimeID())
					<< SubmixHasActivity.SubmixID(GetTypeHash(OwningAtomBusObject->GetRack()->GetPathName()))
					<< SubmixHasActivity.Timestamp(FPlatformTime::Cycles64())
					<< SubmixHasActivity.HasActivity(!bIsNowSilent);
			}
		}
#endif
	}

	void FMixerSubmix::SendAudioToRegisteredAudioBuses(FAlignedFloatBuffer& OutAudioBuffer)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FMixerSubmix::SendAudioToRegisteredAudioBuses);

		for (auto& [AudioBusKey, Elem] : AudioBuses)
		{
			auto& [PatchInput, NumBusChannels] = Elem;

			if (NumChannels != NumBusChannels)
			{
				DownmixedBuffer.SetNumUninitialized((OutAudioBuffer.Num() / NumChannels) * NumBusChannels);
				DownmixBuffer(NumChannels, OutAudioBuffer, NumBusChannels, DownmixedBuffer);
				PatchInput.PushAudio(DownmixedBuffer.GetData(), DownmixedBuffer.Num());
			}
			else
			{
				PatchInput.PushAudio(OutAudioBuffer.GetData(), OutAudioBuffer.Num());
			}

#if ATOM_MIXER_ENABLE_PCM_TRACE
			/*UE_LOG(LogTemp, Error, TEXT("SUBMIX->BUS[%d] %d %d [%f %f] [%f %f]"), AudioBusKey.ObjectId, OutAudioBuffer.Num() / NumChannels, OutAudioBuffer.Num()
				, OutAudioBuffer.GetData()[0]
				, OutAudioBuffer.GetData()[1]
				, OutAudioBuffer.GetData()[2]
				, OutAudioBuffer.GetData()[3]);*/
#endif
		}
	}

	void FMixerSubmix::StartEnvelopeFollowing(int32 AttackTime, int32 ReleaseTime)
	{
		if (!bIsEnvelopeFollowing)
		{
			Audio::FEnvelopeFollowerInitParams EnvelopeFollowerInitParams;
			EnvelopeFollowerInitParams.SampleRate = GetSampleRate();
			EnvelopeFollowerInitParams.NumChannels = GetNumOutputChannels();
			EnvelopeFollowerInitParams.AttackTimeMsec = static_cast<float>(AttackTime);
			EnvelopeFollowerInitParams.ReleaseTimeMsec = static_cast<float>(ReleaseTime);
			EnvelopeFollower.Init(EnvelopeFollowerInitParams);

			// Zero out any previous envelope values which may have been in the array before starting up
			for (int32 ChannelIndex = 0; ChannelIndex < ATOM_MAX_DSP_CHANNELS; ++ChannelIndex)
			{
				EnvelopeValues[ChannelIndex] = 0.0f;
			}

			bIsEnvelopeFollowing = true;
		}
	}

	void FMixerSubmix::StopEnvelopeFollowing()
	{
		bIsEnvelopeFollowing = false;
	}

	void FMixerSubmix::AddEnvelopeFollowerDelegate(const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP)
	{
		OnAtomBusEnvelope.AddUnique(OnAtomBusEnvelopeBP);
	}

	void FMixerSubmix::RemoveEnvelopeFollowerDelegate(const FOnAtomBusEnvelopeBP& OnAtomBusEnvelopeBP)
	{
		OnAtomBusEnvelope.Remove(OnAtomBusEnvelopeBP);
	}

	void FMixerSubmix::AddSpectralAnalysisDelegate(const FAtomSoundSpectrumAnalyzerDelegateSettings& InDelegateSettings, const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP)
	{
		FSpectrumAnalysisDelegateInfo NewDelegateInfo;

		NewDelegateInfo.LastUpdateTime = -1.0f;
		NewDelegateInfo.DelegateSettings = InDelegateSettings;
		NewDelegateInfo.DelegateSettings.UpdateRate = FMath::Clamp(NewDelegateInfo.DelegateSettings.UpdateRate, 1.0f, 30.0f);
		NewDelegateInfo.UpdateDelta = 1.0f / NewDelegateInfo.DelegateSettings.UpdateRate;

		NewDelegateInfo.OnAtomBusSpectralAnalysis.AddUnique(OnAtomBusSpectralAnalysisBP);

		{
			FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);

			SpectralAnalysisDelegates.Add(MoveTemp(NewDelegateInfo));
		}
	}

	void FMixerSubmix::RemoveSpectralAnalysisDelegate(const FOnAtomBusSpectralAnalysisBP& OnAtomBusSpectralAnalysisBP)
	{
		FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);

		for (FSpectrumAnalysisDelegateInfo& Info : SpectralAnalysisDelegates)
		{
			if (Info.OnAtomBusSpectralAnalysis.Contains(OnAtomBusSpectralAnalysisBP))
			{
				Info.OnAtomBusSpectralAnalysis.Remove(OnAtomBusSpectralAnalysisBP);
			}
		}

		SpectralAnalysisDelegates.RemoveAllSwap([](FSpectrumAnalysisDelegateInfo& Info)
		{
			return !Info.OnAtomBusSpectralAnalysis.IsBound();
		});
	}

	void FMixerSubmix::StartSpectrumAnalysis(const FAtomSoundSpectrumAnalyzerSettings& InSettings)
	{
		ensure(IsInAtomThread());

		using namespace MixerSubmixIntrinsics;
		using EMetric = FSpectrumBandExtractorSettings::EMetric;
		using EBandType = ISpectrumBandExtractor::EBandType;

		bIsSpectrumAnalyzing = true;

		SpectrumAnalyzerSettings = InSettings;

		FSpectrumAnalyzerSettings AudioSpectrumAnalyzerSettings;

		AudioSpectrumAnalyzerSettings.FFTSize = GetSpectrumAnalyzerFFTSize(SpectrumAnalyzerSettings.FFTSize);
		AudioSpectrumAnalyzerSettings.WindowType = GetWindowType(SpectrumAnalyzerSettings.WindowType);
		AudioSpectrumAnalyzerSettings.HopSize = SpectrumAnalyzerSettings.HopSize;

		EMetric Metric = GetExtractorMetric(SpectrumAnalyzerSettings.SpectrumType);
		EBandType BandType = GetExtractorBandType(SpectrumAnalyzerSettings.InterpolationMethod);

		{
			FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);
			SpectrumAnalyzer = MakeShared<FAsyncSpectrumAnalyzer, ESPMode::ThreadSafe>(AudioSpectrumAnalyzerSettings, GetSampleRate());


			for (FSpectrumAnalysisDelegateInfo& DelegateInfo : SpectralAnalysisDelegates)
			{
				FSpectrumBandExtractorSettings ExtractorSettings;

				ExtractorSettings.Metric = Metric;
				ExtractorSettings.DecibelNoiseFloor = DelegateInfo.DelegateSettings.DecibelNoiseFloor;
				ExtractorSettings.bDoNormalize = DelegateInfo.DelegateSettings.bDoNormalize;
				ExtractorSettings.bDoAutoRange = DelegateInfo.DelegateSettings.bDoAutoRange;
				ExtractorSettings.AutoRangeReleaseTimeInSeconds = DelegateInfo.DelegateSettings.AutoRangeReleaseTime;
				ExtractorSettings.AutoRangeAttackTimeInSeconds = DelegateInfo.DelegateSettings.AutoRangeAttackTime;

				DelegateInfo.SpectrumBandExtractor = Audio::ISpectrumBandExtractor::CreateSpectrumBandExtractor(ExtractorSettings);

				if (DelegateInfo.SpectrumBandExtractor.IsValid())
				{
					for (const FAtomBusSpectralAnalysisBandSettings& BandSettings : DelegateInfo.DelegateSettings.BandSettings)
					{
						Audio::ISpectrumBandExtractor::FBandSettings NewExtractorBandSettings;
						NewExtractorBandSettings.Type = BandType;
						NewExtractorBandSettings.CenterFrequency = BandSettings.BandFrequency;
						NewExtractorBandSettings.QFactor = BandSettings.QFactor;

						DelegateInfo.SpectrumBandExtractor->AddBand(NewExtractorBandSettings);

						FSpectralAnalysisBandInfo NewBand;

						Audio::FInlineEnvelopeFollowerInitParams EnvelopeFollowerInitParams;
						EnvelopeFollowerInitParams.SampleRate = DelegateInfo.DelegateSettings.UpdateRate;
						EnvelopeFollowerInitParams.AttackTimeMsec = static_cast<float>(BandSettings.AttackTimeMsec);
						EnvelopeFollowerInitParams.ReleaseTimeMsec = static_cast<float>(BandSettings.ReleaseTimeMsec);
						NewBand.EnvelopeFollower.Init(EnvelopeFollowerInitParams);

						DelegateInfo.SpectralBands.Add(NewBand);
					}
				}
			}
		}
	}

	void FMixerSubmix::StopSpectrumAnalysis()
	{
		ensure(IsInAtomThread());

		FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);
		bIsSpectrumAnalyzing = false;
		SpectrumAnalyzer.Reset();
	}

	void FMixerSubmix::GetMagnitudeForFrequencies(const TArray<float>& InFrequencies, TArray<float>& OutMagnitudes)
	{
		FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);

		if (SpectrumAnalyzer.IsValid())
		{
			using EMethod = Audio::FSpectrumAnalyzer::EPeakInterpolationMethod;

			EMethod Method;

			switch (SpectrumAnalyzerSettings.InterpolationMethod)
			{
			case EAtomFFTPeakInterpolationMethod::NearestNeighbor: Method = EMethod::NearestNeighbor; break;
			case EAtomFFTPeakInterpolationMethod::Linear: Method = EMethod::Linear; break;
			case EAtomFFTPeakInterpolationMethod::Quadratic: Method = EMethod::Quadratic; break;
			default: Method = EMethod::Linear; break;
			}

			OutMagnitudes.Reset();
			OutMagnitudes.AddUninitialized(InFrequencies.Num());

			SpectrumAnalyzer->LockOutputBuffer();
			for (int32 Index = 0; Index < InFrequencies.Num(); Index++)
			{
				OutMagnitudes[Index] = SpectrumAnalyzer->GetMagnitudeForFrequency(InFrequencies[Index], Method);
			}
			SpectrumAnalyzer->UnlockOutputBuffer();
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Call StartSpectrumAnalysis before calling GetMagnitudeForFrequencies."));
		}
	}

	void FMixerSubmix::GetPhaseForFrequencies(const TArray<float>& InFrequencies, TArray<float>& OutPhases)
	{
		FScopeLock SpectrumAnalyzerLock(&SpectrumAnalyzerCriticalSection);

		if (SpectrumAnalyzer.IsValid())
		{
			using EMethod = Audio::FSpectrumAnalyzer::EPeakInterpolationMethod;

			EMethod Method;

			switch (SpectrumAnalyzerSettings.InterpolationMethod)
			{
			case EAtomFFTPeakInterpolationMethod::NearestNeighbor: Method = EMethod::NearestNeighbor; break;
			case EAtomFFTPeakInterpolationMethod::Linear: Method = EMethod::Linear; break;
			case EAtomFFTPeakInterpolationMethod::Quadratic: Method = EMethod::Quadratic; break;
			default: Method = EMethod::Linear; break;
			}

			OutPhases.Reset();
			OutPhases.AddUninitialized(InFrequencies.Num());

			SpectrumAnalyzer->LockOutputBuffer();
			for (int32 Index = 0; Index < InFrequencies.Num(); Index++)
			{
				OutPhases[Index] = SpectrumAnalyzer->GetPhaseForFrequency(InFrequencies[Index], Method);
			}
			SpectrumAnalyzer->UnlockOutputBuffer();
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Call StartSpectrumAnalysis before calling GetMagnitudeForFrequencies."));
		}
	}

	void FMixerSubmix::StartLevelMeterMeasuring(const FAtomLevelMeterSettings& InSettings)
	{
		ensure(IsInAtomThread());

		// LevelMeters for Racks
		// this are object only avaiable for rack and limited to some mixage.
		// since we don't have any pcm filter for rack we have to rely on this to get final mix output of a rack with its children racks.

		if (bIsMainDspBus && RackID != INDEX_NONE)
		{
			if (bIsLevelMeterMeasuring)
			{
				// Calling twice attach() causes a warning and the stop of the previous.
				StopLevelMeterMeasuring();
			}

			bIsLevelMeterMeasuring = true;

			LevelMeterSettings = InSettings;

			CriAtomLevelMeterConfig LevelMeterConfig;
			LevelMeterConfig.interval = (CriSint32)(FMath::Clamp(LevelMeterSettings.AnalysisPeriod, 0.01f, 0.25f) * 1000.0f); // ms
			LevelMeterConfig.hold_time = (CriSint32)FMath::Max(LevelMeterSettings.PeakHoldTime, 0); // ms
			FCriWareApi::criAtomExAsrRack_AttachLevelMeter(RackID, &LevelMeterConfig, nullptr, 0);
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("Cannot start Level Meter to submix that is not the Main bus of a Atom rack"));
		}
	}

	void FMixerSubmix::StopLevelMeterMeasuring()
	{
		ensure(IsInAtomThread());

		if (bIsLevelMeterMeasuring)
		{
			FCriWareApi::criAtomExAsrRack_DetachLevelMeter(RackID);
		}
		bIsLevelMeterMeasuring = false;
	}

	void FMixerSubmix::AddLevelMeterDelegate(const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP)
	{
		OnAtomRackLevelMeterMeasure.AddUnique(OnAtomRackLevelMeterMeasureBP);
	}

	void FMixerSubmix::RemoveLevelMeterDelegate(const FOnAtomRackLevelMeterMeasureBP& OnAtomRackLevelMeterMeasureBP)
	{
		OnAtomRackLevelMeterMeasure.Remove(OnAtomRackLevelMeterMeasureBP);
	}

	void FMixerSubmix::StartLoudnessMeterMeasuring(const FAtomLoudnessMeterSettings& InSettings)
	{
		ensure(IsInAtomThread());

		// LoudnessMeters for Racks
		// this are object only avaiable for rack and limited to some mixage.
		// since we don't have any pcm filter for rack we have to rely on this to get final mix output of a rack with its children racks.

		if (bIsMainDspBus && RackID != INDEX_NONE)
		{
			if (bIsLoudnessMeterMeasuring)
			{
				// Calling twice attach() causes a warning and the stop of the previous.
				StopLoudnessMeterMeasuring();
			}

			bIsLoudnessMeterMeasuring = true;

			LoudnessMeterSettings = InSettings;

			CriAtomLoudnessMeterConfig LoudnessMeterConfig;
			LoudnessMeterConfig.integrated_time = (CriSint32)FMath::Clamp(LoudnessMeterSettings.ShortTermTime, 1, 10);
			LoudnessMeterConfig.short_term_time = (CriSint32)FMath::Clamp(LoudnessMeterSettings.IntegratedTime, 10, 1000);
			FCriWareApi::criAtomExAsrRack_AttachLoudnessMeter(RackID, &LoudnessMeterConfig, nullptr, 0);
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("Cannot start Loudness Meter to submix that is not the Main bus of a Atom rack"));
		}
	}

	void FMixerSubmix::StopLoudnessMeterMeasuring()
	{
		ensure(IsInAtomThread());

		if (bIsLoudnessMeterMeasuring)
		{
			FCriWareApi::criAtomExAsrRack_DetachLoudnessMeter(RackID);
		}
		bIsLevelMeterMeasuring = false;
	}

	void FMixerSubmix::ResetLoudnessMeterMeasuring()
	{
		ensure(IsInAtomThread());

		if (bIsLoudnessMeterMeasuring)
		{
			FCriWareApi::criAtomExAsrRack_ResetLoudnessMeter(RackID);
		}
	}

	void FMixerSubmix::AddLoudnessMeterDelegate(const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP)
	{
		OnAtomRackLoudnessMeterMeasure.AddUnique(OnAtomRackLoudnessMeterMeasureBP);
	}

	void FMixerSubmix::RemoveLoudnessMeterDelegate(const FOnAtomRackLoudnessMeterMeasureBP& OnAtomRackLoudnessMeterMeasureBP)
	{
		OnAtomRackLoudnessMeterMeasure.Remove(OnAtomRackLoudnessMeterMeasureBP);
	}

	void FMixerSubmix::StartTruePeakMeterMeasuring(const FAtomTruePeakMeterSettings& InSettings)
	{
		ensure(IsInAtomThread());

		// TruePeakMeters for Racks
		// this are object only avaiable for rack and limited to some mixage.
		// since we don't have any pcm filter for rack we have to rely on this to get final mix output of a rack with its children racks.

		if (bIsMainDspBus && RackID != INDEX_NONE)
		{
			if (bIsTruePeakMeterMeasuring)
			{
				// Calling twice attach() causes a warning and the stop of the previous.
				StopTruePeakMeterMeasuring();
			}

			bIsTruePeakMeterMeasuring = true;

			TruePeakMeterSettings = InSettings;

			CriAtomTruePeakMeterConfig TruePeakMeterConfig;
			TruePeakMeterConfig.interval = (CriSint32)(FMath::Clamp(TruePeakMeterSettings.AnalysisPeriod, 0.01f, 0.25f) * 1000.0f); // ms
			TruePeakMeterConfig.hold_time = (CriSint32)FMath::Max(TruePeakMeterSettings.PeakHoldTime, 0); // ms
			TruePeakMeterConfig.sample_clipping = (CriBool)TruePeakMeterSettings.bSampleClipping;
			FCriWareApi::criAtomExAsrRack_AttachTruePeakMeter(RackID, &TruePeakMeterConfig, nullptr, 0);
		}
		else
		{
			UE_LOG(LogCriWareAtomMixer, Error, TEXT("Cannot start TruePeak Meter to submix that is not the Main bus of a Atom rack"));
		}
	}

	void FMixerSubmix::StopTruePeakMeterMeasuring()
	{
		ensure(IsInAtomThread());

		if (bIsTruePeakMeterMeasuring && bIsMainDspBus && RackID != INDEX_NONE)
		{
			FCriWareApi::criAtomExAsrRack_DetachTruePeakMeter(RackID);
		}
		bIsTruePeakMeterMeasuring = false;
	}

	void FMixerSubmix::AddTruePeakMeterDelegate(const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP)
	{
		OnAtomRackTruePeakMeterMeasure.AddUnique(OnAtomRackTruePeakMeterMeasureBP);
	}

	void FMixerSubmix::RemoveTruePeakMeterDelegate(const FOnAtomRackTruePeakMeterMeasureBP& OnAtomRackTruePeakMeterMeasureBP)
	{
		OnAtomRackTruePeakMeterMeasure.Remove(OnAtomRackTruePeakMeterMeasureBP);
	}

	void FMixerSubmix::AddRenderedSamplesDelegate(const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSampleBP)
	{
		OnAtomRackRenderedSamples.AddUnique(OnAtomRackRenderedSampleBP);
	}

	void FMixerSubmix::RemoveRenderedSamplesDelegate(const FOnAtomRackRenderedSamplesBP& OnAtomRackRenderedSampleBP)
	{
		OnAtomRackRenderedSamples.Remove(OnAtomRackRenderedSampleBP);
	}

	void FMixerSubmix::AddPerformanceMonitorDelegate(const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP)
	{
		OnAtomRackPerformanceMonitorResult.AddUnique(OnAtomRackPerformanceMonitorResultBP);
	}

	void FMixerSubmix::RemovePerformanceMonitorDelegate(const FOnAtomRackPerformanceMonitorResultBP& OnAtomRackPerformanceMonitorResultBP)
	{
		OnAtomRackPerformanceMonitorResult.Remove(OnAtomRackPerformanceMonitorResultBP);
	}

	void FMixerSubmix::ResetPerformanceMonitor()
	{
		ensure(IsInAtomThread());

		if (bIsMainDspBus && RackID != INDEX_NONE)
		{
			FCriWareApi::criAtomExAsrRack_ResetLoudnessMeter(RackID);
		}
	}

	void FMixerSubmix::SetOutputVolume(float InOutputVolume)
	{
		VolumeModifier = FMath::Clamp(InOutputVolume, 0.0f, 1.0f);
	}

	void FMixerSubmix::SetDryLevel(float InDryLevel)
	{
		DryLevelModifier = FMath::Clamp(InDryLevel, 0.0f, 1.0f);
	}

	void FMixerSubmix::SetWetLevel(float InWetLevel)
	{
		WetLevelModifier = FMath::Clamp(InWetLevel, 0.0f, 1.0f);
	}

	void FMixerSubmix::UpdateModulationSettings(const TSet<TObjectPtr<UAtomModulatorBase>>& InOutputModulators, const TSet<TObjectPtr<UAtomModulatorBase>>& InWetLevelModulators, const TSet<TObjectPtr<UAtomModulatorBase>>& InDryLevelModulators)
	{
		VolumeMod.UpdateModulators(InOutputModulators);
		WetLevelMod.UpdateModulators(InWetLevelModulators);
		DryLevelMod.UpdateModulators(InDryLevelModulators);
	}

	void FMixerSubmix::SetModulationBaseLevels(float InVolumeModBaseDb, float InWetModBaseDb, float InDryModBaseDb)
	{
		VolumeModBaseDb = InVolumeModBaseDb;
		WetModBaseDb = InWetModBaseDb;
		DryModBaseDb = InDryModBaseDb;
	}

	FModulationDestination* FMixerSubmix::GetOutputVolumeDestination()
	{
		return &VolumeMod;
	}

	FModulationDestination* FMixerSubmix::GetWetVolumeDestination()
	{
		return &WetLevelMod;
	}

	Audio::FPatchOutputStrongPtr FMixerSubmix::AddPatch(float InGain)
	{
		if (IsSoundfieldSubmix())
		{
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Patch list ening to SoundfieldSubmixes is not supported."));
			return nullptr;
		}

		return PatchSplitter.AddNewPatch(AtomRuntime->GetRuntimeNumOutputFrames()/*NumSamples*/, InGain);
	}

	void FMixerSubmix::BroadcastDelegates()
	{
		if (bIsEnvelopeFollowing)
		{
			// Get the envelope data
			TArray<float> EnvelopeData;

			{
				// Make the copy of the envelope values using a critical section
				FScopeLock EnvelopeScopeLock(&EnvelopeCriticalSection);

				if (EnvelopeNumChannels > 0)
				{
					EnvelopeData.AddUninitialized(EnvelopeNumChannels);
					FMemory::Memcpy(EnvelopeData.GetData(), EnvelopeValues, sizeof(float) * EnvelopeNumChannels);
				}
			}

			// Broadcast to any bound delegates
			if (OnAtomBusEnvelope.IsBound())
			{
				OnAtomBusEnvelope.Broadcast(EnvelopeData);
			}
		}

		// If we're analyzing spectra and if we've got delegates setup
		if (bIsSpectrumAnalyzing)
		{
			FScopeLock SpectrumLock(&SpectrumAnalyzerCriticalSection);

			if (SpectralAnalysisDelegates.Num() > 0)
			{
				if (ensureMsgf(SpectrumAnalyzer.IsValid(), TEXT("Analyzing spectrum with invalid spectrum analyzer")))
				{
					TArray<TPair<FSpectrumAnalysisDelegateInfo*, TArray<float>>> ResultsPerDelegate;

					{
						// This lock ensures that the spectrum analyzer's analysis buffer doesn't
						// change in this scope. 
						Audio::FAsyncSpectrumAnalyzerScopeLock AnalyzerLock(SpectrumAnalyzer.Get());

						for (FSpectrumAnalysisDelegateInfo& DelegateInfo : SpectralAnalysisDelegates)
						{
							TArray<float> SpectralResults;
							SpectralResults.Reset();
							const float CurrentTime = FPlatformTime::ToSeconds64(FPlatformTime::Cycles64());

							// Don't update the spectral band until it's time since the last tick.
							if (DelegateInfo.LastUpdateTime > 0.0f && ((CurrentTime - DelegateInfo.LastUpdateTime) < DelegateInfo.UpdateDelta))
							{
								continue;
							}

							DelegateInfo.LastUpdateTime = CurrentTime;

							if (ensure(DelegateInfo.SpectrumBandExtractor.IsValid()))
							{
								Audio::ISpectrumBandExtractor* Extractor = DelegateInfo.SpectrumBandExtractor.Get();

								SpectrumAnalyzer->GetBands(*Extractor, SpectralResults);
							}
							ResultsPerDelegate.Emplace(&DelegateInfo, MoveTemp(SpectralResults));
						}
					}

					for (TPair<FSpectrumAnalysisDelegateInfo*, TArray<float>> Results : ResultsPerDelegate)
					{
						FSpectrumAnalysisDelegateInfo* DelegateInfo = Results.Key;
						// Feed the results through the band envelope followers
						for (int32 ResultIndex = 0; ResultIndex < Results.Value.Num(); ++ResultIndex)
						{
							if (ensure(ResultIndex < DelegateInfo->SpectralBands.Num()))
							{
								FSpectralAnalysisBandInfo& BandInfo = DelegateInfo->SpectralBands[ResultIndex];

								Results.Value[ResultIndex] = BandInfo.EnvelopeFollower.ProcessSample(Results.Value[ResultIndex]);
							}
						}

						if (DelegateInfo->OnAtomBusSpectralAnalysis.IsBound())
						{
							DelegateInfo->OnAtomBusSpectralAnalysis.Broadcast(MoveTemp(Results.Value));
						}
					}
				}
			}
		}

		if (bIsLevelMeterMeasuring && RackID != INDEX_NONE)
		{
			CriAtomLevelInfo LevelInfo;
			FCriWareApi::criAtomExAsrRack_GetLevelInfo(RackID, &LevelInfo);
			const int32 MeasureNumChannels = LevelInfo.num_channels;

			if (MeasureNumChannels > 0)
			{
				LevelMeterMeasures.Reset();
				LevelMeterMeasures.AddUninitialized(MeasureNumChannels);
				for (int ChannelIndex = 0; ChannelIndex < MeasureNumChannels; ++ChannelIndex)
				{
					const float Level = LevelInfo.rms_levels[ChannelIndex];
					const float PeakLevel = LevelInfo.peak_levels[ChannelIndex];
					const float PeakHoldLevel = LevelInfo.peak_hold_levels[ChannelIndex];
					LevelMeterMeasures[ChannelIndex] = { Level, PeakLevel, PeakHoldLevel };
				}
			}

			// Broadcast to any bound delegates
			if (OnAtomRackLevelMeterMeasure.IsBound())
			{
				OnAtomRackLevelMeterMeasure.Broadcast(LevelMeterMeasures);
			}
		}

		if (bIsLoudnessMeterMeasuring && RackID != INDEX_NONE)
		{
			CriAtomLoudnessInfo LoudnessInfo;
			FCriWareApi::criAtomExAsrRack_GetLoudnessInfo(RackID, &LoudnessInfo);
				
			//const int32 NumMeasuresSinceLastCall = LoudnessInfo.count;
			const float Momentary = LoudnessInfo.momentary;
			const float ShortTerm = LoudnessInfo.short_term;
			const float Integrated = LoudnessInfo.integrated;
			LoudnessMeterMeasure = { Momentary, ShortTerm, Integrated };
		
			// Broadcast to any bound delegates
			if (OnAtomRackLoudnessMeterMeasure.IsBound())
			{
				OnAtomRackLoudnessMeterMeasure.Broadcast(LoudnessMeterMeasure);
			}
		}

		if (bIsTruePeakMeterMeasuring && RackID != INDEX_NONE)
		{
			CriAtomTruePeakInfo TruePeakInfo;
			FCriWareApi::criAtomExAsrRack_GetTruePeakInfo(RackID, &TruePeakInfo);
			const int32 MeasureNumChannels = TruePeakInfo.num_channels;

			if (MeasureNumChannels > 0)
			{
				TruePeakMeterMeasures.Reset();
				TruePeakMeterMeasures.AddUninitialized(MeasureNumChannels);
				for (int ChannelIndex = 0; ChannelIndex < MeasureNumChannels; ++ChannelIndex)
				{
					const float Level = TruePeakInfo.levels[ChannelIndex];
					const float HoldLevel = TruePeakInfo.hold_levels[ChannelIndex];
					TruePeakMeterMeasures[ChannelIndex] = { Level, HoldLevel };
				}
			}

			// Broadcast to any bound delegates
			if (OnAtomRackTruePeakMeterMeasure.IsBound())
			{
				OnAtomRackTruePeakMeterMeasure.Broadcast(TruePeakMeterMeasures);
			}
		}
	}

	bool FMixerSubmix::IsSoundfieldSubmix() const
	{
		if (const UAtomBus* AtomBus = Cast<const UAtomBus>(OwningAtomBusObject))
		{
			return Cast<UAtomSoundfieldRack>(AtomBus->GetRack()) != nullptr;
		}

		return false;
	}

	bool FMixerSubmix::IsDefaultEndpointSubmix() const
	{
		return bIsMainDspBus;
	}

	bool FMixerSubmix::IsDummyEndpointSubmix() const
	{
		if (const UAtomBus* AtomBus = Cast<const UAtomBus>(OwningAtomBusObject))
		{
			if (auto Rack = Cast<UAtomRackWithParentBase>(AtomBus->GetRack()))
			{
				if (auto Endpoint = Cast<UAtomEndpointRack>(Rack->ParentRack))
				{
					return Endpoint->SoundRendererType == EAtomSoundRendererType::Muted;
				}
			}
		}

		return false;
	}

	void FMixerSubmix::HandleNativeOnPreEffectFilter(CriAtomPcmFormat Format, CriSint32 InNumChannels, CriSint32 NumFrames, void* Data[])
	{
		check(FAtomRuntime::GetPcmBitDepthFromAtomPcmFormat(Format) == EAtomPcmBitDepth::Float32);

		// first, copy Atom buffer to interleave buffer for Unreal DSP.
		Interleave((const float**)Data, InterleavedBuffer, InNumChannels, NumFrames);

		// process DSP
		ProcessAudioPreEffects(InterleavedBuffer, InNumChannels);

		// De-interleave input buffer back to Atom buffer
		Deinterleave(InterleavedBuffer, (float**)Data, InNumChannels, NumFrames);
	}

	void FMixerSubmix::HandleNativeOnPostEffectFilter(CriAtomPcmFormat Format, CriSint32 InNumChannels, CriSint32 NumFrames, void* Data[])
	{
		check(FAtomRuntime::GetPcmBitDepthFromAtomPcmFormat(Format) == EAtomPcmBitDepth::Float32);

		// first, copy Atom buffer to interleave buffer for Unreal DSP.
		Interleave((const float**)Data, InterleavedBuffer, InNumChannels, NumFrames);

		// process DSP
		ProcessAudioPostEffects(InterleavedBuffer, InNumChannels);

		// De-interleave input buffer back to Atom buffer
		Deinterleave(InterleavedBuffer, (float**)Data, InNumChannels, NumFrames);
	}
} // namespace
