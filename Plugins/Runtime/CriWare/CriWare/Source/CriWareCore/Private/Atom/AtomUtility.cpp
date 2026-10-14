
#include "Atom/AtomUtility.h"

#include "HAL/UnrealMemory.h"
#include "DSP/FloatArrayMath.h"

namespace Atom
{
	void Interleave(const float** Buffer, FAlignedFloatBuffer& OutBuffer, int32 NumChannels, int32 NumFrames)
	{
		const int NumSamples = NumFrames * NumChannels;

		OutBuffer.Reset();
		OutBuffer.AddUninitialized(NumSamples);

		if (NumChannels == 1)
		{
			FMemory::Memcpy(OutBuffer.GetData(), Buffer[0], NumFrames * sizeof(float));
		}
		else if (NumChannels == 2 && (NumFrames & 0b11) == 0) // Must be a multiple of 4 for vectorization
		{
			Audio::BufferInterleave2ChannelFast(Buffer[0], Buffer[1], OutBuffer.GetData(), NumFrames);
		}
		else
		{
			int32 SampleIndex = 0;
			for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
			{
				for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
				{
					OutBuffer[SampleIndex++] = Buffer[ChannelIndex][FrameIndex];
				}
			}
		}
	}

	void Deinterleave(const FAlignedFloatBuffer& Buffer, float** OutBuffer, int32 NumChannels, int32 NumFrames)
	{
		if (NumChannels == 1)
		{
			FMemory::Memcpy(OutBuffer[0], Buffer.GetData(), NumFrames * sizeof(float));
		}
		else if (NumChannels == 2 && (NumFrames & 0b11) == 0) // Must be a multiple of 4 for vectorization
		{
			Audio::BufferDeinterleave2ChannelFast(Buffer.GetData(), OutBuffer[0], OutBuffer[1], NumFrames);
		}
		else
		{
			int32 SampleIndex = 0;
			for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
			{
				for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
				{
					OutBuffer[ChannelIndex][FrameIndex] = Buffer[SampleIndex++];
				}
			}
		}
	}
} // namespace
