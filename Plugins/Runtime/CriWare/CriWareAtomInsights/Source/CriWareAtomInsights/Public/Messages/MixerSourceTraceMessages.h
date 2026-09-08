
#pragma once

#include "DSP/Dsp.h"
#include "Trace/Analyzer.h"

#include "CriWareDefines.h"
#include "Messages/AnalyzerMessageQueue.h"
#include "Views/TableDashboardViewFactory.h"

#if !WITH_EDITOR
#include "Common/PagedArray.h"
#include "TraceServices/Model/AnalysisSession.h"
#endif // !WITH_EDITOR

namespace Atom::Insights
{
	struct FMixerSourceMessageBase
	{
		FMixerSourceMessageBase() = default;
		FMixerSourceMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
			RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
			Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
			PlayOrder = EventData.GetValue<uint32>("PlayOrder");
		}

		FAtomRuntimeId RuntimeID = INDEX_NONE;
		uint32 PlayOrder = INDEX_NONE;
		uint64 ComponentID = TNumericLimits<uint64>::Max();
		double Timestamp = 0.0;
	};

	using FMixerSourceStopMessage = FMixerSourceMessageBase;

	struct FMixerSourceStartMessage : public FMixerSourceMessageBase
	{
		FMixerSourceStartMessage() = default;
		FMixerSourceStartMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FMixerSourceMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
			EventData.GetString("Name", Name);
			ComponentID = EventData.GetValue<uint64>("ComponentID");
			SourceID = EventData.GetValue<int32>("SourceID");
		}

		FString Name;
		int32 SourceID = INDEX_NONE;
		uint64 ComponentID = TNumericLimits<uint64>::Max();
	};

	struct FMixerSourceParameterMessage : public FMixerSourceMessageBase
	{
		FMixerSourceParameterMessage() = default;

		FMixerSourceParameterMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FMixerSourceMessageBase(InContext)
		{
			ActiveSoundPlayOrder = InContext.EventData.GetValue<uint32>("ActiveSoundPlayOrder");
		}

		uint32 ActiveSoundPlayOrder;
	};

#define DEFINE_MIXERSOURCE_PARAM_MESSAGE(ClassName, SuperClassName, ParamName, Type, Default)	\
	struct ClassName : SuperClassName															\
	{																							\
		ClassName() = default;																	\
		ClassName(const UE::Trace::IAnalyzer::FOnEventContext& InContext)						\
			: SuperClassName(InContext)															\
		{																						\
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;			\
			ParamName = EventData.GetValue<Type>(#ParamName);									\
		}																						\
		Type ParamName = Default;																\
	};

	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourceDistanceAttenuationMessage, FMixerSourceMessageBase, DistanceAttenuation, float, 0.0f)
	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourceHPFFreqMessage, FMixerSourceMessageBase, HPFFrequency, float, ATOM_MIN_FILTER_FREQUENCY)
	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourceLPFFreqMessage, FMixerSourceMessageBase, LPFFrequency, float, ATOM_MAX_FILTER_FREQUENCY)
	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourceEnvelopeMessage, FMixerSourceParameterMessage, Envelope, float, 0.0f)
	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourcePitchMessage, FMixerSourceParameterMessage, Pitch, float, 1.0f)
	DEFINE_MIXERSOURCE_PARAM_MESSAGE(FMixerSourceVolumeMessage, FMixerSourceParameterMessage, Volume, float, 1.0f)
#undef DEFINE_MIXERSOURCE_PARAM_MESSAGE

	using FDataPoint = TPair<double, float>; // (Timestamp, Value)

	class FMixerSourceDashboardEntry : public FSoundAssetDashboardEntry
	{
	public:
		FMixerSourceDashboardEntry()
		{
			constexpr uint32 DataPointsCapacity = 256;

			VolumeDataPoints.SetCapacity(DataPointsCapacity);
			PitchDataPoints.SetCapacity(DataPointsCapacity);
			LPFFreqDataPoints.SetCapacity(DataPointsCapacity);
			HPFFreqDataPoints.SetCapacity(DataPointsCapacity);
			EnvelopeDataPoints.SetCapacity(DataPointsCapacity);
			DistanceAttenuationDataPoints.SetCapacity(DataPointsCapacity);
		}

		virtual ~FMixerSourceDashboardEntry() = default;

		int32 SourceID = INDEX_NONE;

		::Audio::TCircularAudioBuffer<FDataPoint> VolumeDataPoints;
		::Audio::TCircularAudioBuffer<FDataPoint> PitchDataPoints;
		::Audio::TCircularAudioBuffer<FDataPoint> LPFFreqDataPoints;
		::Audio::TCircularAudioBuffer<FDataPoint> HPFFreqDataPoints;
		::Audio::TCircularAudioBuffer<FDataPoint> EnvelopeDataPoints;
		::Audio::TCircularAudioBuffer<FDataPoint> DistanceAttenuationDataPoints;
	};

	class FMixerSourceMessages
	{
	public:
		TAnalyzerMessageQueue<FMixerSourceDistanceAttenuationMessage> DistanceAttenuationMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceEnvelopeMessage> EnvelopeMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceHPFFreqMessage> HPFFreqMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceLPFFreqMessage> LPFFreqMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourcePitchMessage> PitchMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceStartMessage> StartMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceStopMessage> StopMessages { 0.1 };
		TAnalyzerMessageQueue<FMixerSourceVolumeMessage> VolumeMessages { 0.1 };
	};

#if !WITH_EDITOR
	struct FMixerSourceSessionCachedMessages
	{
		FMixerSourceSessionCachedMessages(TraceServices::IAnalysisSession& InSession)
			: DistanceAttenuationCachedMessages(InSession.GetLinearAllocator(), 16384)
			, EnvelopeCachedMessages(InSession.GetLinearAllocator(), 16384)
			, HPFFreqCachedMessages(InSession.GetLinearAllocator(), 16384)
			, LPFFreqCachedMessages(InSession.GetLinearAllocator(), 16384)
			, PitchCachedMessages(InSession.GetLinearAllocator(), 16384)
			, StartCachedMessages(InSession.GetLinearAllocator(), 16384)
			, StopCachedMessages(InSession.GetLinearAllocator(), 4096)
			, VolumeCachedMessages(InSession.GetLinearAllocator(), 16384)
		{

		}

		TraceServices::TPagedArray<FMixerSourceDistanceAttenuationMessage> DistanceAttenuationCachedMessages;
		TraceServices::TPagedArray<FMixerSourceEnvelopeMessage> EnvelopeCachedMessages;
		TraceServices::TPagedArray<FMixerSourceHPFFreqMessage> HPFFreqCachedMessages;
		TraceServices::TPagedArray<FMixerSourceLPFFreqMessage> LPFFreqCachedMessages;
		TraceServices::TPagedArray<FMixerSourcePitchMessage> PitchCachedMessages;
		TraceServices::TPagedArray<FMixerSourceStartMessage> StartCachedMessages;
		TraceServices::TPagedArray<FMixerSourceStopMessage> StopCachedMessages;
		TraceServices::TPagedArray<FMixerSourceVolumeMessage> VolumeCachedMessages;

		TMap<uint32, FMixerSourceStartMessage> PlayOrderToStartMessageMap;
	};
#endif // !WITH_EDITOR
} // namespace
