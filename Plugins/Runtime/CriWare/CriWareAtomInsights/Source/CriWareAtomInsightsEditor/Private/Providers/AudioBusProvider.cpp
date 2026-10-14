
#include "Providers/AudioBusProvider.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Atom/AtomAudioBus.h"
#include "Atom/Mixer/AtomMixerTrace.h"

#include "AtomInsightsEditorDashboardFactory.h"
#include "CriWareAtomInsightsEditorModule.h"

namespace Atom::Insights
{
	FAudioBusProvider::FAudioBusProvider()
		: TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FAudioBusAssetDashboardEntry>>(GetName_Static())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FAudioBusProvider::OnAssetAdded);
		AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FAudioBusProvider::OnAssetRemoved);
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FAudioBusProvider::OnFilesLoaded);

		FEditorDashboardFactory::OnActiveAtomRuntimeChanged.AddRaw(this, &FAudioBusProvider::OnActiveAtomRuntimeChanged);

#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.AddRaw(this, &FAudioBusProvider::OnTraceStarted);
#endif
	}
	
	FAudioBusProvider::~FAudioBusProvider()
	{
		if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
		{
			AssetRegistryModule->Get().OnAssetAdded().RemoveAll(this);
			AssetRegistryModule->Get().OnAssetRemoved().RemoveAll(this);
			AssetRegistryModule->Get().OnFilesLoaded().RemoveAll(this);
		}

		FEditorDashboardFactory::OnActiveAtomRuntimeChanged.RemoveAll(this);

#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.RemoveAll(this);
#endif
	}

	FName FAudioBusProvider::GetName_Static()
	{
		return "AudioBusProvider";
	}

	void FAudioBusProvider::OnAssetAdded(const FAssetData& InAssetData)
	{
		if (bAreFilesLoaded && InAssetData.AssetClassPath == FTopLevelAssetPath(UAtomAudioBus::StaticClass()))
		{
			AddAudioBusAsset(InAssetData);
		}
	}

	void FAudioBusProvider::OnAssetRemoved(const FAssetData& InAssetData)
	{
		if (InAssetData.AssetClassPath == FTopLevelAssetPath(UAtomAudioBus::StaticClass()))
		{
			RemoveAudioBusAsset(InAssetData);
		}
	}

	void FAudioBusProvider::OnFilesLoaded()
	{
		bAreFilesLoaded = true;
		UpdateAudioBusAssetNames();
	}

	void FAudioBusProvider::OnActiveAtomRuntimeChanged()
	{
		UpdateAudioBusAssetNames();
	}

	void FAudioBusProvider::RequestEntriesUpdate()
	{
		UpdateAudioBusAssetNames();
	}

	void FAudioBusProvider::OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination)
	{
#if ATOM_PROFILERTRACE_ENABLED
		UpdateAudioBusAssetNames();
#endif
	}

	void FAudioBusProvider::AddAudioBusAsset(const FAssetData& InAssetData)
	{
		const bool bIsAudioBusAssetAlreadAdded = AudioBusDataViewEntries.ContainsByPredicate(
			[AssetName = InAssetData.GetObjectPathString()](const TSharedPtr<FAudioBusAssetDashboardEntry>& AudioBusAssetDashboardEntry)
			{
				return AudioBusAssetDashboardEntry->Name == AssetName;
			});

		if (!bIsAudioBusAssetAlreadAdded)
		{
			const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

			TSharedPtr<FAudioBusAssetDashboardEntry> AudioBusAssetDashboardEntry = MakeShared<FAudioBusAssetDashboardEntry>();
			AudioBusAssetDashboardEntry->RuntimeID = AtomRuntimeID;
			AudioBusAssetDashboardEntry->EntryType = EAudioBusEntryType::AssetBased;
			AudioBusAssetDashboardEntry->Name     = InAssetData.GetObjectPathString();
			AudioBusAssetDashboardEntry->AudioBus = Cast<UAtomAudioBus>(InAssetData.GetAsset());
			AudioBusAssetDashboardEntry->AudioBusID = AudioBusAssetDashboardEntry->AudioBus->GetUniqueID();

			AudioBusDataViewEntries.Add(MoveTemp(AudioBusAssetDashboardEntry));

			OnAudioBusAssetAdded.Broadcast(InAssetData.GetAsset());
			++LastUpdateID;
		}
	}

	void FAudioBusProvider::RemoveAudioBusAsset(const FAssetData& InAssetData)
	{
		const int32 FoundAudioBusAssetNameIndex = AudioBusDataViewEntries.IndexOfByPredicate(
			[AssetName = InAssetData.GetObjectPathString()](const TSharedPtr<FAudioBusAssetDashboardEntry>& AudioBusAssetDashboardEntry)
			{
				return AudioBusAssetDashboardEntry->Name == AssetName;
			});

		if (FoundAudioBusAssetNameIndex != INDEX_NONE)
		{
			const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

			RemoveRuntimeEntry(AtomRuntimeID, AudioBusDataViewEntries[FoundAudioBusAssetNameIndex]->AudioBus->GetUniqueID());

			AudioBusDataViewEntries.RemoveAt(FoundAudioBusAssetNameIndex);

			OnAudioBusAssetRemoved.Broadcast(InAssetData.GetAsset());
			++LastUpdateID;
		}
	}

	void FAudioBusProvider::UpdateAudioBusAssetNames()
	{
		// Get all UAtomAudioBus assets
		TArray<FAssetData> AssetDataArray;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UAtomAudioBus::StaticClass()), AssetDataArray);

		// Build AudioBusDataViewEntries
		Reset();
		AudioBusDataViewEntries.Empty();

		for (const FAssetData& AssetData : AssetDataArray)
		{
			AddAudioBusAsset(AssetData);
		}

		AudioBusDataViewEntries.Sort([](const TSharedPtr<FAudioBusAssetDashboardEntry>& A, const TSharedPtr<FAudioBusAssetDashboardEntry>& B)
		{
			return A->GetDisplayName().CompareToCaseIgnored(B->GetDisplayName()) < 0;
		});

		OnAudioBusAssetListUpdated.Broadcast();
	}

	bool FAudioBusProvider::ProcessMessages()
	{
		// Helper lambdas
		auto BumpEntryFunc = [this](const FAudioBusMessageBase& Msg)
		{
			TSharedPtr<FAudioBusAssetDashboardEntry>* ToReturn = nullptr;

			UpdateRuntimeEntry(Msg.RuntimeID, Msg.AudioBusID, [&ToReturn, &Msg](TSharedPtr<FAudioBusAssetDashboardEntry>& Entry)
			{
				if (!Entry.IsValid())
				{
					Entry = MakeShared<FAudioBusAssetDashboardEntry>();
					Entry->RuntimeID = Msg.RuntimeID;
					Entry->AudioBusID = Msg.AudioBusID;
				}

				Entry->Timestamp = Msg.Timestamp;

				ToReturn = &Entry;
			});

			return ToReturn;
		};

		auto GetEntry = [this](const FAudioBusMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.AudioBusID);
		};

		// Process messages
		if (bAssetEntriesNeedRefreshing)
		{
			const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

			for (const TSharedPtr<FAudioBusAssetDashboardEntry>& AudioBusDataViewEntry : AudioBusDataViewEntries)
			{
				if (AudioBusDataViewEntry.IsValid() && AudioBusDataViewEntry->AudioBus.IsValid())
				{
					UpdateRuntimeEntry(AtomRuntimeID, AudioBusDataViewEntry->AudioBusID, [&AudioBusDataViewEntry](TSharedPtr<FAudioBusAssetDashboardEntry>& Entry)
					{
						if (!Entry.IsValid())
						{
							Entry = AudioBusDataViewEntry;
						}
					});
				}
			}

			bAssetEntriesNeedRefreshing = false;
		}

		ProcessMessageQueue<FAudioBusStartMessage>(TraceMessages.StartMessages, BumpEntryFunc,
		[](const FAudioBusStartMessage& Msg, TSharedPtr<FAudioBusAssetDashboardEntry>* OutEntry)
		{
			FAudioBusAssetDashboardEntry& EntryRef = *OutEntry->Get();

			if (EntryRef.Name.IsEmpty())
			{
				EntryRef.Name = *Msg.Name;
			}

			if (EntryRef.NumChannels == 0)
			{
				EntryRef.NumChannels = Msg.NumChannels;
			}

			if (EntryRef.EntryType == EAudioBusEntryType::None)
			{
				EntryRef.EntryType = Msg.AudioBusType;
			}
		});

		ProcessMessageQueue<FAudioBusHasActivityMessage>(TraceMessages.HasActivityMessages, GetEntry,
		[](const FAudioBusHasActivityMessage& Msg, TSharedPtr<FAudioBusAssetDashboardEntry>* OutEntry)
		{
			if (OutEntry)
			{
				FAudioBusAssetDashboardEntry& EntryRef = *OutEntry->Get();

				if (!EntryRef.Name.IsEmpty() && EntryRef.EntryType != EAudioBusEntryType::None)
				{
					EntryRef.bHasActivity = Msg.bHasActivity;
					EntryRef.Timestamp = Msg.Timestamp;
				}
			}
		});

		ProcessMessageQueue<FAudioBusEnvelopeFollowerEnabledMessage>(TraceMessages.EnvelopeFollowerEnabledMessages, GetEntry,
		[](const FAudioBusEnvelopeFollowerEnabledMessage& Msg, TSharedPtr<FAudioBusAssetDashboardEntry>* OutEntry)
		{
			if (OutEntry)
			{
				FAudioBusAssetDashboardEntry& EntryRef = *OutEntry->Get();

				if (!EntryRef.Name.IsEmpty() && EntryRef.EntryType != EAudioBusEntryType::None)
				{
					EntryRef.bEnvelopeFollowerEnabled = Msg.bEnvelopeFollowerEnabled;
					EntryRef.Timestamp = Msg.Timestamp;
				}
			}
		});

		ProcessMessageQueue<FAudioBusStopMessage>(TraceMessages.StopMessages, GetEntry,
		[this](const FAudioBusStopMessage& Msg, TSharedPtr<FAudioBusAssetDashboardEntry>* OutEntry)
		{
			if (OutEntry && (*OutEntry)->Timestamp < Msg.Timestamp)
			{
				FAudioBusAssetDashboardEntry& EntryRef = *OutEntry->Get();

				if (EntryRef.EntryType == EAudioBusEntryType::CodeGenerated)
				{
					RemoveRuntimeEntry(Msg.RuntimeID, Msg.AudioBusID);
				}
			}
		});
		
		return true;
	}

	UE::Trace::IAnalyzer* FAudioBusProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FAudioBusTraceAnalyzer : public FTraceAnalyzerBase
		{
		public:
			FAudioBusTraceAnalyzer(TSharedRef<FAudioBusProvider> InProvider, TraceServices::IAnalysisSession & InSession)
				: FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const FOnAnalysisContext & Context) override
			{
				FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;

				Builder.RouteEvent(RouteId_Start,					"CriWareAtom", "AudioBusStart");
				Builder.RouteEvent(RouteId_HasActivity,				"CriWareAtom", "AudioBusHasActivity");
				Builder.RouteEvent(RouteId_EnvelopeFollowerEnabled,	"CriWareAtom", "AudioBusEnvelopeFollowerEnabled");
				Builder.RouteEvent(RouteId_EnvelopeValues,			"CriWareAtom", "AudioBusEnvelopeValues");
				Builder.RouteEvent(RouteId_Stop,					"CriWareAtom", "AudioBusStop");
			}

			virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext & Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("Insights/Atom::FAudioBusTraceAnalyzer"));

				FAudioBusMessages& Messages = GetProvider<FAudioBusProvider>().TraceMessages;

				switch (RouteId)
				{
					case RouteId_Start:
					{
						Messages.StartMessages.Enqueue(FAudioBusStartMessage { Context });
						break;
					}

					case RouteId_HasActivity:
					{
						Messages.HasActivityMessages.Enqueue(FAudioBusHasActivityMessage{ Context });
						break;
					}

					case RouteId_EnvelopeFollowerEnabled:
					{
						Messages.StopMessages.Enqueue(FAudioBusEnvelopeFollowerEnabledMessage{ Context });
						break;
					}

					case RouteId_EnvelopeValues:
					{
						Messages.StopMessages.Enqueue(FAudioBusEnvelopeValuesMessage{ Context });
						break;
					}

					case RouteId_Stop:
					{
						Messages.StopMessages.Enqueue(FAudioBusStopMessage{ Context });
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
				RouteId_Start,
				RouteId_HasActivity,
				RouteId_EnvelopeFollowerEnabled,
				RouteId_EnvelopeValues,
				RouteId_Stop
			};

			TraceServices::IAnalysisSession& Session;
		};

		bAssetEntriesNeedRefreshing = true;

		return new FAudioBusTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
