
#include "Providers/SoundTraceProvider.h"

#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"

#include "CriWareAtomInsightsModule.h"
#include "Messages/MixerSourceTraceMessages.h"

#if !WITH_EDITOR
#include "Common/PagedArray.h"
#include "Async/ParallelFor.h"
#endif // !WITH_EDITOR

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	namespace FSoundTraceProviderPrivate
	{
		const FString SoundCueDisplayName			{ LOCTEXT("AudioDashboard_Sounds_EntryType_SoundCues", "Sound Cues").ToString() };
		const FString SoundWaveDisplayName			{ LOCTEXT("AudioDashboard_Sounds_EntryType_SoundWaves", "Sound Waves").ToString() };
		const FString ProceduralSourceDisplayName	{ LOCTEXT("AudioDashboard_Sounds_EntryType_ProceduralSources", "Procedural Sources").ToString() };
		const FString UncategorizedSoundName		{ LOCTEXT("AudioDashboard_Sounds_EntryType_UncategorizedSound", "Others").ToString() };

		FSoundDashboardEntry& CastEntry(IDashboardDataTreeViewEntry& InData)
		{
			return static_cast<FSoundDashboardEntry&>(InData);
		};

		FString GetEntryTypeDisplayName(const ESoundDashboardEntryType EntryType)
		{
			switch (EntryType)
			{
				case ESoundDashboardEntryType::SoundCue:
					return SoundCueDisplayName;
				case ESoundDashboardEntryType::SoundWave:
					return SoundWaveDisplayName;
				case ESoundDashboardEntryType::ProceduralSource:
					return ProceduralSourceDisplayName;
			}

			return UncategorizedSoundName;
		}

		bool EntryTypeHasSoundPlaybackEntries(const ESoundDashboardEntryType EntryType)
		{
			switch (EntryType)
			{
				case ESoundDashboardEntryType::SoundCue:
					return true;

				case ESoundDashboardEntryType::ProceduralSource:
				case ESoundDashboardEntryType::SoundWave:
					return false;
				}

			return true;
		}

		void UpdateParameterEntry(const FMixerSourceParameterMessage& Msg, const float DataPoint, ::Audio::TCircularAudioBuffer<FDataPoint>& OutDataPoints, TArray<int32, TInlineAllocator<64>>& OutEntriesWithPoppedDataPoints)
		{
			if (!OutEntriesWithPoppedDataPoints.Contains(Msg.PlayOrder))
			{
				OutDataPoints.Pop(OutDataPoints.Num());
				OutEntriesWithPoppedDataPoints.Add(Msg.PlayOrder);
			}

			OutDataPoints.Push({ Msg.Timestamp, DataPoint });
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Finds an Active Sound entry inside a Category entry
		void FindActiveSoundEntryInCategory(const TSharedPtr<FSoundDashboardEntry>* CategoryEntry, const uint32 ActiveSoundPlayOrder, TSharedPtr<FSoundDashboardEntry>& OutActiveSoundEntry)
		{
			OutActiveSoundEntry = nullptr;
			if (CategoryEntry == nullptr || !CategoryEntry->IsValid())
			{
				return;
			}

			TSharedPtr<IDashboardDataTreeViewEntry>* ActiveSoundEntry = CategoryEntry->Get()->Children.FindByPredicate([ActiveSoundPlayOrder](TSharedPtr<IDashboardDataTreeViewEntry> ChildEntry)
			{
				FSoundDashboardEntry& NewEntry = CastEntry(*ChildEntry.Get());
				return NewEntry.PlayOrder == ActiveSoundPlayOrder;
			});

			if (ActiveSoundEntry == nullptr || !ActiveSoundEntry->IsValid())
			{
				return;
			}

			OutActiveSoundEntry = StaticCastSharedPtr<FSoundDashboardEntry>(*ActiveSoundEntry);
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Will return either the Active Sound entry or the child SoundPlayback entry depending on the sound's EntryType
		FSoundDashboardEntry* FindSoundEntryForActiveSound(const uint32 SoundPlaybackPlayOrder, const TSharedPtr<FSoundDashboardEntry>* ActiveSoundEntry)
		{
			if (ActiveSoundEntry == nullptr || !ActiveSoundEntry->IsValid())
			{
				return nullptr;
			}

			if (!EntryTypeHasSoundPlaybackEntries((*ActiveSoundEntry)->EntryType))
			{
				return ActiveSoundEntry->Get();
			}

			for (TSharedPtr<IDashboardDataTreeViewEntry> SoundPlaybackEntry : (*ActiveSoundEntry)->Children)
			{
				if (SoundPlaybackEntry == nullptr || !SoundPlaybackEntry.IsValid())
				{
					continue;
				}

				FSoundDashboardEntry* SoundPlaybackEntryCast = static_cast<FSoundDashboardEntry*>(SoundPlaybackEntry.Get());
				if (SoundPlaybackEntryCast->PlayOrder == SoundPlaybackPlayOrder)
				{
					return SoundPlaybackEntryCast;
				}
			}

			return nullptr;
		}

#if !WITH_EDITOR
		template<typename T>
		const T* FindClosestMessageToTimestamp(const TraceServices::TPagedArray<T>& InCachedMessages, const double InTimeMarker, const uint32 InPlayOrder)
		{
			const int32 ClosestMessageToTimeStampIndex = TraceServices::PagedArrayAlgo::BinarySearchClosestBy(InCachedMessages, InTimeMarker, [](const T& Msg) { return Msg.Timestamp; });

			// Iterate backwards from TimeMarker until we find the matching PlayOrder
			for (auto It = InCachedMessages.GetIteratorFromItem(ClosestMessageToTimeStampIndex); It; --It)
			{
				if (It->ActiveSoundPlayOrder == InPlayOrder)
				{
					return &(*It);
				}
			}

			return nullptr;
		}
#endif // !WITH_EDITOR
	}

	FSoundTraceProvider::FSoundTraceProvider()
		: TRuntimeDataMapTraceProvider<ESoundDashboardEntryType, TSharedPtr<FSoundDashboardEntry>>(GetName_Static())
	{
	}

	FName FSoundTraceProvider::GetName_Static()
	{
		static const FLazyName SoundTraceProviderName = "SoundProvider";
		return SoundTraceProviderName;
	}

	UE::Trace::IAnalyzer* FSoundTraceProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FSoundTraceAnalyzer : public FTraceAnalyzerBase
		{
		public:
			FSoundTraceAnalyzer(TSharedRef<FSoundTraceProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override
			{
				FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;

				Builder.RouteEvent(RouteId_ActiveSoundStart,	"CriWareAtom", "SoundStart");
				Builder.RouteEvent(RouteId_SoundPlaybackStart,	"CriWareAtom", "SoundPlaybackStart");
				Builder.RouteEvent(RouteId_Stop,				"CriWareAtom", "SoundStop");
				Builder.RouteEvent(RouteId_Priority,			"CriWareAtom", "SoundPriority");
				Builder.RouteEvent(RouteId_Distance,			"CriWareAtom", "SoundDistance");
				Builder.RouteEvent(RouteId_Amplitude,			"CriWareAtom", "MixerSourceEnvelope");
				Builder.RouteEvent(RouteId_Volume,				"CriWareAtom", "MixerSourceVolume");
				Builder.RouteEvent(RouteId_Pitch,				"CriWareAtom", "MixerSourcePitch");
			}

			virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("Insights/Atom::FSoundTraceAnalyzer"));

				FSoundMessages& Messages = GetProvider<FSoundTraceProvider>().TraceMessages;

				switch (RouteId)
				{
					case RouteId_ActiveSoundStart:
					{
						Messages.ActiveSoundStartMessages.Enqueue(FSoundStartMessage{ Context });
						break;
					}

					case RouteId_SoundPlaybackStart:
					{
						Messages.SoundPlaybackStartMessages.Enqueue(FSoundPlaybackStartMessage(Context));
						break;
					}

					case RouteId_Stop:
					{
						Messages.StopMessages.Enqueue(FSoundStopMessage{ Context });
						break;
					}

					case RouteId_Priority:
					{
						Messages.PriorityMessages.Enqueue(FSoundPriorityMessage{ Context });
						break;
					}

					case RouteId_Distance:
					{
						Messages.DistanceMessages.Enqueue(FSoundDistanceMessage{ Context });
						break;
					}

					case RouteId_Amplitude:
					{
						Messages.AmplitudeMessages.Enqueue(FMixerSourceEnvelopeMessage{ Context });
						break;
					}

					case RouteId_Volume:
					{
						Messages.VolumeMessages.Enqueue(FMixerSourceVolumeMessage{ Context });
						break;
					}

					case RouteId_Pitch:
					{
						Messages.PitchMessages.Enqueue(FMixerSourcePitchMessage{ Context });
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
				RouteId_ActiveSoundStart,
				RouteId_SoundPlaybackStart,
				RouteId_Stop,
				RouteId_Priority,
				RouteId_Distance,
				RouteId_Amplitude,
				RouteId_Volume,
				RouteId_Pitch
			};

			TraceServices::IAnalysisSession& Session;
		};

		ActiveSoundToEntryKeysMap.Empty();
		EntriesTimingOut.Empty();
		SoundsStoppedBeforeStart.Empty();

		return new FSoundTraceAnalyzer(AsShared(), InSession);
	}

#if !WITH_EDITOR
	void FSoundTraceProvider::InitSessionCachedMessages(TraceServices::IAnalysisSession& InSession)
	{
		SessionCachedMessages = MakeUnique<FSoundSessionCachedMessages>(InSession);
	}
#endif // !WITH_EDITOR

	bool FSoundTraceProvider::ProcessMessages()
	{
		// Helper lambdas
		TSharedPtr<FSoundDashboardEntry> ActiveSoundEntryReturn = nullptr;
		auto GetActiveSoundEntryFromActiveStartMessage = [this, &ActiveSoundEntryReturn](const FSoundStartMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			ActiveSoundEntryReturn = nullptr;

			GetOrCreateActiveSoundEntry(Msg, ActiveSoundEntryReturn);
			return &ActiveSoundEntryReturn;
		};

		TSharedPtr<FSoundDashboardEntry> SoundPlaybackActiveEntryReturn = nullptr;
		auto GetActiveSoundEntryFromSoundPlaybackStartMessage = [this, &SoundPlaybackActiveEntryReturn](const FSoundPlaybackStartMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			SoundPlaybackActiveEntryReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.ActiveSoundPlayOrder, Msg.RuntimeID, SoundPlaybackActiveEntryReturn);
			return &SoundPlaybackActiveEntryReturn;
		};

		TSharedPtr<FSoundDashboardEntry> ParamReturn = nullptr;
		auto GetActiveSoundEntryFromParameterMessage = [this, &ParamReturn](const FMixerSourceParameterMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			ParamReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.ActiveSoundPlayOrder, Msg.RuntimeID, ParamReturn);
			return &ParamReturn;
		};

		TSharedPtr<FSoundDashboardEntry> SoundEntryReturn = nullptr;
		auto GetSoundEntryFromStopMessage = [this, &SoundEntryReturn](const FSoundStopMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			SoundEntryReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.PlayOrder, Msg.RuntimeID, SoundEntryReturn);
			return &SoundEntryReturn;
		};

		// Process messages
		ProcessMessageQueue<FSoundStartMessage>(TraceMessages.ActiveSoundStartMessages, GetActiveSoundEntryFromActiveStartMessage,
		[this](const FSoundStartMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			if (OutActiveSoundEntry == nullptr || !OutActiveSoundEntry->IsValid())
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->StartCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundDashboardEntry& ActiveSoundEntryRef = *OutActiveSoundEntry->Get();
			ActiveSoundEntryRef.Name = *Msg.Name;
			ActiveSoundEntryRef.EntryType = Msg.EntryType;
			ActiveSoundEntryRef.bShouldForceExpandChildren = false;
		});

		ProcessMessageQueue<FSoundPlaybackStartMessage>(TraceMessages.SoundPlaybackStartMessages, GetActiveSoundEntryFromSoundPlaybackStartMessage,
		[this](const FSoundPlaybackStartMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			if (OutActiveSoundEntry == nullptr || !OutActiveSoundEntry->IsValid())
			{
				return;
			}

			if (!FSoundTraceProviderPrivate::EntryTypeHasSoundPlaybackEntries((*OutActiveSoundEntry)->EntryType))
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->SoundPlaybackStartCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR
			
			TSharedPtr<FSoundDashboardEntry> SoundPlaybackEntry = MakeShared<FSoundDashboardEntry>();
			SoundPlaybackEntry->RuntimeID = Msg.RuntimeID;
			SoundPlaybackEntry->PlayOrder = Msg.PlayOrder;
			SoundPlaybackEntry->Timestamp = Msg.Timestamp;
			SoundPlaybackEntry->Name = *Msg.Name;
			SoundPlaybackEntry->EntryType = Msg.EntryType;
			SoundPlaybackEntry->PinnedEntryType = (*OutActiveSoundEntry)->PinnedEntryType;
			SoundPlaybackEntry->bShouldForceExpandChildren = false;

			(*OutActiveSoundEntry)->Children.Add(SoundPlaybackEntry);
		});

		TArray<int32, TInlineAllocator<64>> EntriesWithPoppedDataPoints;

		ProcessMessageQueue<FSoundPriorityMessage>(TraceMessages.PriorityMessages, GetActiveSoundEntryFromParameterMessage,
		[this, &EntriesWithPoppedDataPoints](const FSoundPriorityMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(Msg.PlayOrder, OutActiveSoundEntry);
			if (SoundEntry == nullptr)
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->PriorityCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundTraceProviderPrivate::UpdateParameterEntry(Msg, Msg.Priority, SoundEntry->PriorityDataPoints, EntriesWithPoppedDataPoints);
		});
		EntriesWithPoppedDataPoints.Reset();

		ProcessMessageQueue<FSoundDistanceMessage>(TraceMessages.DistanceMessages, GetActiveSoundEntryFromParameterMessage,
		[this, &EntriesWithPoppedDataPoints](const FSoundDistanceMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(Msg.PlayOrder, OutActiveSoundEntry);
			if (SoundEntry == nullptr)
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->DistanceCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundTraceProviderPrivate::UpdateParameterEntry(Msg, Msg.Distance, SoundEntry->DistanceDataPoints, EntriesWithPoppedDataPoints);
		});
		EntriesWithPoppedDataPoints.Reset();

		ProcessMessageQueue<FMixerSourceEnvelopeMessage>(TraceMessages.AmplitudeMessages, GetActiveSoundEntryFromParameterMessage,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceEnvelopeMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(Msg.PlayOrder, OutActiveSoundEntry);
			if (SoundEntry == nullptr)
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->AmplitudeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundTraceProviderPrivate::UpdateParameterEntry(Msg, Msg.Envelope, SoundEntry->AmplitudeDataPoints, EntriesWithPoppedDataPoints);
		});
		EntriesWithPoppedDataPoints.Reset();

		ProcessMessageQueue<FMixerSourceVolumeMessage>(TraceMessages.VolumeMessages, GetActiveSoundEntryFromParameterMessage,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourceVolumeMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(Msg.PlayOrder, OutActiveSoundEntry);
			if (SoundEntry == nullptr)
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->VolumeCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundTraceProviderPrivate::UpdateParameterEntry(Msg, Msg.Volume, SoundEntry->VolumeDataPoints, EntriesWithPoppedDataPoints);
		});
		EntriesWithPoppedDataPoints.Reset();

		ProcessMessageQueue<FMixerSourcePitchMessage>(TraceMessages.PitchMessages, GetActiveSoundEntryFromParameterMessage,
		[this, &EntriesWithPoppedDataPoints](const FMixerSourcePitchMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
			FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(Msg.PlayOrder, OutActiveSoundEntry);
			if (SoundEntry == nullptr)
			{
				return;
			}

#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->PitchCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			FSoundTraceProviderPrivate::UpdateParameterEntry(Msg, Msg.Pitch, SoundEntry->PitchDataPoints, EntriesWithPoppedDataPoints);
		});

		ProcessMessageQueue<FSoundStopMessage>(TraceMessages.StopMessages, GetSoundEntryFromStopMessage,
		[this](const FSoundStopMessage& Msg, const TSharedPtr<FSoundDashboardEntry>* OutActiveSoundEntry)
		{
#if !WITH_EDITOR
			if (SessionCachedMessages.IsValid())
			{
				SessionCachedMessages->StopCachedMessages.EmplaceBack(Msg);
			}
#endif // !WITH_EDITOR

			if (OutActiveSoundEntry && OutActiveSoundEntry->IsValid())
			{
				if ((*OutActiveSoundEntry)->Timestamp < Msg.Timestamp)
				{
					static constexpr double DashboardTimeoutTime = 3.0;
					const double TimeoutTimestamp = FPlatformTime::Seconds() + DashboardTimeoutTime;

					(*OutActiveSoundEntry)->TimeoutTimestamp = TimeoutTimestamp;
					for (TSharedPtr<IDashboardDataTreeViewEntry> ChildEntry : (*OutActiveSoundEntry)->Children)
					{
						FSoundDashboardEntry& SoundPlaybackEntry = FSoundTraceProviderPrivate::CastEntry(*ChildEntry.Get());
						SoundPlaybackEntry.TimeoutTimestamp = TimeoutTimestamp;
					}

					EntriesTimingOut.Push(SoundMessageIDs{ Msg.RuntimeID, Msg.PlayOrder });
				}
			}
			else
			{
				// Keep track of any messages that have sent stop but no start message
				// In rare cases these can be processed out of order
				SoundsStoppedBeforeStart.Add(Msg.PlayOrder);
			}
		});

		return true;
	}

	bool FSoundTraceProvider::ProcessManuallyUpdatedEntries()
	{
		// Timeout old dashboard entries
		const double CurrentTimeStamp = FPlatformTime::Seconds();

		int32 NumToTrim = 0;
		for (const SoundMessageIDs SoundMessageIDs : EntriesTimingOut)
		{
			TSharedPtr<FSoundDashboardEntry> ActiveSoundEntry = nullptr;
			GetActiveSoundEntryFromIDs(SoundMessageIDs.PlayOrder, SoundMessageIDs.RuntimeID, ActiveSoundEntry);

			if (!ActiveSoundEntry.IsValid())
			{
				NumToTrim++;
				continue;
			}

			if (ActiveSoundEntry->TimeoutTimestamp <= CurrentTimeStamp)
			{
				RemoveActiveSoundEntry(SoundMessageIDs.RuntimeID, ActiveSoundEntry);
				NumToTrim++;
			}
			else
			{
				break;
			}
		}

		if (NumToTrim >= 0)
		{
			EntriesTimingOut.RemoveAt(0, NumToTrim, EAllowShrinking::No);
			return true;
		}

		return false;
	}

	void FSoundTraceProvider::GetOrCreateActiveSoundEntry(const FSoundStartMessage& Msg, TSharedPtr<FSoundDashboardEntry>& OutReturnedSoundEntry)
	{
		OutReturnedSoundEntry = nullptr;
		
		// We cannot always guarentee the order of receiving start and stop messages
		// If the stop message preceeded the start message, do not create a new entry
		if (SoundsStoppedBeforeStart.Contains(Msg.PlayOrder))
		{
			SoundsStoppedBeforeStart.Remove(Msg.PlayOrder);
			return;
		}

		UpdateRuntimeEntry(Msg.RuntimeID, Msg.EntryType, [this, &OutReturnedSoundEntry, &Msg](TSharedPtr<FSoundDashboardEntry>& CategoryEntry)
		{
			if (!CategoryEntry.IsValid())
			{
				CategoryEntry = MakeShared<FSoundDashboardEntry>();
				CategoryEntry->RuntimeID = Msg.RuntimeID;
				CategoryEntry->EntryType = Msg.EntryType;
				CategoryEntry->Name = FSoundTraceProviderPrivate::GetEntryTypeDisplayName(Msg.EntryType);
				CategoryEntry->bIsCategory = true;
			}

			TSharedPtr<IDashboardDataTreeViewEntry>* SoundEntry = CategoryEntry->Children.FindByPredicate([&Msg](TSharedPtr<IDashboardDataTreeViewEntry> ChildEntry)
				{
					FSoundDashboardEntry& NewEntry = FSoundTraceProviderPrivate::CastEntry(*ChildEntry.Get());
					return NewEntry.PlayOrder == Msg.PlayOrder;
				});

			if (SoundEntry == nullptr || !SoundEntry->IsValid())
			{
				TSharedPtr<FSoundDashboardEntry> NewEntry = MakeShared<FSoundDashboardEntry>();
				NewEntry->RuntimeID = Msg.RuntimeID;
				NewEntry->PlayOrder = Msg.PlayOrder;

				CategoryEntry->Children.Add(NewEntry);

				ActiveSoundToEntryKeysMap.Add(Msg.PlayOrder, SoundEntryKeys{ Msg.EntryType });

				OutReturnedSoundEntry = NewEntry;
			}
			else
			{
				TSharedPtr<FSoundDashboardEntry> NewEntry = StaticCastSharedPtr<FSoundDashboardEntry>(*SoundEntry);
				if (NewEntry->TimeoutTimestamp != INVALID_TIMEOUT)
				{
					NewEntry->TimeoutTimestamp = INVALID_TIMEOUT;

					EntriesTimingOut.RemoveAll([&Msg](const SoundMessageIDs& SoundMessageIDs)
					{
						return Msg.RuntimeID == SoundMessageIDs.RuntimeID && Msg.PlayOrder == SoundMessageIDs.PlayOrder;
					});

					// If this entry has SoundPlaybacks, they will replay with new play order IDs, so we need to clear the old ones out
					NewEntry->Children.RemoveAll([](TSharedPtr<IDashboardDataTreeViewEntry> ChildEntry)
					{
						FSoundDashboardEntry& SoundPlaybackEntry = FSoundTraceProviderPrivate::CastEntry(*ChildEntry.Get());
						return SoundPlaybackEntry.TimeoutTimestamp != INVALID_TIMEOUT;
					});
				}
				OutReturnedSoundEntry = NewEntry;
			}

			if (OutReturnedSoundEntry == nullptr)
			{
				return;
			}

			OutReturnedSoundEntry->Timestamp = Msg.Timestamp;
		});
	}

	void FSoundTraceProvider::GetActiveSoundEntryFromIDs(const uint32 ActiveSoundPlayOrder, const FAtomRuntimeId RuntimeID, TSharedPtr<FSoundDashboardEntry>& OutActiveSoundEntry)
	{
		const SoundEntryKeys* SoundEntryKeys = ActiveSoundToEntryKeysMap.Find(ActiveSoundPlayOrder);
		if (SoundEntryKeys == nullptr)
		{
			return;
		}

		TSharedPtr<FSoundDashboardEntry>* CategoryEntry = FindRuntimeEntry(RuntimeID, SoundEntryKeys->EntryType);

		FSoundTraceProviderPrivate::FindActiveSoundEntryInCategory(CategoryEntry, ActiveSoundPlayOrder, OutActiveSoundEntry);
	}

	void FSoundTraceProvider::RemoveActiveSoundEntry(const FAtomRuntimeId RuntimeID, TSharedPtr<FSoundDashboardEntry> OutActiveSoundEntry)
	{
		if (!OutActiveSoundEntry.IsValid())
		{
			return;
		}

		const uint32 ActiveSoundPlayOrder = OutActiveSoundEntry->PlayOrder;
		const SoundEntryKeys* SoundEntryKeys = ActiveSoundToEntryKeysMap.Find(ActiveSoundPlayOrder);

		if (SoundEntryKeys == nullptr)
		{
			return;
		}

		TSharedPtr<FSoundDashboardEntry>* CategoryEntry = FindRuntimeEntry(RuntimeID, SoundEntryKeys->EntryType);
		if (CategoryEntry == nullptr || !CategoryEntry->IsValid())
		{
			return;
		}

		(*CategoryEntry)->Children.Remove(OutActiveSoundEntry);

		if ((*CategoryEntry)->Children.Num() == 0)
		{
			RemoveRuntimeEntry(RuntimeID, OutActiveSoundEntry->EntryType);
		}

		ActiveSoundToEntryKeysMap.Remove(ActiveSoundPlayOrder);
	}

#if !WITH_EDITOR
	void FSoundTraceProvider::OnTimingViewTimeMarkerChanged(double TimeMarker)
	{
		using namespace FSoundTraceProviderPrivate;

		if (!SessionCachedMessages.IsValid())
		{
			return;
		}

		TSharedPtr<FSoundDashboardEntry> ActiveSoundEntryReturn = nullptr;
		auto GetOrCreateActiveSoundEntryFromActiveStartMessage = [this, &ActiveSoundEntryReturn](const FSoundStartMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			ActiveSoundEntryReturn = nullptr;

			GetOrCreateActiveSoundEntry(Msg, ActiveSoundEntryReturn);
			return &ActiveSoundEntryReturn;
		};

		TSharedPtr<FSoundDashboardEntry> StartMessageReturn = nullptr;
		auto GetActiveSoundEntryFromStartMessage = [this, &StartMessageReturn](const FSoundStartMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			StartMessageReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.PlayOrder, Msg.RuntimeID, StartMessageReturn);
			return &StartMessageReturn;
		};

		TSharedPtr<FSoundDashboardEntry> SoundEntryReturn = nullptr;
		auto GetActiveSoundEntryFromStopMessage = [this, &SoundEntryReturn](const FSoundStopMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			SoundEntryReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.PlayOrder, Msg.RuntimeID, SoundEntryReturn);
			return &SoundEntryReturn;
		};

		TSharedPtr<FSoundDashboardEntry> ParamReturn = nullptr;
		auto GetActiveSoundEntryFromParameterMessage = [this, &ParamReturn](const FMixerSourceParameterMessage& Msg) -> TSharedPtr<FSoundDashboardEntry>*
		{
			ParamReturn = nullptr;

			GetActiveSoundEntryFromIDs(Msg.ActiveSoundPlayOrder, Msg.RuntimeID, ParamReturn);
			return &ParamReturn;
		};

		// Collect all the start messages registered until this point in time 
		for (const FSoundStartMessage& StartCachedMessage : SessionCachedMessages->StartCachedMessages)
		{
			if (StartCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			TSharedPtr<FSoundDashboardEntry>* ActiveSoundEntry = GetOrCreateActiveSoundEntryFromActiveStartMessage(StartCachedMessage);
			if (ActiveSoundEntry == nullptr || !ActiveSoundEntry->IsValid())
			{
				continue;
			}

			FSoundDashboardEntry& EntryRef = *ActiveSoundEntry->Get();
			EntryRef.Name = *StartCachedMessage.Name;
			EntryRef.EntryType = StartCachedMessage.EntryType;
		}

		////////////////////////////////////////
		// Add all sound playback instance start messages registered until this point in time 
		TSharedPtr<FSoundDashboardEntry> ParentSoundEntryReturn = nullptr;
		for (const FSoundPlaybackStartMessage& SoundPlaybackStartCachedMessage : SessionCachedMessages->SoundPlaybackStartCachedMessages)
		{
			if (SoundPlaybackStartCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			if (!FSoundTraceProviderPrivate::EntryTypeHasSoundPlaybackEntries(SoundPlaybackStartCachedMessage.EntryType))
			{
				continue;
			}

			GetActiveSoundEntryFromIDs(SoundPlaybackStartCachedMessage.ActiveSoundPlayOrder, SoundPlaybackStartCachedMessage.RuntimeID, ParentSoundEntryReturn);

			if (!ParentSoundEntryReturn->IsValid())
			{
				continue;
			}

			TSharedPtr<FSoundDashboardEntry> ChildEntry = MakeShared<FSoundDashboardEntry>();
			ChildEntry->RuntimeID = SoundPlaybackStartCachedMessage.RuntimeID;
			ChildEntry->PlayOrder = SoundPlaybackStartCachedMessage.PlayOrder;
			ChildEntry->Timestamp = SoundPlaybackStartCachedMessage.Timestamp;
			ChildEntry->Name = *SoundPlaybackStartCachedMessage.Name;
			ChildEntry->EntryType = SoundPlaybackStartCachedMessage.EntryType;

			ParentSoundEntryReturn->Children.Add(ChildEntry);
		}

		// Selectively remove start messages collected in the step above by knowing which sounds were stopped.
		// With this we will know what are the active sounds at this point in time.
		for (const FSoundStopMessage& StopCachedMessage : SessionCachedMessages->StopCachedMessages)
		{
			if (StopCachedMessage.Timestamp > TimeMarker)
			{
				break;
			}

			TSharedPtr<FSoundDashboardEntry>* ActiveSoundEntry = GetActiveSoundEntryFromStopMessage(StopCachedMessage);

			if (ActiveSoundEntry && ActiveSoundEntry->IsValid() && (*ActiveSoundEntry)->Timestamp < StopCachedMessage.Timestamp)
			{
				RemoveActiveSoundEntry(StopCachedMessage.RuntimeID, *ActiveSoundEntry);
			}
		}

		// Re-run through start messages after stop messages have been processed
		// Any entries found with timestamps higher than the current time marker need to be cleared out too.
		for (const FSoundStartMessage& StartCachedMessage : SessionCachedMessages->StartCachedMessages)
		{
			if (StartCachedMessage.Timestamp > TimeMarker)
			{
				TSharedPtr<FSoundDashboardEntry>* FoundSoundEntry = GetActiveSoundEntryFromStartMessage(StartCachedMessage);

				if (FoundSoundEntry && FoundSoundEntry->IsValid())
				{
					RemoveActiveSoundEntry(StartCachedMessage.RuntimeID, *FoundSoundEntry);
				}
			}
		}
		
		// For now we only retrieve information from AtomRuntimeID 1 (main runtime in standalone games)
		const FRuntimeData* RuntimeData = RuntimeDataMap.Find(1);
		if (RuntimeData)
		{
			// Collect messages (volume, pitch, etc.) from active sounds (based on active sounds's PlayOrder)
			struct CachedEntryInfo
			{
				FSoundPriorityMessage PriorityMessage;
				FSoundDistanceMessage DistanceMessage;
				FMixerSourceEnvelopeMessage AmplitudeMessage;
				FMixerSourceVolumeMessage VolumeMessage;
				FMixerSourcePitchMessage PitchMessage;
			};

			TArray<uint32> PlayOrderArray;
			ActiveSoundToEntryKeysMap.GetKeys(PlayOrderArray);

			TArray<CachedEntryInfo> CachedEntryInfos;
			CachedEntryInfos.SetNumUninitialized(PlayOrderArray.Num());

			// Using ParallelFor to speed-up the cached messages retrieval, using a traditional for loop is unacceptably slower, specially in large traces.
			ParallelFor(PlayOrderArray.Num(), 
			[&PlayOrderArray, &CachedEntryInfos, TimeMarker, this](const int32 Index)
			{
				const uint32 PlayOrder = PlayOrderArray[Index];

				// Priority
				const FSoundPriorityMessage* FoundPriorityCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->PriorityCachedMessages, TimeMarker, PlayOrder);
				if (FoundPriorityCachedMessage)
				{
					CachedEntryInfos[Index].PriorityMessage = *FoundPriorityCachedMessage;
				}

				// Distance
				const FSoundDistanceMessage* FoundDistanceCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->DistanceCachedMessages, TimeMarker, PlayOrder);
				if (FoundDistanceCachedMessage)
				{
					CachedEntryInfos[Index].DistanceMessage = *FoundDistanceCachedMessage;
				}

				// Amplitude
				const FMixerSourceEnvelopeMessage* FoundAmplitudeCachedMessage = FindClosestMessageToTimestamp(SessionCachedMessages->AmplitudeCachedMessages, TimeMarker, PlayOrder);
				if (FoundAmplitudeCachedMessage)
				{
					CachedEntryInfos[Index].AmplitudeMessage = *FoundAmplitudeCachedMessage;
				}

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
			});

			// Update the runtime entries with the collected info
			for (const CachedEntryInfo& CachedEntryInfo : CachedEntryInfos)
			{
				TSharedPtr<FSoundDashboardEntry>* ActiveSoundEntry = GetActiveSoundEntryFromParameterMessage(CachedEntryInfo.PriorityMessage);
				
				if (ActiveSoundEntry == nullptr || !ActiveSoundEntry->IsValid())
				{
					continue;
				}

				FSoundDashboardEntry* SoundEntry = FSoundTraceProviderPrivate::FindSoundEntryForActiveSound(CachedEntryInfo.PriorityMessage.PlayOrder, ActiveSoundEntry);
				if (SoundEntry == nullptr)
				{
					continue;
				}

				SoundEntry->PriorityDataPoints.Push({ CachedEntryInfo.PriorityMessage.Timestamp, CachedEntryInfo.PriorityMessage.Priority });
				SoundEntry->DistanceDataPoints.Push({ CachedEntryInfo.DistanceMessage.Timestamp, CachedEntryInfo.DistanceMessage.Distance });
				SoundEntry->AmplitudeDataPoints.Push({ CachedEntryInfo.AmplitudeMessage.Timestamp, CachedEntryInfo.AmplitudeMessage.Envelope });
				SoundEntry->VolumeDataPoints.Push({ CachedEntryInfo.VolumeMessage.Timestamp, CachedEntryInfo.VolumeMessage.Volume });
				SoundEntry->PitchDataPoints.Push({ CachedEntryInfo.PitchMessage.Timestamp, CachedEntryInfo.PitchMessage.Pitch });

				SoundEntry->Timestamp = CachedEntryInfo.PriorityMessage.Timestamp;
			}
		}

		// Call parent method to update LastMessageId
		FTraceProviderBase::OnTimingViewTimeMarkerChanged(TimeMarker);
	}
#endif // !WITH_EDITOR

} // namespace UE::Audio::Insights
#undef LOCTEXT_NAMESPACE