/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBaseThumbnailRenderer.cpp
 *
 ****************************************************************************/

#include "AtomSoundBaseThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"

#include "Atom/AtomSoundBase.h"

bool UAtomSoundBaseThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	UAtomSoundBase* AtomSound = Cast<UAtomSoundBase>(Object);
	if (!AtomSound)
	{
		return false;
	}

	return AtomSound->CanVisualizeAsset();
}

void UAtomSoundBaseThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UAtomSoundBase* AtomSound = Cast<UAtomSoundBase>(Object);
	if (AtomSound != nullptr /*&& AtomSound->Validate()*/)
	{
		if (AtomSound->RawSnapshot.GetBulkDataSize() > 0)
		{
			AtomSound->SetRedrawThumbnail(false);

			// Canvas line item to draw with
			FCanvasLineItem LineItem;
			LineItem.SetColor(AtomSound->SoundColor);

			uint16* RawWaveData = (uint16*)AtomSound->RawSnapshot.Lock(LOCK_READ_ONLY);
			int32 RawDataSize = AtomSound->RawSnapshot.GetBulkDataSize();

			// Compute the scaled y-value used to render the channel data
			const float SampleYScale = Height / (2.f * 32767 * AtomSound->RawSnapshotNumChannels);

			// Sample count
			const uint32 NumChannels = AtomSound->RawSnapshotNumChannels;
			const uint32 TotalFrameCount = 512; // actually fixed to 512
			const uint32 TotalSampleCount = TotalFrameCount * NumChannels;
			const uint32 FramesPerPixel = TotalFrameCount / Width;
			const float FrameScale = (float)TotalFrameCount / Width;

			// Get the sample data of this file
			const int16* SamplePtr = reinterpret_cast<const int16*>(RawWaveData);

			// Render each channel separately so outer loop is the sound wave channel index.
			// Note: for multi-channel files this will always be 1-channel (mono).
			for (uint32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				// Reset the current frame count as we're starting from the beginning of the file to
				// render the channel data
				uint32 CurrentFrameCount = 0;

				// Loop through each pixel (in x direction)
				for (uint32 PixelIndex = 0; PixelIndex < Width; ++PixelIndex)
				{
					// reset the sample sum and num samples in pixel for each pixel
					int64 SampleSum = 0;
					int32 NumSamplesInPixel = 0;

					if (FramesPerPixel > 0)
					{
						// resampling

						// Loop through all pixels in this x-frame, sum all audio data. Track total frames rendered to avoid writing past buffer boundary
						for (uint32 PixelFrameIndex = 0; PixelFrameIndex < FramesPerPixel && CurrentFrameCount < TotalFrameCount; ++PixelFrameIndex)
						{
							// Get the sample value of the wave file
							const uint32 SampleIndex = CurrentFrameCount * NumChannels + ChannelIndex;
							check(SampleIndex < TotalSampleCount);
							int16 SampleValue = SamplePtr[SampleIndex];

							// Sum the sample value with the running sum
							SampleSum += FMath::Abs(SampleValue);

							// Track the number of samples we're actually summing to get an accurate average
							++NumSamplesInPixel;

							// Increment the frame after processing channels
							++CurrentFrameCount;
						}
					}
					else
					{
						// interpolation to nearest

						// Get the sample value of the wave file
						const uint32 SampleIndex = (uint32)(CurrentFrameCount * FrameScale) * NumChannels + ChannelIndex;
						check(SampleIndex < TotalSampleCount);
						int16 SampleValue = SamplePtr[SampleIndex];

						// copy the sample value
						SampleSum = FMath::Abs(SampleValue);

						// Only one sample is added per pixel
						NumSamplesInPixel = 1;

						// Increment the frame after processing channels
						++CurrentFrameCount;
					}

					// If we actually added any audio data in this pixel
					if (NumSamplesInPixel > 0)
					{
						const float AverageSampleValue = (float)SampleSum / NumSamplesInPixel;
						const float AverageSampleValueScaled = AverageSampleValue * SampleYScale;

						// Don't try to draw anything if the audio data was too quiet
						if (AverageSampleValueScaled > 0.001f)
						{
							// Draw vertical line mirrored around x-axis for channel equal to average sample value height
							const float YCenter = Y + ((2 * ChannelIndex) + 1) * Height / (2.f * NumChannels);
							LineItem.Draw(Canvas, FVector2D(X + PixelIndex, YCenter - AverageSampleValueScaled), FVector2D(X + PixelIndex, YCenter + AverageSampleValueScaled));
						}
					}
				}
			}

			AtomSound->RawSnapshot.Unlock();
		}

		// overlays
		FString TypeChars = ToString(AtomSound->WaveInfo.Format);
		int32 TypeWidth = 0;
		int32 TypeHeight = 0;
		StringSize(GEngine->GetLargeFont(), TypeWidth, TypeHeight, *TypeChars);
		float PaddingX = Width / 128.0f;
		float PaddingY = Height / 128.0f;
		float ScaleX = Width / 64.0f; //Text is 1/64'th of the size of the thumbnails
		float ScaleY = Height / 64.0f;
		// Type/Time overlay
		FCanvasTextItem TextItem(FVector2D(Width - PaddingX - TypeWidth * ScaleX, Height - PaddingY - TypeHeight * ScaleY), FText::FromString(TypeChars), GEngine->GetLargeFont(), FLinearColor::White);
		TextItem.EnableShadow(FLinearColor::Black);
		TextItem.Scale = FVector2D(ScaleX, ScaleY);
		TextItem.Draw(Canvas);
	}
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
EThumbnailRenderFrequency UAtomSoundBaseThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	if (UAtomSoundBase* AtomSound = Cast<UAtomSoundBase>(Object))
	{
		return AtomSound->CanVisualizeAsset()
			? EThumbnailRenderFrequency::Realtime
			: EThumbnailRenderFrequency::OnPropertyChange;
	}

	return EThumbnailRenderFrequency::OnPropertyChange;
}
#else
bool UAtomSoundBaseThumbnailRenderer::AllowsRealtimeThumbnails(UObject* Object) const
{
	UAtomSoundBase* AtomSound = Cast<UAtomSoundBase>(Object);
	if (AtomSound)
	{
		return AtomSound->CanVisualizeAsset();
	}
	return false;
}
#endif