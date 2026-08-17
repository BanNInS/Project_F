
#pragma once

#include "Messages/AnalyzerMessageQueue.h"
#include "Trace/Analyzer.h"
#include "Views/TableDashboardViewFactory.h"

#include "CriWareDefines.h"

class UAtomAudioBus;

namespace Atom::Insights
{
	enum class EAudioBusEntryType : uint8
	{
		AssetBased,
		CodeGenerated,
		None
	};

	struct FAudioBusMessageBase
	{
		FAudioBusMessageBase() = default;

		FAudioBusMessageBase(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			RuntimeID = static_cast<FAtomRuntimeId>(EventData.GetValue<uint32>("RuntimeID"));
			AudioBusID = EventData.GetValue<uint32>("AudioBusID");
			Timestamp = InContext.EventTime.AsSeconds(EventData.GetValue<uint64>("Timestamp"));
		}

		FAtomRuntimeId RuntimeID = INDEX_NONE;
		uint32 AudioBusID = INDEX_NONE;
		double Timestamp = 0.0;
	};

	struct FAudioBusStartMessage : public FAudioBusMessageBase
	{
		FAudioBusStartMessage() = default;

		FAudioBusStartMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FAudioBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			EventData.GetString("Name", Name);
			NumChannels = EventData.GetValue<int32>("NumChannels");

			AudioBusType = FSoftObjectPath(Name).IsAsset() ? EAudioBusEntryType::AssetBased : EAudioBusEntryType::CodeGenerated;
		}

		FString Name;
		int32 NumChannels = 0;
		EAudioBusEntryType AudioBusType = EAudioBusEntryType::None;
	};

	struct FAudioBusHasActivityMessage : public FAudioBusMessageBase
	{
		FAudioBusHasActivityMessage() = default;

		FAudioBusHasActivityMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FAudioBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			bHasActivity = EventData.GetValue<bool>("HasActivity");
		}

		bool bHasActivity = false;
	};

	struct FAudioBusEnvelopeFollowerEnabledMessage : public FAudioBusMessageBase
	{
		FAudioBusEnvelopeFollowerEnabledMessage() = default;

		FAudioBusEnvelopeFollowerEnabledMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FAudioBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			bEnvelopeFollowerEnabled = EventData.GetValue<bool>("EnvelopeFollowerEnabled");
		}

		bool bEnvelopeFollowerEnabled = false;
	};

	struct FAudioBusEnvelopeValuesMessage : public FAudioBusMessageBase
	{
		FAudioBusEnvelopeValuesMessage() = default;

		FAudioBusEnvelopeValuesMessage(const UE::Trace::IAnalyzer::FOnEventContext& InContext)
			: FAudioBusMessageBase(InContext)
		{
			const UE::Trace::IAnalyzer::FEventData& EventData = InContext.EventData;

			const TArrayView<const float> EnvelopeValuesView = EventData.GetArrayView<float>("EnvelopeValues");

			for (const auto& EnvelopeValue : EnvelopeValuesView)
			{
				EnvelopeValues.Emplace(EnvelopeValue);
			}
		}

		TArray<float> EnvelopeValues;
	};

	using FAudioBusStopMessage = FAudioBusMessageBase;

	class FAudioBusMessages
	{
		TAnalyzerMessageQueue<FAudioBusStartMessage> StartMessages{ 0.1 };
		TAnalyzerMessageQueue<FAudioBusHasActivityMessage> HasActivityMessages{ 0.1 };
		TAnalyzerMessageQueue<FAudioBusEnvelopeFollowerEnabledMessage> EnvelopeFollowerEnabledMessages;
		TAnalyzerMessageQueue<FAudioBusEnvelopeValuesMessage> EnvelopeValuesMessages;
		TAnalyzerMessageQueue<FAudioBusStopMessage> StopMessages{ 0.1 };

		friend class FAudioBusProvider;
	};

	struct FAudioBusAssetDashboardEntry : public FSoundAssetDashboardEntry
	{
		FAudioBusAssetDashboardEntry() = default;
		virtual ~FAudioBusAssetDashboardEntry() = default;

		EAudioBusEntryType EntryType = EAudioBusEntryType::None;

		TWeakObjectPtr<UAtomAudioBus> AudioBus;
		uint32 AudioBusID = INDEX_NONE;
		int32 NumChannels = 0;
		bool bHasActivity = false;
		bool bEnvelopeFollowerEnabled = false;
	};
} // namespace
