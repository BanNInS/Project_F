// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AtomSpectrumAnalyzer.h"
#include "Extensions/IAtomAnalyzerInterface.h"

#define CRI_API CRIWAREATOMWIDGETS_API

// Forward Declarations
namespace Audio
{ 
	template <typename InSampleType> class TSlidingBuffer;
}

namespace Atom
{
	/**
	 * Contains settings for Spectrum analyzer.
	 */
	class FSpectrumAnalysisSettings : public IAnalyzerSettings, public FSpectrumAnalyzerSettings
	{	
	public:
		/** Number of seconds between spectrum measurements */
		float AnalysisPeriod = 0.01f;
	};

	/**
	 * Holds the spectrum results per a time step for each channel
	 */
	struct FSpectrumEntry
	{
		int32 Channel = 0;
		float Timestamp = 0.0f;
		TArray<float> SpectrumValues;
	};

	/** 
	 * FAtomSpectrumResult contains the temporal evolution of loudness for the last buffer of audio analyzed.
	 */
	class FSpectrumResult : public IAnalyzerResult
	{
	public:
		FSpectrumResult() {}

		/** Appends an FAtomSpectrumEntry to the container. */
		CRI_API  void Add(FSpectrumEntry&& InEntry);

		/** Returns const reference to FAtomSpectrumEntry array for individual channel. */
		CRI_API const TArray<FSpectrumEntry>& GetChannelSpectrumArray(int32 ChannelIdx) const;

		/** Returns the number of channels. */
		CRI_API int32 GetNumChannels() const;

	private:
		float DurationInSeconds = 0.0f;
		TMap<int32, TArray<FSpectrumEntry>> ChannelSpectrumArrays;
	};

	/**
	 * FAtomSpectrumWorker performs Spectrum analysis on input sample buffers.
	 */
	class FSpectrumAnalysisWorker : public IAnalyzerWorker
	{
	public:
		/** Construct a worker */
		CRI_API FSpectrumAnalysisWorker(const FAnalyzerParameters& InParams, const FSpectrumAnalysisSettings& InAnalyzerSettings);

		/** Analyzes input sample buffer and updates result. */
		CRI_API virtual void Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult) override;

	private:

		int32 NumChannels = 0;
		bool bDownmixToMono = true;
		int32 NumOutputFrames = 0;
		int32 NumWindowFrames = 0;
		int32 NumWindowSamples = 0;
		int32 NumHopFrames = 0;
		int32 SampleRate = 0;
		int64 FrameCounter = 0;

		Audio::FAlignedFloatBuffer MonoBuffer; 
		Audio::FAlignedFloatBuffer ChannelBuffer;

		TArray<float> InternalWindow;
		TUniquePtr<Audio::TSlidingBuffer<float>> InternalBuffer;
		TUniquePtr<FSpectrumAnalyzer> SpectrumAnalyzer;
	};

	/**
	 * Defines the Spectrum analyzer and creates related classes.
	 */
	class FSpectrumAnalysisFactory : public IAnalyzerFactory
	{
	public:

		/** Name of specific analyzer type. */
		CRI_API virtual FName GetName() const override;

		/** Human readable name of analyzer. */
		CRI_API virtual FString GetTitle() const override;

		/** Creates a new FAtomSpectrumResult */
		CRI_API virtual TUniquePtr<IAnalyzerResult> NewResult() const override;

		/**  Creates a new FAtomSpectrumAnalysisWorker. This expects IAnalyzerNRTSettings to be a valid pointer to a FAtomSpectrumAnalysisSettings object. */
		CRI_API virtual TUniquePtr<IAnalyzerWorker> NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const override;
	};
}

#undef CRI_API
