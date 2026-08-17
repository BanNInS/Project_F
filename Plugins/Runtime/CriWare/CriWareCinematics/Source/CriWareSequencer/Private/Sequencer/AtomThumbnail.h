/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareSequencer
 * File     : AtomThumbnail.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "MovieSceneSection.h"

// Forward Declarations
class UAtomSoundBase;
class FSlateShaderResource;
class FSlateTexture2DRHIRef;

/**
 *
 */
class FAtomThumbnail
	: public ISlateViewport
	, public TSharedFromThis<FAtomThumbnail>
{
public:
	FAtomThumbnail(UMovieSceneSection& InSection, TRange<float> InDrawRange, int InTextureSize, const FLinearColor& BaseColor, float InDisplayScale);
	~FAtomThumbnail();

	//~ ISlateViewport interface
	virtual FIntPoint GetSize() const override;
	virtual FSlateShaderResource* GetViewportRenderTargetTexture() const override;
	virtual bool RequiresVsync() const override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float DeltaTime) override;

	/** Returns whether this thumbnail has a texture to render */
	virtual bool ShouldRender() const { return TextureSize > 0; }

private:
	/** Generates the waveform preview and dumps it out to the OutBuffer */
	void GenerateWaveformPreview(TArray<uint8>& OutData, TRange<float> DrawRange, float DisplayScale);

	/** Sample the audio data at the given lookup position. Appends the sample result to the Samples array */
	void SampleAudio(int32 NumChannels, const int16* LookupData, int32 LookupStartIndex, int32 LookupEndIndex, int32 LookupSize, int32 MaxAmplitude);

	/** Generate a natural cubic spline from the sample buffer */
	void GenerateSpline(int32 NumChannels, int32 SamplePositionOffset);

private:

	void DestroyTexture();

	/** A specific sample from the audio, specifying peak and average amplitude over the sample's range */
	struct FAudioSample
	{
		FAudioSample() : RMS(0.f), Peak(0), NumSamples(0) {}

		float RMS;
		int32 Peak;
		int32 NumSamples;
	};

	/** A segment in a cubic spline */
	struct FSplineSegment
	{
		FSplineSegment() : A(0.f), B(0.f), C(0.f), D(0.f) {}

		/** Cubic polynomial coefficients for the equation f(x) = A + Bx + Cx^2 + Dx^3*/
		float A, B, C, D;
		/** The width of this segment */
		float SampleSize;
		/** The x-position of this segment */
		float Position;
	};

	/** The maximum number of channels we support */
	static const int32 MaxSupportedChannels = 2;
	/** The number of pixels between which to place control points for cubic interpolation */
	static const int32 SmoothingAmount = 6;
	/** The size of the sroked border of the audio wave */
	static const int32 StrokeBorderSize = 2;

	/** The section we are visualizing */
	UMovieSceneSection& Section;

	/** The Texture RHI that holds the thumbnail */
	FSlateTexture2DRHIRef* Texture;

	/** Size of the texture */
	int32 TextureSize;

	/** */
	TRange<float> DrawRange;

	/** */
	float DisplayScale;

	/** Accumulation of audio samples for each channel */
	TArray<FAudioSample> Samples[MaxSupportedChannels];

	/** Spline segments generated from the above Samples array */
	TArray<FSplineSegment> SplineSegments[MaxSupportedChannels];

	/** Waveform colors */
	FLinearColor BoundaryColorHSV;
	FLinearColor FillColor_A, FillColor_B;
};
