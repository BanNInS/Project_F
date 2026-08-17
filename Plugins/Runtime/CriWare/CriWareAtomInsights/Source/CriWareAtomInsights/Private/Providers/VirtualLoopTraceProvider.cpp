
#include "Providers/VirtualLoopTraceProvider.h"

#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/ModuleService.h"

#if !WITH_EDITOR
#include "Common/PagedArray.h"
#include "Async/ParallelFor.h"
#endif // !WITH_EDITOR

namespace Atom::Insights
{
	namespace FVirtualLoopTraceProviderPrivate
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

	FName FVirtualLoopTraceProvider::GetName_Static()
	{
		return "AtomVirtualLoopProvider";
	}

#if !WITH_EDITOR
	void FVirtualLoopTraceProvider::OnTimingViewTimeMarkerChanged(double TimeMarker)
	{
		using namespace FVirtualLoopTraceProviderPrivate;

		if (!SessionCachedMessages.IsValid())
		{
			return;
		}

		RuntimeDataMap.Empty();

		// Collect all the virtualize messages registered until this point in time 
		for (const FVirtualLoopVirtualizeMessage& VirtualizeCachedMessage : SessionCachedMessages->VirtualizeCachedMessages)
		{
			if (VirtualizeCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			UpdateRuntimeEntry(VirtualizeCachedMessage.RuntimeID, VirtualizeCachedMessage.PlayOrder, [&VirtualizeCachedMessage](TSharedPtr<FVirtualLoopDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FVirtualLoopDashboardEntry>();
						Entry->RuntimeID = VirtualizeCachedMessage.RuntimeID;
						Entry->PlayOrder = VirtualizeCachedMessage.PlayOrder;
					}
					Entry->Timestamp = VirtualizeCachedMessage.Timestamp;

					Entry->Name = *VirtualizeCachedMessage.Name;
					Entry->ComponentID = VirtualizeCachedMessage.ComponentID;
				});
		}

		// Selectively remove virtualize messages collected in the step above by knowing which sounds were stopped/realized.
		// With this we will know what are the virtualized sounds at this point in time.
		for (const FVirtualLoopMessageBase& StopOrRealizeCachedMessage : SessionCachedMessages->StopOrRealizeCachedMessages)
		{
			if (StopOrRealizeCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			auto* OutEntry = FindRuntimeEntry(StopOrRealizeCachedMessage.RuntimeID, StopOrRealizeCachedMessage.PlayOrder);

			if (OutEntry && (*OutEntry)->Timestamp < StopOrRealizeCachedMessage.Timestamp)
			{
				RemoveRuntimeEntry(StopOrRealizeCachedMessage.RuntimeID, StopOrRealizeCachedMessage.PlayOrder);
			}
		}

		// For now we only retrieve information from AtomRuntimeID 1 (main runtime in standalone games)
		const FRuntimeData* RuntimeData = RuntimeDataMap.Find(1);
		if (RuntimeData)
		{
			// Collect update messages from virtualized sounds (based on active sounds's PlayOrder)
			struct CachedEntryInfo
			{
				FVirtualLoopUpdateMessage UpdateMessage;
			};

			TArray<uint32> PlayOrderArray;
			(*RuntimeData).GenerateKeyArray(PlayOrderArray);

			TArray<CachedEntryInfo> CachedEntryInfos;
			CachedEntryInfos.SetNumUninitialized(PlayOrderArray.Num());

			ParallelFor(PlayOrderArray.Num(),
				[&PlayOrderArray, &CachedEntryInfos, TimeMarker, this](const int32 Index)
				{
					const uint32 PlayOrder = PlayOrderArray[Index];

					// Update
					const FVirtualLoopUpdateMessage* FoundUpdateCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->UpdateCachedMessages, TimeMarker, PlayOrder);
					if (FoundUpdateCachedMessage)
					{
						CachedEntryInfos[Index].UpdateMessage = *FoundUpdateCachedMessage;
					}
				});

			// Update the runtime entries with the collected info
			for (const CachedEntryInfo& CachedEntryInfo : CachedEntryInfos)
			{
				UpdateRuntimeEntry(CachedEntryInfo.UpdateMessage.RuntimeID, CachedEntryInfo.UpdateMessage.PlayOrder, [&CachedEntryInfo](TSharedPtr<FVirtualLoopDashboardEntry>& Entry)
					{
						if (!Entry.IsValid())
						{
							Entry = MakeShared<FVirtualLoopDashboardEntry>();
							Entry->RuntimeID = CachedEntryInfo.UpdateMessage.RuntimeID;
							Entry->PlayOrder = CachedEntryInfo.UpdateMessage.PlayOrder;
						}

						Entry->Timestamp = CachedEntryInfo.UpdateMessage.Timestamp;

						Entry->PlaybackTime = CachedEntryInfo.UpdateMessage.PlaybackTime;
						Entry->TimeVirtualized = CachedEntryInfo.UpdateMessage.TimeVirtualized;
						Entry->UpdateInterval = CachedEntryInfo.UpdateMessage.UpdateInterval;
						Entry->Location = FVector{ CachedEntryInfo.UpdateMessage.LocationX, CachedEntryInfo.UpdateMessage.LocationY, CachedEntryInfo.UpdateMessage.LocationZ };
						Entry->Rotator = FRotator{ CachedEntryInfo.UpdateMessage.RotatorPitch, CachedEntryInfo.UpdateMessage.RotatorYaw, CachedEntryInfo.UpdateMessage.RotatorRoll };
					});
			}
		}

		// Call parent method to update LastMessageId
		FTraceProviderBase::OnTimingViewTimeMarkerChanged(TimeMarker);
	}
#endif // !WITH_EDITOR

	bool FVirtualLoopTraceProvider::ProcessMessages()
	{
		auto RemoveEntryFunc = [this](const FVirtualLoopRealizeMessage& Msg, TSharedPtr<FVirtualLoopDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->StopOrRealizeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (OutEntry && (*OutEntry)->Timestamp < Msg.Timestamp)
			{
				RemoveRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder);
			}
		};

		auto GetEntryFunc = [this](const FVirtualLoopMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder);
		};

		auto BumpEntryFunc = [this](const FVirtualLoopMessageBase& Msg)
		{
			TSharedPtr<FVirtualLoopDashboardEntry>* ToReturn = nullptr;
			UpdateRuntimeEntry(Msg.RuntimeID, Msg.PlayOrder, [&ToReturn, &Msg](TSharedPtr<FVirtualLoopDashboardEntry>& Entry)
				{
					if (!Entry.IsValid())
					{
						Entry = MakeShared<FVirtualLoopDashboardEntry>();
						Entry->RuntimeID = Msg.RuntimeID;
						Entry->PlayOrder = Msg.PlayOrder;
					}
					Entry->Timestamp = Msg.Timestamp;

					ToReturn = &Entry;
				});

			return ToReturn;
		};

		ProcessMessageQueue<FVirtualLoopVirtualizeMessage>(TraceMessages.VirtualizeMessages,
		BumpEntryFunc,
		[this](const FVirtualLoopVirtualizeMessage& Msg, TSharedPtr<FVirtualLoopDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->VirtualizeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FVirtualLoopDashboardEntry& EntryRef = *OutEntry->Get();
			EntryRef.Name = Msg.Name;
			EntryRef.ComponentID = Msg.ComponentID;
		});

		ProcessMessageQueue<FVirtualLoopUpdateMessage>(TraceMessages.UpdateMessages,
		GetEntryFunc,
		[this](const FVirtualLoopUpdateMessage& Msg, TSharedPtr<FVirtualLoopDashboardEntry>* OutEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->UpdateCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (OutEntry)
			{
				FVirtualLoopDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.PlaybackTime = Msg.PlaybackTime;
				EntryRef.TimeVirtualized = Msg.TimeVirtualized;
				EntryRef.UpdateInterval = Msg.UpdateInterval;
				EntryRef.Location = FVector{ Msg.LocationX, Msg.LocationY, Msg.LocationZ };
				EntryRef.Rotator = FRotator{ Msg.RotatorPitch, Msg.RotatorYaw, Msg.RotatorRoll };
			}
		});

		ProcessMessageQueue<FVirtualLoopRealizeMessage>(TraceMessages.RealizeMessages, GetEntryFunc, RemoveEntryFunc);

		return true;
	}

#if !WITH_EDITOR
	void FVirtualLoopTraceProvider::InitSessionCachedMessages(TraceServices::IAnalysisSession& InSession)
	{
		SessionCachedMessages = MakeUnique<FVirtualLoopSessionCachedMessages>(InSession);
	}
#endif // !WITH_EDITOR

	UE::Trace::IAnalyzer* FVirtualLoopTraceProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FVirtualLoopTraceAnalyzer : public FTraceAnalyzerBase
		{
		public:
			FVirtualLoopTraceAnalyzer(TSharedRef<FVirtualLoopTraceProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override
			{
				FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;
				Builder.RouteEvent(RouteId_Stop, "CriWareAtom", "VirtualLoopStopOrRealize");
				Builder.RouteEvent(RouteId_Update, "CriWareAtom", "VirtualLoopUpdate");
				Builder.RouteEvent(RouteId_Virtualize, "CriWareAtom", "VirtualLoopVirtualize");
			}

			virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("Insights/Atom::FVirtualLoopTraceAnalyzer"));

				FVirtualLoopMessages& Messages = GetProvider<FVirtualLoopTraceProvider>().TraceMessages;
				switch (RouteId)
				{
					case RouteId_Stop:
					{
						Messages.StopMessages.Enqueue(FVirtualLoopStopMessage { Context });
						break;
					}

					case RouteId_Update:
					{
						Messages.UpdateMessages.Enqueue(FVirtualLoopUpdateMessage { Context });
						break;
					}

					case RouteId_Virtualize:
					{
						Messages.VirtualizeMessages.Enqueue(FVirtualLoopVirtualizeMessage { Context });
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
				RouteId_Virtualize,
				RouteId_Update,
				RouteId_Stop
			};

			TraceServices::IAnalysisSession& Session;
		};

		return new FVirtualLoopTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
