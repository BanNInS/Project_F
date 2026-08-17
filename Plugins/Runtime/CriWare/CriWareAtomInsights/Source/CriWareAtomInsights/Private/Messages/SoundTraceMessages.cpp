
#include "Messages/SoundTraceMessages.h"

namespace Atom::Insights
{
	namespace SoundClassNames
	{
		const FString MetaSoundSource = TEXT("MetaSoundSource");
		const FString SoundWaveProcedural = TEXT("AtomSoundWaveProcedural");
		const FString SoundCue = TEXT("AtomSoundCue");
		const FString SoundWave = TEXT("AtomSoundWave");
		//const FString Sound = TEXT("AtomSound...");
	}

	FSoundMessageBase::FSoundMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
	{
		const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

		RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
		Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
		PlayOrder = EventData.GetValue<uint32>("PlayOrder");
	}

	FSoundStartMessage::FSoundStartMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		: FSoundMessageBase(InContext)
	{
		const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;
		EventData.GetString("Name", Name);

		FString SoundClassName;
		FString SoundSuperClassName;

		EventData.GetString("SoundClassName", SoundClassName);
		EventData.GetString("SoundSuperClassName", SoundSuperClassName);

		if (SoundClassName == SoundClassNames::SoundCue)
		{
			EntryType = ESoundDashboardEntryType::SoundCue;
		}
		else if (SoundClassName == SoundClassNames::SoundWave)
		{
			EntryType = ESoundDashboardEntryType::SoundWave;
		}
		else if (SoundClassName == SoundClassNames::SoundWaveProcedural
			|| SoundSuperClassName == SoundClassNames::SoundWaveProcedural)
		{
			EntryType = ESoundDashboardEntryType::ProceduralSource;
		}
		else
		{
			EntryType = ESoundDashboardEntryType::None;
		}
	}

	FSoundDashboardEntry::FSoundDashboardEntry()
	{
		constexpr uint32 DataPointsCapacity = 256;

		PriorityDataPoints.SetCapacity(DataPointsCapacity);
		DistanceDataPoints.SetCapacity(DataPointsCapacity);
		AmplitudeDataPoints.SetCapacity(DataPointsCapacity);
		VolumeDataPoints.SetCapacity(DataPointsCapacity);
		PitchDataPoints.SetCapacity(DataPointsCapacity);
	}
}