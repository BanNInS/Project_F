/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomRackSubmixListener.cpp
 *
 ****************************************************************************/

#include "AtomRackSubmixListener.h"

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Sound/SoundSubmix.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRack.h"

FAtomRackSubmixListener::FAtomRackSubmixListener(int32 InRackID, int32 InRackNumChannels, int32 InRackSampleRate, USoundSubmix* InSubmix)
	: RackBuffer(InRackNumChannels)
	, RackID(InRackID)
	, RackNumChannels(InRackNumChannels)
	, RackSampleRate(InRackSampleRate)
	, Submix(InSubmix)
{
	Buffers.AddDefaulted(InRackNumChannels);
	OutputBuffers.AddDefaulted(InRackNumChannels);
}

FAtomRackSubmixListener::~FAtomRackSubmixListener()
{
	UnregisterSubmixListener();

	RackID = INDEX_NONE;
	Submix = nullptr;
}

void FAtomRackSubmixListener::RegisterSubmixListener()
{
	if (FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get())
	{
		if (FAudioDeviceHandle AudioDeviceHandle = AudioDeviceManager->GetMainAudioDeviceHandle())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
            USoundSubmix* SubmixToRegister = Submix ? Submix : &AudioDeviceHandle->GetMainSubmixObject();
			AudioDeviceHandle->RegisterSubmixBufferListener(AsShared(), *SubmixToRegister);
#else
            AudioDeviceHandle->RegisterSubmixBufferListener(this, Submix);
#endif
			// RegisterSubmixBufferListener lazily enqueues the registration on the audio thread,
			// so we have to wait for the audio thread to destroy.
			//FAudioCommandFence Fence;
			//Fence.BeginFence();
			//Fence.Wait();
			return;
		}
	}
	ensure(false); // if coming here, something is wrong with unreal audio
}

void FAtomRackSubmixListener::UnregisterSubmixListener()
{
	if (FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get())
	{
		if (FAudioDeviceHandle AudioDeviceHandle = AudioDeviceManager->GetMainAudioDeviceHandle())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
            USoundSubmix* SubmixToUnregister = Submix ? Submix : &AudioDeviceHandle->GetMainSubmixObject();
            AudioDeviceHandle->UnregisterSubmixBufferListener(AsShared(), *SubmixToUnregister);
#else
			AudioDeviceHandle->UnregisterSubmixBufferListener(this, Submix);
#endif
			// UnregisterSubmixBufferListener lazily enqueues the unregistration on the audio thread,
			// so we have to wait for the audio thread to destroy.
			FAudioCommandFence Fence;
			Fence.BeginFence();
			Fence.Wait();
		}
	}
}

void FAtomRackSubmixListener::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock)
{
	const int32 NumFrames = NumSamples / NumChannels;
	const int32 NumChannelsToUse = FMath::Min(RackNumChannels, NumChannels);
	const int32 NumBufferedFrames = FCriWareApi::criAtomExAsrRack_GetNumBufferedSamples((CriAtomExAsrRackId)RackID); // Atom use 'samples' as 'frames'
	const int32 NumFramesToUse = NumFrames;
	const int32 NumFramesToRequest = FMath::Min(NumBufferedFrames, FMath::Max(RackBuffer.GetRemainder() / NumChannels, 0));
	
	// not interleaved
	for (int32 ChannelIndex = 0; ChannelIndex < RackNumChannels; ++ChannelIndex)
	{
		auto& OutputBuffer = Buffers[ChannelIndex];
		OutputBuffer.Reset();
		OutputBuffer.AddZeroed(NumFramesToRequest);
		OutputBuffers[ChannelIndex] = OutputBuffer.GetData();
	}

	// Enqueue to ring buffer if some new - a bug in atom force us to get all rack channels buffered.
	const int32 NumPoppedFrames = FCriWareApi::criAtomExAsrRack_GetPcmDataFloat32((CriAtomExAsrRackId)RackID, (CriSint32)RackNumChannels, (CriSint32)NumFramesToRequest, OutputBuffers.GetData());
	if (NumPoppedFrames > 0)
	{
		const float RateRatio = (float)SampleRate / RackSampleRate;

		UE_LOG(LogCriWareAtom, VeryVerbose, TEXT("RackSubmixListener: Number of frames popped by Atom: %i"), NumPoppedFrames);
		RackBuffer.EnqueueData(Buffers, NumChannelsToUse, NumFramesToRequest, RateRatio);
	}


	RackBuffer.PopData(NumSamples, AudioData);
}

bool FAtomRackSubmixListener::IsRenderingAudio() const
{
	return true;
}

FAtomRackSubmixListener::FAtomRackSubmixBuffer::FAtomRackSubmixBuffer(int InitialNumChannels, FBufferParams&& InParams)
	: BufferParams(MoveTemp(InParams))
	, Buffer(BufferParams.ReceivingBufferSizeInFrames * InitialNumChannels * BufferParams.RackToSubmixBufferRatio)
	, NumChannels(InitialNumChannels)
	, SampleRateRatio(1.0f)
{
	Buffer.PushZeros(NumChannels * BufferParams.ReceivingBufferSizeInFrames * BufferParams.InitialSilenceFillRatio);
	AudioResampler.Init(Audio::EResamplingMethod::Linear, 1.0f, NumChannels);
}

int32 FAtomRackSubmixListener::FAtomRackSubmixBuffer::EnqueueData(TArray<TArray<float>>& InData, int32 InNumChannels, int NumFrames, float InSampleRateRatio)
{
	if (InNumChannels != NumChannels || SampleRateRatio != InSampleRateRatio)
	{
		// scratch all and reset
		Reset(InNumChannels, InSampleRateRatio);
	}

	const int32 NumSamples = NumFrames * NumChannels;

	TempInterleavedBuffer.Reset();
	TempInterleavedBuffer.AddZeroed(NumSamples);

	// interleave data and discared useless data
	for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
	{
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
		{
			TempInterleavedBuffer[FrameIndex * NumChannels + ChannelIndex] = InData[ChannelIndex][FrameIndex];
		}
	}

	if (!FMath::IsNearlyEqual(SampleRateRatio, 1.0f))
	{
		UE_LOG(LogCriWareAtom, VeryVerbose, TEXT("RackSubmixListener: Sample rate conversion ratio is : %f."), SampleRateRatio);
		AudioResampler.SetSampleRateRatio(SampleRateRatio);

		int32 NumConvertedFrames = NumSamples * SampleRateRatio;
		int32 OutNumFrames = INDEX_NONE;
		
		SampleRateConversionBuffer.Reset();
		SampleRateConversionBuffer.AddZeroed(NumConvertedFrames * NumChannels);

		int32 ErrorCode = AudioResampler.ProcessAudio(TempInterleavedBuffer.GetData(), NumFrames, false, SampleRateConversionBuffer.GetData(), NumConvertedFrames, OutNumFrames);
		ensure(OutNumFrames <= NumConvertedFrames);
		if (ErrorCode != 0)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("RackSubmixListener: Problem occured resampling Atom rack PCM data."));
			ensure(false);
		}
		// due to resampler implementation details, it's possible to have the number of output samples not match up to the number of requested samples. We shrink the SRC buffer in that case
		UE_LOG(LogCriWareAtom, VeryVerbose, TEXT("RackSubmixListener: Calculated SRC buffer size: %i. Actual output frames: %i."), NumConvertedFrames, OutNumFrames);
		SampleRateConversionBuffer.SetNum(OutNumFrames * NumChannels);

		return PushData(SampleRateConversionBuffer);
	}

	return PushData(TempInterleavedBuffer);
}

int32 FAtomRackSubmixListener::FAtomRackSubmixBuffer::PushData(TArray<float> InData)
{
	return Buffer.Push(InData.GetData(), InData.Num());
}

int32 FAtomRackSubmixListener::FAtomRackSubmixBuffer::PopData(int32 NumRequestedSamples, float* OutBuffer)
{
	return Buffer.Pop(OutBuffer, NumRequestedSamples);
}

void FAtomRackSubmixListener::FAtomRackSubmixBuffer::Reset(int InNumChannels, float InSampleRateRatio)
{
	if (InNumChannels != NumChannels)
	{
		AudioResampler.Init(Audio::EResamplingMethod::Linear, 1.0f, InNumChannels);
	}

	NumChannels = InNumChannels;
	SampleRateRatio = InSampleRateRatio;
	Buffer.SetCapacity(NumChannels * BufferParams.ReceivingBufferSizeInFrames * BufferParams.RackToSubmixBufferRatio * SampleRateRatio);
	Buffer.PushZeros(NumChannels * BufferParams.ReceivingBufferSizeInFrames * BufferParams.InitialSilenceFillRatio * SampleRateRatio);
}

bool FAtomRackSubmixListener::FAtomRackSubmixBuffer::IsEmpty() const
{
	return Buffer.Remainder() == Buffer.GetCapacity();
}

bool FAtomRackSubmixListener::FAtomRackSubmixBuffer::IsFull() const
{
	return Buffer.Remainder() == 0;
}
