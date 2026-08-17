/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomRackSubmixListener.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "ISubmixBufferListener.h"
#include "AudioResampler.h"
#include "Atom/AtomUtility.h"

class USoundSubmix;

/** SubmixBufferListener used to Mix Atom sound rack output with unreal sound submix. */
class FAtomRackSubmixListener
	: public ISubmixBufferListener
{
public:
	
	FAtomRackSubmixListener(int32 InRackID, int32 InRackNumChannels, int32 InRackSampleRate, USoundSubmix* InSubmix = nullptr);
	virtual ~FAtomRackSubmixListener();

	void RegisterSubmixListener();

	// ISubmixBufferListener
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;
	virtual bool IsRenderingAudio() const override;
	// ~ISubmixBufferListener

	struct FBufferParams
	{
		FBufferParams() {}
		
		const int32 ReceivingBufferSizeInFrames = 1024;
		const float RackToSubmixBufferRatio = 3.0f;
		const float InitialSilenceFillRatio = 1.0f;
	};

private:

	void UnregisterSubmixListener();

	class FAtomRackSubmixBuffer
	{
	public:

		FAtomRackSubmixBuffer(int32 InitialNumChannels = 2, FBufferParams&& Params = FBufferParams());

		int32 EnqueueData(TArray<TArray<float>>& AtomData, int32 NumChannels, int NumFrames, float SampleRateRatio = 1.0f);
		int32 PushData(TArray<float> InData);
		int32 PopData(int32 NumResquestedSamples, float* OutBuffer);
		int32 GetRemainder() const { return Buffer.Remainder(); }
		void Reset(int NumChannels, float SampleRateRatio = 1.0f);
		bool IsEmpty() const;
		
	private:
		
		bool IsFull() const;
		
		FBufferParams BufferParams;
		Atom::FCircularSampleBuffer Buffer;
		Audio::FResampler AudioResampler;
		int32 NumChannels;
		float SampleRateRatio;

		TArray<float> TempInterleavedBuffer;
		TArray<float> SampleRateConversionBuffer;
	};

	FAtomRackSubmixBuffer RackBuffer;
	TArray<TArray<float>> Buffers;
	TArray<float*> OutputBuffers;

	int32 RackID;
	int32 RackNumChannels;
	int32 RackSampleRate;
	USoundSubmix* Submix;
};