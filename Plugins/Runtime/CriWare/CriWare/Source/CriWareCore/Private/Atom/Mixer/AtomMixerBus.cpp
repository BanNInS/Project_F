
#include "Atom/Mixer/AtomMixerBus.h"

#include "Algo/ForEach.h"
#include "DSP/AlignedBuffer.h"
#include "DSP/FloatArrayMath.h"

#include "Atom/Mixer/AtomMixerSourceManager.h"

namespace Atom
{
	FMixerAudioBus::FMixerAudioBus(FMixerSourceManager* InSourceManager, const FAudioBusKey& InBusKey, bool bInIsAutomatic, int32 InNumChannels, int32 InSampleRate)
		: MixedSourceData(InSourceManager->GetNumOutputFrames())
		, CurrentBufferIndex(1)
		, SampleRate(InSampleRate)
		, NumChannels(InNumChannels)
		, NumFrames(InSourceManager->GetNumOutputFrames())
		, SourceManager(InSourceManager)
		, BusKey(InBusKey)
		, bIsAutomatic(bInIsAutomatic)
	{
		SetNumOutputChannels(NumChannels);
	}

	void FMixerAudioBus::SetNumOutputChannels(int32 InNumOutputChannels)
	{
		NumChannels = InNumOutputChannels;
		NumFrames = SourceManager->GetNumOutputFrames();

//#if !ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		// add input port size/call speed setting too 
		int32 BufferSizeInSamples = NumFrames * InNumOutputChannels;
		int32 ReserveSizeInSamples = (float)BufferSizeInSamples * IntermediateBufferRatio;
		int32 SilenceToAddToFirstBuffer = FMath::Min((float)BufferSizeInSamples * InitialSilenceFillRatio, ReserveSizeInSamples);
		MixedSourceData.SetCapacity(ReserveSizeInSamples);
		MixedSourceData.PushZeros(SilenceToAddToFirstBuffer);
//#endif
	}

	void FMixerAudioBus::Update()
	{
		CurrentBufferIndex = 1 - CurrentBufferIndex;
	}

	void FMixerAudioBus::AddInstanceID(const int32 InSourceID, const uint64 InTransmitterID, int32 InNumOutputChannels)
	{
		InstanceIDs.Add(InSourceID);
	}

	bool FMixerAudioBus::RemoveInstanceID(const int32 InSourceID, const uint64 InTransmitterID)
	{
		InstanceIDs.Remove(InSourceID);

		// Return true if there is no more instances or sends
		return bIsAutomatic && !InstanceIDs.Num() && !AudioBusSends[(int32)EAtomBusSendStage::PreEffect].Num() && !AudioBusSends[(int32)EAtomBusSendStage::PostEffect].Num();
	}

	void FMixerAudioBus::AddSend(EAtomBusSendStage BusSendStage, const FAudioBusSend& InAudioBusSend)
	{
		// Make sure we don't have duplicates in the bus sends
		for (FAudioBusSend& BusSend : AudioBusSends[(int32)BusSendStage])
		{
			// If it's already added, just update the send level
			if (BusSend.SourceID == InAudioBusSend.SourceID)
			{
				BusSend.SendLevel = InAudioBusSend.SendLevel;
				return;
			}
		}

		// It's a new source id so just add it
		int32 Index = AudioBusSends[(int32)BusSendStage].Add(InAudioBusSend);

#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
		auto& PatchInput = AudioBusSends[(int32)BusSendStage][Index].PatchInput;
		PatchInput = SourceManager->GetAtomRuntime()->MakePatch(NumFrames * 2, InAudioBusSend.SendLevel, NumChannels);
		AddNewPatchInput(PatchInput);
#endif
	}

	bool FMixerAudioBus::RemoveSend(EAtomBusSendStage BusSendStage, const int32 InSourceID)
	{
		TArray<FAudioBusSend>& Sends = AudioBusSends[(int32)BusSendStage];
		for (int32 Index = Sends.Num() - 1; Index >= 0; --Index)
		{
			// Remove this source id's send
			if (Sends[Index].SourceID == InSourceID)
			{
#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
				// Remove the patch input
				RemovePatchInput(Sends[Index].PatchInput);
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
				Sends.RemoveAtSwap(Index, EAllowShrinking::No);
#else
				Sends.RemoveAtSwap(Index, 1, false);
#endif
				// There will only be one entry
				break;
			}
		}

		// Return true if there is no more instances or sends and this is an automatic audio bus
		return bIsAutomatic && !InstanceIDs.Num() && !AudioBusSends[(int32)EAtomBusSendStage::PreEffect].Num() && !AudioBusSends[(int32)EAtomBusSendStage::PostEffect].Num();
	}

#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
	int32 FMixerAudioBus::PushSendAudio(EAtomBusSendStage BusSendStage, const int32 InSourceID, const Atom::FAlignedFloatBuffer& InBuffer)
	{
		TArray<FAudioBusSend>& Sends = AudioBusSends[(int32)BusSendStage];
		for (int32 Index = 0; Index < Sends.Num(); ++Index)
		{
			const auto& SourceSend = Sends[Index];
			if (SourceSend.SourceID == InSourceID)
			{
				const int32 NumSourceChannels = SourceManager->GetNumChannels(SourceSend.SourceID);
				if (NumSourceChannels > 0)
				{
					if (NumChannels != NumSourceChannels)
					{
						FAlignedFloatBuffer ChannelMap;
						SourceManager->Get2DChannelMap(InSourceID, NumChannels, ChannelMap);
						Algo::ForEach(ChannelMap, [SendLevel = SourceSend.SendLevel](float& ChannelValue) { ChannelValue *= SendLevel; });

						DownmixedBuffer.SetNumUninitialized((InBuffer.Num() / NumSourceChannels) * NumChannels);
						Audio::DownmixBuffer(NumSourceChannels, NumChannels, InBuffer, DownmixedBuffer, ChannelMap.GetData());

						return Sends[Index].PatchInput.PushAudio(DownmixedBuffer.GetData(), DownmixedBuffer.Num()); // set the data in the patch input
					}
					else
					{
						return Sends[Index].PatchInput.PushAudio(InBuffer.GetData(), InBuffer.Num()); // set the data in the patch input
					}
				}
			}
		}

		return -1;
	}
#endif

	void FMixerAudioBus::MixBuffer()
	{
		// Mix the patch mixer's inputs into the source data buffer
		const int32 MaxNumSamples = NumFrames * NumChannels; // 1024 per ch max

		// Mix max we can from the patch mixer
		int32 NumMixedSamples = PatchMixer.MaxNumberOfSamplesThatCanBePopped(); // set the max number of samples that can be popped from the patch mixer
		NumMixedSamples = FMath::Min(NumMixedSamples, MaxNumSamples); // make sure we don't exceed the max number of samples
		NumMixedSamples = FMath::Max(NumMixedSamples, 0); // make sure we don't have negative samples

#if ATOM_MIXER_ENABLE_PCM_TRACE
		UE_LOG(LogTemp, Error, TEXT("------ MIX in %d"), NumMixedSamples);
#endif

#if ATOM_MIXER_SOURCE_USES_BUS_PATCH_INPUTS
#if 1
		if (NumMixedSamples > 0)
		{
			// If some mixed data exist, pop them
			IntermediateBuffers[CurrentBufferIndex].SetNumUninitialized(NumMixedSamples); // set the size of the buffer
			float* BusDataBufferPtr = IntermediateBuffers[CurrentBufferIndex].GetData(); // get the data from the buffer
			int32 NumSamples = PatchMixer.PopAudio(BusDataBufferPtr, NumMixedSamples, false);
			check(NumSamples == NumMixedSamples);

			int32 MaxPushSamples = (MixedSourceData.Remainder() / NumChannels) * NumChannels;
			MaxPushSamples = FMath::Min(MaxPushSamples, NumMixedSamples);
			MixedSourceData.Push(BusDataBufferPtr, MaxPushSamples); // push the mixed data to the buffer
		}

		// Send the final mix to the patch splitter's outputs
		int32 MaxSplitSamples = FMath::Min(MixedSourceData.Num(), PatchSplitter.MaxNumberOfSamplesThatCanBePushed()); // make sure we don't exceed the max number of samples availables
		MaxSplitSamples = (MaxSplitSamples / NumChannels) * NumChannels;
		if (MaxSplitSamples > 0)
		{
			PushBuffer.SetNumUninitialized(MaxSplitSamples); // set the size of the buffer
			float* PushBufferPtr = PushBuffer.GetData();
			MaxSplitSamples = MixedSourceData.Pop(PushBufferPtr, MaxSplitSamples);
			if (MaxSplitSamples > 0)
			{
				PatchSplitter.PushAudio(PushBufferPtr, MaxSplitSamples);

#if ATOM_PROFILERTRACE_ENABLED
				if (bIsEnvelopeFollowing)
				{
					ProcessEnvelopeFollower(PushBufferPtr);
				}
#endif // ATOM_PROFILERTRACE_ENABLED
			}
		}
#endif
#if 0
		int32 MaxSplitSamples = FMath::Min(NumMixedSamples, PatchSplitter.MaxNumberOfSamplesThatCanBePushed()); // make sure we don't exceed the max number of samples availables
		MaxSplitSamples = (MaxSplitSamples / NumChannels) * NumChannels;
		if (MaxSplitSamples > 0)
		{
			MixedSourceBuffers[CurrentBufferIndex].SetNumUninitialized(MaxSplitSamples); // set the size of the buffer
			float* PushBufferPtr = MixedSourceBuffers[CurrentBufferIndex].GetData(); // get the data from the buffer
			MaxSplitSamples = PatchMixer.PopAudio(PushBufferPtr, MaxSplitSamples, false);
			if (MaxSplitSamples > 0)
			{
				PatchSplitter.PushAudio(PushBufferPtr, MaxSplitSamples);
			}
		}
#endif
#else
		TArray<const FAlignedFloatBuffer*> SourceBuffers;
		TArray<const FAudioBusSend*> SourceSends;
		int32 NumFramesToSend = NumMixedSamples > 0 ? NumMixedSamples / NumChannels : NumFrames;

		// mix max we can from source sends if the mixer is empty the source with the max buffer give the max samples
		for (int32 BusSendStage = 0; BusSendStage < (int32)EAtomBusSendStage::Count; ++BusSendStage)
		{
			// Loop through the send list for this bus

			for (const FAudioBusSend& AudioBusSend : AudioBusSends[BusSendStage])
			{
				const float* SourceBufferPtr = nullptr;
				const int32 NumSourceChannels = SourceManager->GetNumChannels(AudioBusSend.SourceID);

				// If the audio source mixing to this audio bus is itself a source bus, we need to use the previous renderer buffer to avoid infinite recursion
				if (SourceManager->IsSourceBus(AudioBusSend.SourceID))
				{
					auto& SourceBuffer = SourceManager->GetPreviousSourceBusBuffer(AudioBusSend.SourceID);
					SourceBuffers.Add(&SourceBuffer);
					SourceSends.Add(&AudioBusSend);
					NumFramesToSend = FMath::Min(SourceBuffer.Num() / NumSourceChannels, NumFramesToSend);
				}
				// If the source mixing into this is not itself a bus, then simply mix the pre-attenuation audio of the source into the bus
				// The source will have already computed its buffers for this frame
				else if (BusSendStage == (int32)EAtomBusSendStage::PostEffect)
				{
					auto& SourceBuffer = SourceManager->GetPreDistanceAttenuationBuffer(AudioBusSend.SourceID);
					SourceBuffers.Add(&SourceBuffer);
					SourceSends.Add(&AudioBusSend);
					NumFramesToSend = FMath::Min(SourceBuffer.Num() / NumSourceChannels, NumFramesToSend);
				}
				else
				{
					auto& SourceBuffer = SourceManager->GetPreEffectBuffer(AudioBusSend.SourceID);
					SourceBuffers.Add(&SourceBuffer);
					SourceSends.Add(&AudioBusSend);
					NumFramesToSend = FMath::Min(SourceBuffer.Num() / NumSourceChannels, NumFramesToSend);
				}
			}
		}

		const int NumSamplesToSend = NumFramesToSend * NumChannels;

		// get the buffer to mix into
		float* BusDataBufferPtr = nullptr;

		if (NumMixedSamples > 0 && NumSamplesToSend > 0)
		{
			// If some mixed data exist, pop them
			IntermediateBuffers[CurrentBufferIndex].SetNumUninitialized(NumSamplesToSend); // set the size of the buffer
			BusDataBufferPtr = IntermediateBuffers[CurrentBufferIndex].GetData(); // get the data from the buffer
			int32 NumSamples = PatchMixer.PopAudio(BusDataBufferPtr, NumSamplesToSend, false);
			check(NumSamples == NumSamplesToSend);
#if ATOM_MIXER_ENABLE_PCM_TRACE
			/*UE_LOG(LogTemp, Error, TEXT("MixIn data: %d [%f %f][%f %f]"), NumSamplesToSend
				, BusDataBufferPtr[0]
				, BusDataBufferPtr[1]
				, BusDataBufferPtr[2]
				, BusDataBufferPtr[3]);*/
#endif
		}
		else if (NumSamplesToSend > 0)
		{
			// create empty with zeros buffer to mix in
			IntermediateBuffers[CurrentBufferIndex].Empty(); // set the size of the buffer
			IntermediateBuffers[CurrentBufferIndex].SetNumZeroed(NumSamplesToSend);
			BusDataBufferPtr = IntermediateBuffers[CurrentBufferIndex].GetData(); // get the data from the buffer
		}
		else
		{
			// Prebuffer cannot be 0
			IntermediateBuffers[CurrentBufferIndex].Empty();
			IntermediateBuffers[CurrentBufferIndex].SetNumZeroed(MaxNumSamples);
		}

#if ATOM_MIXER_ENABLE_PCM_TRACE
		UE_LOG(LogTemp, Error, TEXT("MIX mid: M:%d, toSend:%d, Sources:%d"), NumMixedSamples, NumSamplesToSend, SourceBuffers.Num());
#endif

		if (BusDataBufferPtr)
		{
			if (SourceBuffers.Num() > 0)
			{
				// then mix the source sends to it
				for (int Index = 0; Index < SourceSends.Num(); ++Index)
				{
					const auto& SourceSend = *SourceSends[Index];
					const auto SourceBuffer = SourceBuffers[Index];
					const float* SourceBufferPtr = SourceBuffer->GetData(); // get the data from the source buffer

					// It's possible we may not have a source buffer ptr here if the sound is not playing
					if (SourceBufferPtr)
					{
#if ATOM_MIXER_ENABLE_PCM_TRACE
						/*UE_LOG(LogTemp, Error, TEXT("Source: %d [%f %f][%f %f]"), SourceSend.SourceID
							, SourceBufferPtr[0]
							, SourceBufferPtr[1]
							, SourceBufferPtr[2]
							, SourceBufferPtr[3]);*/
#endif
						const int32 NumSourceChannels = SourceManager->GetNumChannels(SourceSend.SourceID);
						if (NumSourceChannels > 0)
						{
							// Up-mix or down-mix if source channels differ from bus channels
							if (NumSourceChannels != NumChannels)
							{
								FAlignedFloatBuffer ChannelMap;
								SourceManager->Get2DChannelMap(SourceSend.SourceID, NumChannels, ChannelMap);
								Algo::ForEach(ChannelMap, [SendLevel = SourceSend.SendLevel](float& ChannelValue) { ChannelValue *= SendLevel; });
								Audio::DownmixAndSumIntoBuffer(NumSourceChannels, NumChannels, SourceBufferPtr, BusDataBufferPtr, NumFramesToSend, ChannelMap.GetData());
							}
							else
							{
								TArrayView<const float> SourceBufferView(SourceBufferPtr, NumSamplesToSend);
								TArrayView<float> BusDataBufferView(BusDataBufferPtr, NumSamplesToSend);
								Audio::ArrayMixIn(SourceBufferView, BusDataBufferView, SourceSend.SendLevel);
							}
						}
					}
				}
			}

			int32 MaxPushSamples = (MixedSourceData.Remainder() / NumChannels) * NumChannels;
			MaxPushSamples = FMath::Min(MaxPushSamples, NumSamplesToSend);
			MixedSourceData.Push(BusDataBufferPtr, MaxPushSamples); // push the mixed data to the buffer
#if ATOM_MIXER_ENABLE_PCM_TRACE
			/*UE_LOG(LogTemp, Error, TEXT("Push: %d/%d (->%dFree) [%f %f %f][%f %f %f]"), MixedSourceData.Num(), MixedSourceData.GetCapacity(), MixedSourceData.Remainder()
				, BusDataBufferPtr[0]
				, BusDataBufferPtr[1]
				, BusDataBufferPtr[2]
				, BusDataBufferPtr[3]
				, BusDataBufferPtr[4]
				, BusDataBufferPtr[5]);*/
#endif
		}

		// Send the final mix to the patch splitter's outputs
		int32 MaxSplitSamples = FMath::Min(MixedSourceData.Num(), PatchSplitter.MaxNumberOfSamplesThatCanBePushed()); // make sure we don't exceed the max number of samples availables
		MaxSplitSamples = (MaxSplitSamples / NumChannels) * NumChannels;
		if (MaxSplitSamples > 0)
		{
			PushBuffer.Reset();
			PushBuffer.AddUninitialized(MaxSplitSamples); // set the size of the buffer
			float* PushBufferPtr = PushBuffer.GetData();
			MaxSplitSamples = MixedSourceData.Pop(PushBufferPtr, MaxSplitSamples);
			if (MaxSplitSamples > 0)
			{
				PatchSplitter.PushAudio(PushBufferPtr, MaxSplitSamples);

#if ATOM_PROFILERTRACE_ENABLED
				if (bIsEnvelopeFollowing)
				{
					ProcessEnvelopeFollower(PushBufferPtr);
				}
#endif // ATOM_PROFILERTRACE_ENABLED

#if ATOM_MIXER_ENABLE_PCM_TRACE
				/*UE_LOG(LogTemp, Error, TEXT("Pop->Splitter: %d (%d)  [%f %f %f][%f %f %f]"), MaxSplitSamples, PatchSplitter.Num()
					, PushBufferPtr[0]
					, PushBufferPtr[1]
					, PushBufferPtr[2]
					, PushBufferPtr[3]
					, PushBufferPtr[4]
					, PushBufferPtr[5]);*/
#endif
			}
		}
#endif

#if ATOM_MIXER_ENABLE_PCM_TRACE
		UE_LOG(LogTemp, Error, TEXT("------- MIX out: %d"), MaxSplitSamples);
#endif
	}

	const Atom::FAlignedFloatBuffer& FMixerAudioBus::GetCurrentBusBuffer() const
	{
		return IntermediateBuffers[CurrentBufferIndex];
	}

	const Atom::FAlignedFloatBuffer& FMixerAudioBus::GetPreviousBusBuffer() const
	{
		return IntermediateBuffers[!CurrentBufferIndex];
	}

	void FMixerAudioBus::AddNewPatchOutput(const  Audio::FPatchOutputStrongPtr& InPatchOutputStrongPtr)
	{
		PatchSplitter.AddNewPatch(InPatchOutputStrongPtr);
	}

	void FMixerAudioBus::AddNewPatchInput(const Audio::FPatchInput& InPatchInput)
	{
		return PatchMixer.AddNewInput(InPatchInput);
	}

	void FMixerAudioBus::RemovePatchInput(const Audio::FPatchInput& PatchInput)
	{
		return PatchMixer.RemovePatch(PatchInput);
	}

#if ATOM_PROFILERTRACE_ENABLED
	void FMixerAudioBus::StartEnvelopeFollower(const float InAttackTime, const float InReleaseTime)
	{
		if (!bIsEnvelopeFollowing)
		{
			Audio::FEnvelopeFollowerInitParams EnvelopeFollowerInitParams;

			EnvelopeFollowerInitParams.SampleRate = SampleRate;
			EnvelopeFollowerInitParams.NumChannels = NumChannels;
			EnvelopeFollowerInitParams.AttackTimeMsec = InAttackTime;
			EnvelopeFollowerInitParams.ReleaseTimeMsec = InReleaseTime;

			EnvelopeFollower.Init(EnvelopeFollowerInitParams);

			// Zero out any previous envelope values which may have been in the array before starting up
			for (int32 ChannelIndex = 0; ChannelIndex < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++ChannelIndex)
			{
				EnvelopeValues[ChannelIndex] = 0.0f;
			}

			bIsEnvelopeFollowing = true;
		}
	}

	void FMixerAudioBus::StopEnvelopeFollower()
	{
		bIsEnvelopeFollowing = false;
	}

	void FMixerAudioBus::ProcessEnvelopeFollower(const float* InBuffer)
	{
		FMemory::Memset(EnvelopeValues, sizeof(float) * ATOM_MIXER_MAX_OUTPUT_CHANNELS);

		if (NumChannels > 0)
		{
			if (EnvelopeFollower.GetNumChannels() != NumChannels)
			{
				EnvelopeFollower.SetNumChannels(NumChannels);
			}

			EnvelopeFollower.ProcessAudio(InBuffer, NumFrames);

			const TArray<float>& EnvValues = EnvelopeFollower.GetEnvelopeValues();

			check(EnvValues.Num() == NumChannels);

			FMemory::Memcpy(EnvelopeValues, EnvValues.GetData(), sizeof(float) * NumChannels);

			Audio::ArrayClampInPlace(MakeArrayView(EnvelopeValues, NumChannels), 0.0f, 1.0f);
		}

		EnvelopeNumChannels = NumChannels;
	}
#endif // ATOM_PROFILERTRACE_ENABLED
}
