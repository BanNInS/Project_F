
#pragma once

#include "AtomLoudnessAnalyzer.h"
#include "Extensions/IAtomAnalyzerInterface.h"

#define CRI_API CRIWAREATOMWIDGETS_API

namespace Audio { template <typename InSampleType> class TSlidingBuffer; }

namespace Atom
{
	/**
	 * Contains settings for loudness analyzer.
	 */
	class FLoudnessSettings : public IAnalyzerSettings, public FLoudnessAnalyzerSettings
	{
	public:
		/** Number of seconds between loudness measurements. (100ms) */
		float AnalysisPeriod = 0.1f;
	};

	/**
	 * Holds the loudness values per a time step.
	 */
	struct FLoudnessEntry
	{
		float Timestamp = 0.f;
		float Momentary = 0.f;
		float ShortTerm = 0.f;
		float Integrated = 0.f;
	};

	/** 
	 * FLoudnessResult contains the temporal evolution of loudness for the last buffer of audio analyzed.
	 */
	class FLoudnessResult : public IAnalyzerResult
	{
	public:

		FLoudnessResult() {}

		/** Appends an FLoudnessEntry to the container. */
		CRI_API void Add(const FLoudnessEntry& InDatum);

		/** Returns const reference to FLoudnessEntry array associated with overall loudness. */
		CRI_API const TArray<FLoudnessEntry>& GetLoudnessArray() const;

	private:
		TArray<FLoudnessEntry> LoudnessArray;
	};

	/**
	 * FLoudnessWorker performs loudness analysis on input sample buffers.
	 */
	class FLoudnessWorker : public IAnalyzerWorker
	{
	public:
		/** Construct a worker */
		CRI_API FLoudnessWorker(const FAnalyzerParameters& InParams, const FLoudnessSettings& InAnalyzerSettings);

		/** Analyzes input sample buffer and updates result. */
		CRI_API virtual void Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult) override;

	private:

		/** Analyze a single window. */
		void AnalyzeWindow(TArrayView<const float> InWindow, FLoudnessResult& OutResult);

		int32 NumChannels = 0;
		int32 NumAnalyzedBuffers = 0;
		int32 NumHopFrames = 0;
		int32 SampleRate = 0;
		TArray<float> InternalWindow;
		TUniquePtr<Audio::TSlidingBuffer<float>> InternalBuffer;
		TUniquePtr<FLoudnessAnalyzer> Analyzer;
	};

	/**
	 * Defines the Loudness analyzer and creates related classes.
	 */
	class FLoudnessFactory : public IAnalyzerFactory
	{
	public:

		/** Name of specific analyzer type. */
		CRI_API virtual FName GetName() const override;

		/** Human readable name of analyzer. */
		CRI_API virtual FString GetTitle() const override;

		/** Creates a new FLoudnessNRTResult */
		CRI_API virtual TUniquePtr<IAnalyzerResult> NewResult() const override;

		/**  Creates a new FLoudnessWorker. This expects IAnalyzerNRTSettings to be a valid pointer to a FLoudnessSettings object. */
		CRI_API virtual TUniquePtr<IAnalyzerWorker> NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const override;
	};
}

#undef CRI_API
