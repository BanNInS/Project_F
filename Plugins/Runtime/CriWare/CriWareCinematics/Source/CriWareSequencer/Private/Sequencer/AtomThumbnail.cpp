/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareSequencer
 * File     : AtomThumbnail.cpp
 *
 ****************************************************************************/

#include "AtomThumbnail.h"

#include "RenderUtils.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Slate/SlateTextures.h"
#include "MovieScene.h"
#include "MovieSceneToolHelpers.h"
#include "Editor.h"

#include "Atom/AtomSoundBase.h"

#include "MovieSceneAtomTrack.h"
#include "MovieSceneAtomSection.h"

#include "CriWareSequencerPrivate.h"

#define LOCTEXT_NAMESPACE "FAtomThumbnail"

namespace AnimatableAtomEditorConstants
{
	// Optimization - maximum samples per pixel this sound allows
	const uint32 MaxSamplesPerPixel = 60;
}

float Modulate(float Value, float Delta, float Range)
{
	Value = FMath::Fmod(Value + Delta, Range);
	if (Value < 0.0f)
	{
		Value += Range;
	}
	return Value;
}

FAtomThumbnail::FAtomThumbnail(UMovieSceneSection& InSection, TRange<float> InDrawRange, int32 InTextureSize, const FLinearColor& BaseColor, float InDisplayScale)
	: Section(InSection)
	, Texture(nullptr)
	, TextureSize(InTextureSize)
	, DrawRange(InDrawRange)
	, DisplayScale(InDisplayScale)
{
	UMovieSceneAtomSection* AtomSection = Cast<UMovieSceneAtomSection>(&Section);

	const FLinearColor BaseHSV = BaseColor.LinearRGBToHSV();

	const float BaseValue = FMath::Min(BaseHSV.B, .5f) * BaseHSV.A;
	const float BaseSaturation = FMath::Max((BaseHSV.G - .45f), 0.f) * BaseHSV.A;

	FillColor_A = FLinearColor(Modulate(BaseHSV.R, -2.5f, 360), BaseSaturation + .35f, BaseValue);
	FillColor_B = FLinearColor(Modulate(BaseHSV.R, 2.5f, 360), BaseSaturation + .4f, BaseValue + .15f);

	BoundaryColorHSV = FLinearColor(BaseHSV.R, BaseSaturation, BaseValue + .35f);

	if (ShouldRender())
	{
		// ask to uncompress and generate raw PCM Data
		if (UAtomSoundBase* Sound = AtomSection->GetSound())
		{
			if (Sound->RawPCMData == nullptr)
			{
				Sound->GenerateWaveRawData();
				return;
			}
		}

		uint32 Size = GetSize().X * GetSize().Y * GPixelFormats[PF_B8G8R8A8].BlockBytes;
		TArray<uint8> RawData;
		RawData.AddZeroed(Size);

		GenerateWaveformPreview(RawData, DrawRange, DisplayScale);
		
		FSlateTextureDataPtr BulkData(new FSlateTextureData(GetSize().X, GetSize().Y, GPixelFormats[PF_B8G8R8A8].BlockBytes, RawData));

		Texture = new FSlateTexture2DRHIRef(GetSize().X, GetSize().Y, PF_B8G8R8A8, BulkData, TexCreate_Dynamic);

		BeginInitResource(Texture);
	}
}

void FAtomThumbnail::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float DeltaTime)
{
	// retry to generate 
	if (ShouldRender() && Texture == nullptr)
	{
		UMovieSceneAtomSection* AtomSection = Cast<UMovieSceneAtomSection>(&Section);

		// check if uncompress and generate raw PCM Data
		if (UAtomSoundBase* Sound = AtomSection->GetSound())
		{
			if (Sound->RawPCMData == nullptr)
			{
				return;
			}
		}

		uint32 Size = GetSize().X * GetSize().Y * GPixelFormats[PF_B8G8R8A8].BlockBytes;
		TArray<uint8> RawData;
		RawData.AddZeroed(Size);

		GenerateWaveformPreview(RawData, DrawRange, DisplayScale);

		FSlateTextureDataPtr BulkData(new FSlateTextureData(GetSize().X, GetSize().Y, GPixelFormats[PF_B8G8R8A8].BlockBytes, RawData));

		Texture = new FSlateTexture2DRHIRef(GetSize().X, GetSize().Y, PF_B8G8R8A8, BulkData, TexCreate_Dynamic);

		BeginInitResource(Texture);
	}
}

FAtomThumbnail::~FAtomThumbnail()
{
	DestroyTexture();
}

void FAtomThumbnail::DestroyTexture()
{
	if (Texture)
	{
		// UE-114425: Defer the destroy until the next tick to work around the RHI getting destroyed before the render command completes.
		FSlateTexture2DRHIRef* InTexture = Texture;

		Texture = nullptr;

		GEditor->GetTimerManager()->SetTimerForNextTick([this, InTexture]()
		{
			ENQUEUE_RENDER_COMMAND(DestroyTexture)(
				[InTexture](FRHICommandList& RHICmdList)
				{
					if (InTexture)
					{
						InTexture->ReleaseResource();
						delete InTexture;
					}
				}
			);
		});
	}
}

FIntPoint FAtomThumbnail::GetSize() const
{
	return FIntPoint(TextureSize, Section.GetTypedOuter<UMovieSceneAtomTrack>()->GetRowHeight());
}

FSlateShaderResource* FAtomThumbnail::GetViewportRenderTargetTexture() const
{
	return Texture;
}

bool FAtomThumbnail::RequiresVsync() const
{ 
	return false;
}

/** Lookup a pixel in the given data buffer based on the specified X and Y */
uint8* LookupPixel(TArray<uint8>& Data, int32 X, int32 YPos, int32 Width, int32 Height, int32 Channel, int32 NumChannels)
{
	int32 Y = Height - YPos - 1;
	if (NumChannels == 2)
	{
		Y = Channel == 0 ? Height / 2 - YPos : Height / 2 + YPos;
	}

	int32 Index = (Y * Width + X) * GPixelFormats[PF_B8G8R8A8].BlockBytes;
	return &Data[Index];
}

/** Lerp between 2 HSV space colors */
FLinearColor LerpHSV(const FLinearColor& A, const FLinearColor& B, float Alpha)
{
	float SrcHue = A.R;
	float DestHue = B.R;

	// Take the shortest path to the new hue
	if (FMath::Abs(SrcHue - DestHue) > 180.0f)
	{
		if (DestHue > SrcHue)
		{
			SrcHue += 360.0f;
		}
		else
		{
			DestHue += 360.0f;
		}
	}

	float NewHue = FMath::Fmod(FMath::Lerp(SrcHue, DestHue, Alpha), 360.0f);
	if (NewHue < 0.0f)
	{
		NewHue += 360.0f;
	}

	return FLinearColor(
		NewHue,
		FMath::Lerp(A.G, B.G, Alpha),
		FMath::Lerp(A.B, B.B, Alpha),
		FMath::Lerp(A.A, B.A, Alpha)
	);
}

void FAtomThumbnail::GenerateWaveformPreview(TArray<uint8>& OutData, TRange<float> InDrawRange, float InDisplayScale)
{
	UMovieSceneAtomSection* AtomSection = Cast<UMovieSceneAtomSection>(&Section);

	UAtomSoundBase* Sound = AtomSection->GetSound();
	check(Sound);

	check(Sound->WaveInfo.NumChannels == 1 || Sound->WaveInfo.NumChannels == 2);

	// If this Sound is generated procedurally, it's not possible to render a thumbnail.
	if (Sound->bProcedural)
	{
		return;
	}

	uint32 SampleRate;
	uint16 NumChannels;
	TArray<uint8> RawPCMData;
	if (!Sound->GetImportedSoundWaveData(RawPCMData, SampleRate, NumChannels))
	{
		UE_LOG(LogMovieScene, Warning, TEXT("Failed to get sound wave data for: %s Waveform will not be visible."), *Sound->GetPathName());
		return;
	}

	const int16* LookupData = (int16*)RawPCMData.GetData();
	const int32 LookupSize = RawPCMData.Num() * sizeof(uint8) / sizeof(int16);

	if (!LookupData || !AtomSection->HasStartFrame() || !AtomSection->HasEndFrame())
	{
		return;
	}

	FFrameRate FrameRate = AtomSection->GetTypedOuter<UMovieScene>()->GetTickResolution();
	float      PitchMultiplierValue = AtomSection->GetPitchMultiplierChannel().GetDefault().Get(1.f);
	double     SectionStartTime = AtomSection->GetInclusiveStartFrame() / FrameRate;

	// the length of the waveform without considering loops
	float UnloopedDuration = Sound->WaveInfo.GetDuration();

	// @todo Sequencer This fixes looping drawing by pretending we are only dealing with a SoundWave
	TRange<float> AudioTrueRange = TRange<float>(
		SectionStartTime - FrameRate.AsSeconds(AtomSection->GetStartOffset()),
		SectionStartTime - FrameRate.AsSeconds(AtomSection->GetStartOffset()) + UnloopedDuration * (1.0f / PitchMultiplierValue));

	float TrueRangeSize = AudioTrueRange.Size<float>();
	float DrawRangeSize = InDrawRange.Size<float>();

	const int32 MaxAmplitude = NumChannels == 1 ? GetSize().Y : GetSize().Y / 2;
	const int32 DrawOffsetPx = FMath::Max(FMath::RoundToInt((InDrawRange.GetLowerBoundValue() - SectionStartTime) / InDisplayScale), 0);

	// In order to prevent flickering waveforms when moving the display position/range around, we have to lock our sample position and spline segments to the view range
	const float RangeLookupFraction = (SmoothingAmount * InDisplayScale) / TrueRangeSize;
	const int32 LookupRange = FMath::Clamp(FMath::TruncToInt(RangeLookupFraction * LookupSize), 1, LookupSize);

	const int32 SampleLockOffset = DrawOffsetPx % SmoothingAmount;

	const FIntPoint ThumbnailSize = GetSize();
	const int32 FirstSample = -2.f * SmoothingAmount - SampleLockOffset;
	const int32 LastSample = ThumbnailSize.X + 2.f * SmoothingAmount;

	// Sample the audio one pixel to the left and right
	for (int32 X = FirstSample; X < LastSample; ++X)
	{
		float LookupTime = ((float)(X - 0.5f) / (float)ThumbnailSize.X) * DrawRangeSize + InDrawRange.GetLowerBoundValue();
		float LookupFraction = (LookupTime - AudioTrueRange.GetLowerBoundValue()) / TrueRangeSize;
		float LookupFractionLooping = FMath::Fmod(LookupFraction, 1.f);
		int32 LookupIndex = FMath::TruncToInt(LookupFractionLooping * LookupSize);
		
		float NextLookupTime = ((float)(X + 0.5f) / (float)ThumbnailSize.X) * DrawRangeSize + InDrawRange.GetLowerBoundValue();
		float NextLookupFraction = (NextLookupTime - AudioTrueRange.GetLowerBoundValue()) / TrueRangeSize;
		float NextLookupFractionLooping = FMath::Fmod(NextLookupFraction, 1.f);
		int32 NextLookupIndex = FMath::TruncToInt(NextLookupFractionLooping * LookupSize);
		
		if (!AtomSection->GetLooping() && LookupFraction > 1.f)
		{
			break;
		}

		SampleAudio(Sound->WaveInfo.NumChannels, LookupData, LookupIndex, NextLookupIndex, LookupSize, MaxAmplitude);
	}

	// Generate a spline
	GenerateSpline(Sound->WaveInfo.NumChannels, FirstSample);

	// Now draw the spline
	const int32 Width = ThumbnailSize.X;
	const int32 Height = ThumbnailSize.Y;

	FLinearColor BoundaryColor = BoundaryColorHSV.HSVToLinearRGB();

	for (int32 ChannelIndex = 0; ChannelIndex < Sound->WaveInfo.NumChannels; ++ChannelIndex)
	{
		int32 SplineIndex = 0;

		for (int32 X = 0; X < Width; ++X)
		{
			bool bOutOfRange = SplineIndex >= SplineSegments[ChannelIndex].Num();
			while (!bOutOfRange && X >= SplineSegments[ChannelIndex][SplineIndex].Position+SplineSegments[ChannelIndex][SplineIndex].SampleSize)
			{
				++SplineIndex;
				bOutOfRange = SplineIndex >= SplineSegments[ChannelIndex].Num();
			}
			
			if (bOutOfRange)
			{
				break;
			}

			// Evaluate the spline
			const float DistBetweenPts = (X-SplineSegments[ChannelIndex][SplineIndex].Position)/SplineSegments[ChannelIndex][SplineIndex].SampleSize;
			const float Amplitude = 
				SplineSegments[ChannelIndex][SplineIndex].A +
				SplineSegments[ChannelIndex][SplineIndex].B * DistBetweenPts +
				SplineSegments[ChannelIndex][SplineIndex].C * FMath::Pow(DistBetweenPts, 2) +
				SplineSegments[ChannelIndex][SplineIndex].D * FMath::Pow(DistBetweenPts, 3);

			// @todo: draw border according to gradient of curve to prevent aliasing on steep gradients? This would be non-trivial...
			const float BoundaryStart = Amplitude - StrokeBorderSize * 0.5f;
			const float BoundaryEnd = Amplitude + StrokeBorderSize * 0.5f;

			const FAudioSample& Sample = Samples[ChannelIndex][X - FirstSample];

			for (int32 PixelIndex = 0; PixelIndex < MaxAmplitude; ++PixelIndex)
			{
				uint8* Pixel = LookupPixel(OutData, X, PixelIndex, Width, Height, ChannelIndex, NumChannels);

				const float PixelCenter = PixelIndex + 0.5f;

				const float Dither = FMath::FRand() * .025f - .0125f;
				const float GradLerp = FMath::Clamp(float(PixelIndex) / MaxAmplitude + Dither, 0.f, 1.f);
				FLinearColor SolidFilledColor = LerpHSV(FillColor_A, FillColor_B, GradLerp);

				float BorderBlend = 1.f;
				if (PixelIndex <= FMath::TruncToInt(BoundaryStart))
				{
					BorderBlend = 1.f - FMath::Clamp(BoundaryStart - PixelIndex, 0.f, 1.f);
				}
				
				FLinearColor Color = PixelIndex == Sample.Peak ? FillColor_B.HSVToLinearRGB() : LerpHSV(SolidFilledColor, BoundaryColorHSV, BorderBlend).HSVToLinearRGB();

				// Calculate alpha based on how far from the boundary we are
				float Alpha = FMath::Max(FMath::Clamp(BoundaryEnd - PixelCenter, 0.f, 1.f), FMath::Clamp(float(Sample.Peak) - PixelIndex + 0.25f, 0.f, 1.f));
				if (Alpha <= 0.f)
				{
					break;
				}

				// Slate viewports must have pre-multiplied alpha
				*Pixel++ = Color.B*Alpha*255;
				*Pixel++ = Color.G*Alpha*255;
				*Pixel++ = Color.R*Alpha*255;
				*Pixel++ = Alpha*255;
			}
		}
	}
}

void FAtomThumbnail::GenerateSpline(int32 NumChannels, int32 SamplePositionOffset)
{
	// Generate a cubic polynomial spline interpolating the samples
	for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
	{
		TArray<FSplineSegment>& Segments = SplineSegments[ChannelIndex];

		const int32 NumSamples = Samples[ChannelIndex].Num();

		struct FControlPoint
		{
			float Value;
			float Position;
			int32 SampleSize;
		};
		TArray<FControlPoint> ControlPoints;

		for (int SampleIndex = 0; SampleIndex < NumSamples; SampleIndex += SmoothingAmount)
		{
			float RMS = 0.f;
			int32 NumAvgs = FMath::Min(SmoothingAmount, NumSamples - SampleIndex);
			
			for (int32 SubIndex = 0; SubIndex < NumAvgs; ++SubIndex)
			{
				RMS += FMath::Pow(Samples[ChannelIndex][SampleIndex + SubIndex].RMS, 2);
			}

			const int32 SegmentSize2 = NumAvgs / 2;
			const int32 SegmentSize1 = NumAvgs - SegmentSize2;

			RMS = FMath::Sqrt(RMS / NumAvgs);

			FControlPoint& StartPoint = ControlPoints[ControlPoints.AddZeroed()];
			StartPoint.Value = Samples[ChannelIndex][SampleIndex].RMS;
			StartPoint.SampleSize = SegmentSize1;
			StartPoint.Position = SampleIndex + SamplePositionOffset;

			if (SegmentSize2 > 0)
			{
				FControlPoint& MidPoint = ControlPoints[ControlPoints.AddZeroed()];
				MidPoint.Value = RMS;
				MidPoint.SampleSize = SegmentSize2;
				MidPoint.Position = SampleIndex + SamplePositionOffset + SegmentSize1;
			}
		}

		if (ControlPoints.Num() <= 1)
		{
			continue;
		}

		const int32 LastIndex = ControlPoints.Num() - 1;

		// Perform gaussian elimination on the following tridiagonal matrix that defines the piecewise cubic polynomial
		// spline for n control points, given f(x), f'(x) and f''(x) continuity. Imposed boundary conditions are f''(0) = f''(n) = 0.
		//	(D[i] = f[i]'(x))
		//	1	2						D[i]	= 3(y[1] - y[0])
		//	1	4	1					D[i+1]	= 3(y[2] - y[1])
		//		1	4	1				|		|
		//		\	\	\	\	\		|		|
		//					1	4	1	|		= 3(y[n-1] - y[n-2])
		//						1	2	D[n]	= 3(y[n] - y[n-1])
		struct FMinimalMatrixComponent
		{
			float DiagComponent;
			float KnownConstant;
		};

		TArray<FMinimalMatrixComponent> GaussianCoefficients;
		GaussianCoefficients.AddZeroed(ControlPoints.Num());

		// Setup the top left of the matrix
		GaussianCoefficients[0].KnownConstant = 3.f * (ControlPoints[1].Value - ControlPoints[0].Value);
		GaussianCoefficients[0].DiagComponent = 2.f;

		// Calculate the diagonal component of each row, based on the eliminated value of the last
		for (int32 Index = 1; Index < GaussianCoefficients.Num() - 1; ++Index)
		{
			GaussianCoefficients[Index].KnownConstant = (3.f * (ControlPoints[Index+1].Value - ControlPoints[Index-1].Value)) - (GaussianCoefficients[Index-1].KnownConstant / GaussianCoefficients[Index-1].DiagComponent);
			GaussianCoefficients[Index].DiagComponent = 4.f - (1.f / GaussianCoefficients[Index-1].DiagComponent);
		}
		
		// Setup the bottom right of the matrix
		GaussianCoefficients[LastIndex].KnownConstant = (3.f * (ControlPoints[LastIndex].Value - ControlPoints[LastIndex-1].Value)) - (GaussianCoefficients[LastIndex-1].KnownConstant / GaussianCoefficients[LastIndex-1].DiagComponent);
		GaussianCoefficients[LastIndex].DiagComponent = 2.f - (1.f / GaussianCoefficients[LastIndex-1].DiagComponent);

		// Now we have an upper triangular matrix, we can use reverse substitution to calculate D[n] -> D[0]

		TArray<float> FirstOrderDerivatives;
		FirstOrderDerivatives.AddZeroed(GaussianCoefficients.Num());

		FirstOrderDerivatives[LastIndex] = GaussianCoefficients[LastIndex].KnownConstant / GaussianCoefficients[LastIndex].DiagComponent;

		for (int32 Index = GaussianCoefficients.Num() - 2; Index >= 0; --Index)
		{
			FirstOrderDerivatives[Index] = (GaussianCoefficients[Index].KnownConstant - FirstOrderDerivatives[Index+1]) / GaussianCoefficients[Index].DiagComponent;
		}

		// Now we know the first-order derivatives of each control point, calculating the interpolating polynomial is trivial
		// f(x) = a + bx + cx^2 + dx^3
		//	a = y
		//	b = D[i]
		//	c = 3(y[i+1] - y[i]) - 2D[i] - D[i+1]
		//	d = 2(y[i] - y[i+1]) + 2D[i] + D[i+1]
		for (int32 Index = 0; Index < FirstOrderDerivatives.Num() - 2; ++Index)
		{
			Segments.Emplace();
			Segments.Last().A = ControlPoints[Index].Value;
			Segments.Last().B = FirstOrderDerivatives[Index];
			Segments.Last().C = 3.f*(ControlPoints[Index+1].Value - ControlPoints[Index].Value) - 2*FirstOrderDerivatives[Index] - FirstOrderDerivatives[Index+1];
			Segments.Last().D = 2.f*(ControlPoints[Index].Value - ControlPoints[Index+1].Value) + FirstOrderDerivatives[Index] + FirstOrderDerivatives[Index+1];

			Segments.Last().Position = ControlPoints[Index].Position;
			Segments.Last().SampleSize = ControlPoints[Index].SampleSize;
		}
	}
}

void FAtomThumbnail::SampleAudio(int32 NumChannels, const int16* LookupData, int32 LookupStartIndex, int32 LookupEndIndex, int32 LookupSize, int32 MaxAmplitude)
{
	LookupStartIndex = NumChannels == 2 ? (LookupStartIndex % 2 == 0 ? LookupStartIndex : LookupStartIndex - 1) : LookupStartIndex;
	LookupEndIndex = FMath::Max(LookupEndIndex, LookupStartIndex + 1);
	
	int32 StepSize = NumChannels;

	// optimization - don't take more than a maximum number of samples per pixel
	int32 Range = LookupEndIndex - LookupStartIndex;
	int32 SampleCount = Range / StepSize;
	int32 MaxSampleCount = AnimatableAtomEditorConstants::MaxSamplesPerPixel;
	int32 ModifiedStepSize = StepSize;
	
	if (SampleCount > MaxSampleCount)
	{
		// Always start from a common multiple
		int32 Adjustment = LookupStartIndex % MaxSampleCount;
		LookupStartIndex = FMath::Clamp(LookupStartIndex - Adjustment, 0, LookupSize);
		LookupEndIndex = FMath::Clamp(LookupEndIndex - Adjustment, 0, LookupSize);
		ModifiedStepSize *= (SampleCount / MaxSampleCount);
	}

	for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
	{
		FAudioSample& NewSample = Samples[ChannelIndex][Samples[ChannelIndex].Emplace()];

		for (int32 Index = LookupStartIndex; Index < LookupEndIndex; Index += ModifiedStepSize)
		{
			if (Index < 0 || Index >= LookupSize)
			{
				NewSample.RMS += 0.f;
				++NewSample.NumSamples;
				continue;
			}

			int32 DataPoint = LookupData[Index + ChannelIndex];
			int32 Sample = FMath::Clamp(FMath::TruncToInt(FMath::Abs(DataPoint) / 32768.f * MaxAmplitude), 0, MaxAmplitude - 1);

			NewSample.RMS += FMath::Pow(Sample, 2.f);
			NewSample.Peak = FMath::Max(NewSample.Peak, Sample);
			++NewSample.NumSamples;
		}

		if (NewSample.NumSamples)
		{
			NewSample.RMS = (FMath::Sqrt(NewSample.RMS / NewSample.NumSamples));
		}
	}
}

#undef LOCTEXT_NAMESPACE
