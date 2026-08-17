
#include "Providers/MixerSourceTraceProvider.h"

#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/ModuleService.h"

#include "CriWareAtomInsightsModule.h"

#if !WITH_EDITOR
#include "Common/PagedArray.h"
#include "Async/ParallelFor.h"
#endif // !WITH_EDITOR

namespace Atom::Insights
{
	namespace FMixerSourceTraceProviderPrivate
	{
#if !WITH_EDITOR
		template<typename T>
		const T* FindClosestMessageToTimestamp(const TraceServices::TPagedArray<T>& InCachedMessages, const double InTimeMarker, const uint32 InPlayOrder)
		{
			const int32 ClosestMessageToTimeStampIndex = TraceServices::PagedArrayAlgo::BinarySearchClosestBy(InCachedMessages, InTimeMarker, [](const T& Msg) { return Msg.Timestamp; });

			// Iterate backwards from TimeMarker until we find the matching PlayOrder
			for (auto It = InCachedMessages.GetIteratorFromItem(ClosestMessageToTimeStampIndex); It; --It)
			{
				if (It->PlayOrder == InPlayOrder)
				{
					return &(*It);
				}
			}

			return nullptr;
		}
#endif // !WITH_EDITOR
	}

	FName FMixerSourceTraceProvider::GetName_Static()
	{
		return "AtomMixerSourceProvider";
	}

#if !WITH_EDITOR
	void FMixerSourceTraceProvider::OnTimingViewTimeMarkerChanged(double TimeMarker)
	{
		using namespace FMixerSourceTraceProviderPrivate;

		if (!SessionCachedMessages.IsValid())
		{
			return;
		}

		DeviceDataMap.Empty();

		// Collect all the start messages registered until this point in time 
		for (const FMixerSourceStartMessage& StartCachedMessage : SessionCachedMessages->StartCachedMessages)
		{
			if (StartCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			UpdateDeviceEntry(StartCachedMessage.RuntimeID, StartCachedMessage.PlayOrder, [&StartCachedMessage](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
			{
				if (!Entry.IsValid())
				{
					Entry = MakeShared<FMixerSourceDashboardEntry>();
					Entry->RuntimeID  = StartCachedMessage.RuntimeID;
					Entry->PlayOrder = StartCachedMessage.PlayOrder;
				}
				Entry->Timestamp = StartCachedMessage.Timestamp;

				Entry->Name        = *StartCachedMessage.Name;
				Entry->ComponentId = StartCachedMessage.ComponentId;
				Entry->SourceId    = StartCachedMessage.SourceId;
			});
		}

		// Selectively remove start messages collected in the step above by knowing which sounds were stopped.
		// With this we will know what are the active sounds at this point in time.
		for (const FMixerSourceStopMessage& StopCachedMessage : SessionCachedMessages->StopCachedMessages)
		{
			if (StopCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			auto* OutEntry = FindDeviceEntry(StopCachedMessage.RuntimeID, StopCachedMessage.PlayOrder);

			if (OutEntry && (*OutEntry)->Timestamp < StopCachedMessage.Timestamp)
			{
				RemoveDeviceEntry(StopCachedMessage.RuntimeID, StopCachedMessage.PlayOrder);
			}
		}

		// For now we only retrieve information from AtomRuntimeID 1 (main device in standalone games)
		const FDeviceData* DeviceData = DeviceDataMap.Find(1);
		if (DeviceData)
		{
			// Collect messages (volume, pitch, etc.) from active sounds (based on active sounds's PlayOrder)
			struct CachedEntryInfo
			{
				FMixerSourceVolumeMessage VolumeMessage;
				FMixerSourcePitchMessage PitchMessage;
				FMixerSourceLPFFreqMessage LPFFreqMessage;
				FMixerSourceHPFFreqMessage HPFFreqMessage;
				FMixerSourceEnvelopeMessage EnvelopeMessage;
				FMixerSourceDistanceAttenuationMessage DistanceAttenuationMessage;
			};

			TArray<uint32> PlayOrderArray;
			(*DeviceData).GenerateKeyArray(PlayOrderArray);

			TArray<CachedEntryInfo> CachedEntryInfos;
			CachedEntryInfos.SetNumUninitialized(PlayOrderArray.Num());

			// Using ParallelFor to speed-up the cached messages retrieval, using a traditional for loop is unacceptably slower, specially in large traces.
			ParallelFor(PlayOrderArray.Num(), 
			[&PlayOrderArray, &CachedEntryInfos, TimeMarker, this](const int32 Index)
			{
				const uint32 PlayOrder = PlayOrderArray[Index];

				// Volume
				const FMixerSourceVolumeMessage* FoundVolumeCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->VolumeCachedMessages, TimeMarker, PlayOrder);
				if (FoundVolumeCachedMessage)
				{
					CachedEntryInfos[Index].VolumeMessage = *FoundVolumeCachedMessage;
				}

				// Pitch
				const FMixerSourcePitchMessage* FoundPitchCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->PitchCachedMessages, TimeMarker, PlayOrder);
				if (FoundPitchCachedMessage)
				{
					CachedEntryInfos[Index].PitchMessage = *FoundPitchCachedMessage;
				}

				// LPF
				const FMixerSourceLPFFreqMessage* FoundLPFFreqCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->LPFFreqCachedMessages, TimeMarker, PlayOrder);
				if (FoundLPFFreqCachedMessage)
				{
					CachedEntryInfos[Index].LPFFreqMessage = *FoundLPFFreqCachedMessage;
				}

				// HPF
				const FMixerSourceHPFFreqMessage* FoundHPFFreqCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->HPFFreqCachedMessages, TimeMarker, PlayOrder);
				if (FoundHPFFreqCachedMessage)
				{
					CachedEntryInfos[Index].HPFFreqMessage = *FoundHPFFreqCachedMessage;
				}

				// Envelope
				const FMixerSourceEnvelopeMessage* FoundEnvelopeCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->EnvelopeCachedMessages, TimeMarker, PlayOrder);
				if (FoundEnvelopeCachedMessage)
				{
					CachedEntryInfos[Index].EnvelopeMessage = *FoundEnvelopeCachedMessage;
				}

				// Distance Attenuation
				const FMixerSourceDistanceAttenuationMessage* FoundDistanceAttenuationCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->DistanceAttenuationCachedMessages, TimeMarker, PlayOrder);
				if (FoundDistanceAttenuationCachedMessage)
				{
					CachedEntryInfos[Index].DistanceAttenuationMessage = *FoundDistanceAttenuationCachedMessage;
				}
			});

			// Update the device entries with the collected info
			for (const CachedEntryInfo& CachedEntryInfo : CachedEntryInfos)
			{
				UpdateDeviceEntry(CachedEntryInfo.VolumeMessage.RuntimeID, CachedEntryInfo.VolumeMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.VolumeMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.VolumeMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.VolumeMessage.Timestamp;
					Entry->VolumeDataPoints.Push({ CachedEntryInfo.VolumeMessage.Timestamp, CachedEntryInfo.VolumeMessage.Volume });
				});

				UpdateDeviceEntry(CachedEntryInfo.PitchMessage.RuntimeID, CachedEntryInfo.PitchMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.PitchMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.PitchMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.PitchMessage.Timestamp;
					Entry->PitchDataPoints.Push({ CachedEntryInfo.PitchMessage.Timestamp, CachedEntryInfo.PitchMessage.Pitch });
				});

				UpdateDeviceEntry(CachedEntryInfo.LPFFreqMessage.RuntimeID, CachedEntryInfo.LPFFreqMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.LPFFreqMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.LPFFreqMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.LPFFreqMessage.Timestamp;
					Entry->LPFFreqDataPoints.Push({ CachedEntryInfo.LPFFreqMessage.Timestamp, CachedEntryInfo.LPFFreqMessage.LPFFrequency });
				});

				UpdateDeviceEntry(CachedEntryInfo.HPFFreqMessage.RuntimeID, CachedEntryInfo.HPFFreqMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.HPFFreqMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.HPFFreqMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.HPFFreqMessage.Timestamp;
					Entry->HPFFreqDataPoints.Push({ CachedEntryInfo.HPFFreqMessage.Timestamp, CachedEntryInfo.HPFFreqMessage.HPFFrequency });
				});

				UpdateDeviceEntry(CachedEntryInfo.EnvelopeMessage.RuntimeID, CachedEntryInfo.EnvelopeMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.EnvelopeMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.EnvelopeMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.EnvelopeMessage.Timestamp;
					Entry->EnvelopeDataPoints.Push({ CachedEntryInfo.EnvelopeMessage.Timestamp, CachedEntryInfo.EnvelopeMessage.Envelope });
				});

				UpdateDeviceEntry(CachedEntryInfo.DistanceAttenuationMessage.RuntimeID, CachedEntryInfo.DistanceAttenuationMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FMixerSourceDashboardEntry>();
						Entry->RuntimeID  = CachedEntryInfo.DistanceAttenuationMessage.RuntimeID;
						Entry->PlayOrder = CachedEntryInfo.DistanceAttenuationMessage.PlayOrder;
					}

					Entry->Timestamp = CachedEntryInfo.DistanceAttenuationMessage.Timestamp;
					Entry->DistanceAttenuationDataPoints.Push({ CachedEntryInfo.DistanceAttenuationMessage.Timestamp, CachedEntryInfo.DistanceAttenuationMessage.DistanceAttenuation });
				});
			}
		}

		// Call parent method to update LastMessageId
		FTraceProviderBase::OnTimingViewTimeMarkerChanged(TimeMarker);
	}
#endif // !WITH_EDITOR

	bool FMixerSourceTraceProvider::ProcessMessages()
	{
		auto BumpEntryFunc = [this](const FMixerSourceMessageBase& Msg)
		{
			TSharedPtr<FMixerSourceDashboardEntry>* ToReturn = nullptr;
			UpdateRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder, [&ToReturn, &Msg](TSharedPtr<FMixerSourceDashboardEntry>& Entry)
			{
				if (!Entry.IsValid())
				{
					Entry = MakeShared<FMixerSourceDashboardEntry>();
					Entry->RuntimeID = Msg.RuntimeID;
					Entry->PlayOrder = Msg.PlayOrder;
				}
				Entry->Timestamp = Msg.Timestamp;
				ToReturn = &Entry;
			});

			return ToReturn;
		};

		ProcessMessageQueue<FMixerSourceStartMessage>(TraceMessages.StartMessages, BumpEntryFunc,
		[](const FMixerSourceStartMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->StartCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FMixerSourceDashboardEntry& EntryRef = *OutEntry->Get();
			EntryRef.Name = *Msg.Name;
			EntryRef.ComponentID = Msg.ComponentID;
			EntryRef.SourceID = Msg.SourceID;
		});

		TArray<int32, TInlineAllocator<64>> EntriesWithPoppedDataPoints;

		ProcessMessageQueue<FMixerSourceVolumeMessage>(TraceMessages.VolumeMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceVolumeMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->VolumeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->VolumeDataPoints.Pop((*OutEntry)->VolumeDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->VolumeDataPoints.Push({ Msg.Timestamp, Msg.Volume });
		});

		EntriesWithPoppedDataPoints.Reset();
		ProcessMessageQueue<FMixerSourcePitchMessage>(TraceMessages.PitchMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourcePitchMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->PitchCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->PitchDataPoints.Pop((*OutEntry)->PitchDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->PitchDataPoints.Push({ Msg.Timestamp, Msg.Pitch });
		});

		EntriesWithPoppedDataPoints.Reset();
		ProcessMessageQueue<FMixerSourceLPFFreqMessage>(TraceMessages.LPFFreqMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceLPFFreqMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->LPFFreqCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->LPFFreqDataPoints.Pop((*OutEntry)->LPFFreqDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->LPFFreqDataPoints.Push({ Msg.Timestamp, Msg.LPFFrequency });
		});

		EntriesWithPoppedDataPoints.Reset();
		ProcessMessageQueue<FMixerSourceHPFFreqMessage>(TraceMessages.HPFFreqMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceHPFFreqMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->HPFFreqCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->HPFFreqDataPoints.Pop((*OutEntry)->HPFFreqDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->HPFFreqDataPoints.Push({ Msg.Timestamp, Msg.HPFFrequency });
		});

		EntriesWithPoppedDataPoints.Reset();
		ProcessMessageQueue<FMixerSourceEnvelopeMessage>(TraceMessages.EnvelopeMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceEnvelopeMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->EnvelopeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->EnvelopeDataPoints.Pop((*OutEntry)->EnvelopeDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->EnvelopeDataPoints.Push({ Msg.Timestamp, Msg.Envelope });
		});

		EntriesWithPoppedDataPoints.Reset();
		ProcessMessageQueue<FMixerSourceDistanceAttenuationMessage>(TraceMessages.DistanceAttenuationMessages, BumpEntryFunc,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceDistanceAttenuationMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->DistanceAttenuationCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (!EntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				(*OutEntry)->DistanceAttenuationDataPoints.Pop((*OutEntry)->DistanceAttenuationDataPoints.Num());
				EntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			(*OutEntry)->DistanceAttenuationDataPoints.Push({ Msg.Timestamp, Msg.DistanceAttenuation });
		});

		auto GetEntry = [this](const FMixerSourceMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder);
		};

		ProcessMessageQueue<FMixerSourceStopMessage>(TraceMessages.StopMessages, GetEntry,
		[this](const FMixerSourceStopMessage& Msg, TSharedPtr<FMixerSourceDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->StopCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (OutEntry && (*OutEntry)->Timestamp < Msg.Timestamp)
			{
				RemoveRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder);
			}
		});

		return true;
	}

#if !WITH_EDITOR
	void FMixerSourceTraceProvider::InitSessionCachedMessages(TraceServices::IAnalysisSession& InSession)
	{
		SessionCachedMessages = MakeUnique<FMixerSourceSessionCachedMessages>(InSession);
	}
#endif // !WITH_EDITOR

	UE::Trace::IAnalyzer* FMixerSourceTraceProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FMixerSourceTraceAnalyzer : public FTraceAnalyzerBase
		{
		public:
			FMixerSourceTraceAnalyzer(TSharedRef<FMixerSourceTraceProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override
			{
				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;
				Builder.RouteEvent(RouteId_DistanceAttenuation, "CriWareAtom", "MixerSourceDistanceAttenuation");
				Builder.RouteEvent(RouteId_Envelope, "CriWareAtom", "MixerSourceEnvelope");
				Builder.RouteEvent(RouteId_Filters, "CriWareAtom", "MixerSourceFilters");
				Builder.RouteEvent(RouteId_Pitch, "CriWareAtom", "MixerSourcePitch");
				Builder.RouteEvent(RouteId_Start, "CriWareAtom", "MixerSourceStart");
				Builder.RouteEvent(RouteId_Stop, "CriWareAtom", "MixerSourceStop");
				Builder.RouteEvent(RouteId_Volume, "CriWareAtom", "MixerSourceVolume");
			}

			virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("Insights/Atom::FMixerSourceTraceAnalyzer"));

				FMixerSourceMessages& Messages = GetProvider<FMixerSourceTraceProvider>().TraceMessages;
				switch (RouteId)
				{
					case RouteId_Start:
					{
						Messages.StartMessages.Enqueue(FMixerSourceStartMessage { Context });
						break;
					}

					case RouteId_Stop:
					{
						Messages.StopMessages.Enqueue(FMixerSourceStopMessage { Context });
						break;
					}

					case RouteId_Volume:
					{
						Messages.VolumeMessages.Enqueue(FMixerSourceVolumeMessage { Context });
						break;
					}

					case RouteId_Pitch:
					{
						Messages.PitchMessages.Enqueue(FMixerSourcePitchMessage { Context });
						break;
					}

					case RouteId_Envelope:
					{
						Messages.EnvelopeMessages.Enqueue(FMixerSourceEnvelopeMessage { Context });
						break;
					}

					case RouteId_Filters:
					{
						Messages.LPFFreqMessages.Enqueue(FMixerSourceLPFFreqMessage { Context });
						Messages.HPFFreqMessages.Enqueue(FMixerSourceHPFFreqMessage { Context });
						break;
					}

					case RouteId_DistanceAttenuation:
					{
						Messages.DistanceAttenuationMessages.Enqueue(FMixerSourceDistanceAttenuationMessage { Context });
						break;
					}

					default:
					{
						return OnEventFailure(RouteId, Style, Context);
					}
				}

				const double Timestamp = Context.EventTime.AsSeconds(Context.EventData.GetValue<uint64>("Timestamp"));

				{
					TraceServices::FAnalysisSessionEditScope SessionEditScope(Session);
					Session.UpdateDurationSeconds(Timestamp);
				}

				return OnEventSuccess(RouteId, Style, Context);
			}

		private:
			enum : uint16
			{
				RouteId_DistanceAttenuation,
				RouteId_Envelope,
				RouteId_Filters,
				RouteId_Pitch,
				RouteId_Start,
				RouteId_Stop,
				RouteId_Volume,
			};

			TraceServices::IAnalysisSession& Session;
		};

		return new FMixerSourceTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
