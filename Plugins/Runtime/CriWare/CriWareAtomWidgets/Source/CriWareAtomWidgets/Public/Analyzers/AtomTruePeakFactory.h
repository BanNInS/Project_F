
#pragma once

#include "Templates/UniquePtr.h"
#include "DSP/SlidingWindow.h"

#include "AtomTruePeakAnalyzer.h"

#include "Extensions/IAtomAnalyzerInterface.h"

namespace Audio { template <typename InSampleType> class TSlidingBuffer; }

namespace Atom
{
	/**
	 * Contains settings for true peak analyzer.
	 */
	class CRIWAREATOMWIDGETS_API FTruePeakSettings : public IAnalyzerSettings, public FTruePeakAnalyzerSettings
	{	
	public:
		/** Number of seconds between loudness measurements */
		float AnalysisPeriod = 0.01f;
	};

	/**
	 * Holds the true peak results per a time step for each channel
	 */
	struct FTruePeakEntry
	{
		int32 Channel = 0;
		float Timestamp = 0.0f;
		float PeakValue = 0.0f;
		float PeakHoldValue = 0.0f;
		float ClippingValue = 0.0f;
		int32 NumSamplesClipping = 0;
	};

	/** 
	 * FTruePeakResult contains the temporal evolution of loudness for the last buffer of audio analyzed.
	 */
	class CRIWAREATOMWIDGETS_API FTruePeakResult : public IAnalyzerResult
	{
	public:
		/**
		 * Denotes the overall loudness as opposed individual channel indices.
		 */
		static const int32 ChannelIndexOverall;

		FTruePeakResult() {}

		/** Appends an FTruePeakEntry to the container. */
		void Add(const FTruePeakEntry& InEntry);

		/** Returns const reference to FTruePeakEntry array for individual channel. */
		const TArray<FTruePeakEntry>& GetChannelTruePeakArray(int32 ChannelIdx) const;

		/** Returns const reference to FTruePeakEntry array associated with overall loudness. */
		const TArray<FTruePeakEntry>& GetTruePeakArray() const;

		/** Returns the number of channels. */
		int32 GetNumChannels() const;

	private:
		float DurationInSeconds = 0.0f;
		TMap<int32, TArray<FTruePeakEntry> > ChannelTruePeakArrays;
	};

	/**
	 * FTruePeakWorker performs true peak analysis on input sample buffers.
	 */
	class CRIWAREATOMWIDGETS_API FTruePeakWorker : public IAnalyzerWorker
	{
	public:
		/** Construct a worker */
		FTruePeakWorker(const FAnalyzerParameters& InParams, const FTruePeakSettings& InAnalyzerSettings);

		/** Analyzes input sample buffer and updates result. */
		virtual void Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult) override;

	private:

		int32 NumChannels = 0;
		int32 NumAnalyzedBuffers = 0;
		int32 NumHopFrames = 0;
		int32 SampleRate = 0;
		TArray<float> InternalWindow;
		TUniquePtr<Audio::TSlidingBuffer<float>> InternalBuffer;
		TUniquePtr<FTruePeakAnalyzer> TruePeakAnalyzer;
	};

	/**
	 * Defines the true peak analyzer and creates related classes.
	 */
	class CRIWAREATOMWIDGETS_API FTruePeakFactory : public IAnalyzerFactory
	{
		public:

		/** Name of specific analyzer type. */
		virtual FName GetName() const override;

		/** Human readable name of analyzer. */
		virtual FString GetTitle() const override;

		/** Creates a new FLoudnessNRTResult */
		virtual TUniquePtr<IAnalyzerResult> NewResult() const override;

		/**  Creates a new FLoudnessWorker. This expects IAnalyzerNRTSettings to be a valid pointer to a FLoudnessSettings object. */
		virtual TUniquePtr<IAnalyzerWorker> NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const override;
	};
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 7
#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "DSP/SlidingWindow.h"
#endif
#endif