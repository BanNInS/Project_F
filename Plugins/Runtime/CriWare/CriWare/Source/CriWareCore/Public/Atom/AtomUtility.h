#pragma once

#include "Containers/Array.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "HAL/PlatformMath.h"
#include "Math/UnrealMathUtility.h"

// todo move to criware defines ?
#define ATOM_BUFFER_ALIGNMENT 16

// DSP utilites - from UE
namespace Atom
{
	/** Aligned allocator used for fast operations. */
	using FAtomBufferAlignedAllocator = TAlignedHeapAllocator<ATOM_BUFFER_ALIGNMENT>;

	using FAlignedByteBuffer = TArray<uint8, FAtomBufferAlignedAllocator>;
	using FAlignedFloatBuffer = TArray<float, FAtomBufferAlignedAllocator>;
	using FAlignedInt32Buffer = TArray<int32, FAtomBufferAlignedAllocator>;

	// Atom PCM to Unreal Audio PCM
	void Interleave(const float** Buffer, FAlignedFloatBuffer& OutBuffer, int32 NumChannels, int32 NumFrames);

	// Unreal Audio PCM to Atom PCM
	void Deinterleave(const FAlignedFloatBuffer& Buffer, float** OutBuffer, int32 NumChannels, int32 NumFrames);

	// Simple circular buffer
	// Uses 32bit wrap around logic explained here: 
	// https://www.snellman.net/blog/archive/2016-12-13-ring-buffers/
	class FCircularSampleBuffer
	{
	public:
		FCircularSampleBuffer(int32 InInitialCapacity)
		{
			SetCapacity(InInitialCapacity);
		}

		// Allow the caller to lock the CS ahead of calling (if necessary)
		//FCriticalSection& GetCriticialSection() { return CS; }
		//const FCriticalSection& GetCriticialSection() const { return CS; }

		// This doesn't preserve contents.
		void SetCapacity(int32 InCapacity)
		{
			check(InCapacity > 0);

			if (!FMath::IsPowerOfTwo(InCapacity))
			{
				InCapacity = FMath::RoundUpToPowerOfTwo(InCapacity);
			}

			//FScopeLock Lock(&CS);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			Buffer.SetNumZeroed(InCapacity, EAllowShrinking::Yes);
#else
			Buffer.SetNumZeroed(InCapacity, true /* Allow shrinking */);
#endif

			Mask = InCapacity - 1;
			Read = 0; // Empty.
			Write = 0;

			check(Mask != 0);
		}
		int32 GetCapacity() const
		{
			//FScopeLock Lock(&CS);
			return Buffer.Num();
		}
		int32 Num() const
		{
			//FScopeLock Lock(&CS);
			return Write - Read;
		}

		// Get number of samples that can be pushed onto the buffer before it is full.
		int32 Remainder() const
		{
			//FScopeLock Lock(&CS);
			return Buffer.Num() - Num();
		}

		int32 Push(const float* InBuffer, int32 InSize)
		{
			//FScopeLock Lock(&CS);
			int32 CanPush = FMath::Min(Remainder(), InSize);
			for (int32 i = 0; i < CanPush; ++i)
			{
				Enqueue(InBuffer[i]);
			}
			return CanPush;
		}
		int32 Pop(float* OutBuffer, int32 InNumSamples)
		{
			//FScopeLock Lock(&CS);
			int32 CanPop = FMath::Min(Num(), InNumSamples);
			for (int32 i = 0; i < CanPop; ++i)
			{
				OutBuffer[i] = Dequeue();
			}
			return CanPop;
		}

		int32 PushZeros(int32 InNumSamplesOfSilence)
		{
			//FScopeLock Lock(&CS);
			int32 CanPush = FMath::Min(GetCapacity(), InNumSamplesOfSilence);
			for (int32 i = 0; i < CanPush; ++i)
			{
				Enqueue(0.f);
			}
			return CanPush;
		}
	private:
		uint32 Read = 0;		// These grow indefinitely until wrap at 2^32,
		uint32 Write = 0;		// this allows us to use full capacity as write >= read.
		uint32 Mask = 0;
		TArray<float> Buffer;
		//mutable FCriticalSection CS;

		// NOTE: Not forceinline as compiler does better job without.
		void Enqueue(const float InFloat)
		{
			Buffer[Write++ & Mask] = InFloat;
		}
		float Dequeue()
		{
			return Buffer[Read++ & Mask];
		}
	};
} // namespace

// Atom utilites - from UE
namespace Atom
{
	// Low precision, high performance approximation of sine using parabolic polynomial approx
	// Valid on interval [-PI, PI]
	static FORCEINLINE float FastSin(const float X)
	{
		return (4.0f * X) / UE_PI * (1.0f - FMath::Abs(X) / UE_PI);
	}

	// Function to convert linear scale volume to decibels.
	static FORCEINLINE float ConvertToDecibels(const float InLinear, const float InFloor = UE_SMALL_NUMBER)
	{
		return 20.0f * FMath::LogX(10.0f, FMath::Max(InLinear, InFloor));
	}

	// Function to convert decibel to linear scale.
	static FORCEINLINE float ConvertToLinear(const float InDecibels)
	{
		return FMath::Pow(10.0f, InDecibels / 20.0f);
	}

	// Returns the frequency multiplier to scale a base frequency given the input semitones
	static FORCEINLINE float GetFrequencyMultiplier(const float InPitchSemitones)
	{
		if (InPitchSemitones == 0.0f)
		{
			return 1.0f;

		}
		return FMath::Pow(2.0f, InPitchSemitones / 12.0f);
	}

	// Returns the number of semitones relative to a base frequency given the input frequency multiplier
	static FORCEINLINE float GetSemitones(const float InMultiplier)
	{
		if (InMultiplier <= 0.0f)
		{
			return 12.0f * FMath::Log2(UE_SMALL_NUMBER);
		}
		return 12.0f * FMath::Log2(InMultiplier);
	}

	// Returns the log frequency of the input value. Maps linear domain and range values to log output (good for linear slider controlling frequency)
	static FORCEINLINE float GetLogFrequencyClamped(const float InValue, const FVector2D& Domain, const FVector2D& Range)
	{
		// Check if equal as well as less than to avoid round error in case where at edges.
		if (InValue <= Domain.X)
		{
			return UE_REAL_TO_FLOAT(Range.X);
		}

		if (InValue >= Domain.Y)
		{
			return UE_REAL_TO_FLOAT(Range.Y);
		}

		//Handle edge case of NaN
		if (FMath::IsNaN(InValue))
		{
			return UE_REAL_TO_FLOAT(Range.X);
		}

		const FVector2D RangeLog(FMath::Max(FMath::Loge(Range.X), UE_SMALL_NUMBER), FMath::Min(FMath::Loge(Range.Y), UE_BIG_NUMBER));
		const float FreqLinear = (float)FMath::GetMappedRangeValueUnclamped(Domain, RangeLog, (FVector2D::FReal)InValue);
		return FMath::Exp(FreqLinear);
	}

	// Returns the linear frequency of the input value. Maps log domain and range values to linear output (good for linear slider representation/visualization of log frequency). Reverse of GetLogFrequencyClamped.
	static FORCEINLINE float GetLinearFrequencyClamped(const float InFrequencyValue, const FVector2D& Domain, const FVector2D& Range)
	{
		// Check if equal as well as less than to avoid round error in case where at edges.
		if (InFrequencyValue <= Range.X)
		{
			return UE_REAL_TO_FLOAT(Domain.X);
		}

		if (InFrequencyValue >= Range.Y)
		{
			return UE_REAL_TO_FLOAT(Domain.Y);
		}

		//Handle edge case of NaN
		if (FMath::IsNaN(InFrequencyValue))
		{
			return UE_REAL_TO_FLOAT(Domain.X);
		}

		const FVector2D RangeLog(FMath::Max(FMath::Loge(Range.X), UE_SMALL_NUMBER), FMath::Min(FMath::Loge(Range.Y), UE_BIG_NUMBER));
		const FVector2D::FReal FrequencyLog = FMath::Loge(InFrequencyValue);
		return UE_REAL_TO_FLOAT(FMath::GetMappedRangeValueUnclamped(RangeLog, Domain, FrequencyLog));
	}
} // namespace
