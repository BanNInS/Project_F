
#include "Providers/SubmixProvider.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/Mixer/AtomMixerTrace.h"

#include "CriWareAtomInsightsEditorModule.h"
#include "AtomInsightsEditorDashboardFactory.h"

namespace Atom::Insights
{
	FSubmixProvider::FSubmixProvider()
		: TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FSubmixAssetDashboardEntry>>(GetName_Static())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FSubmixProvider::OnAssetAdded);
		AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FSubmixProvider::OnAssetRemoved);
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FSubmixProvider::OnFilesLoaded);

		FEditorDashboardFactory::OnActiveAtomRuntimeChanged.AddRaw(this, &FSubmixProvider::OnActiveAtomRuntimeChanged);

#if ATOM_PROFILERTRACE_ENABLED
		FTraceAuxiliary::OnTraceStarted.AddRaw(this, &FSubmixProvider::OnTraceStarted);
#endif
	}
	
	FSubmixProvider::~FSubmixProvider()
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

	FName FSubmixProvider::GetName_Static()
	{
		return "RacksProvider";
	}

	void FSubmixProvider::OnAssetAdded(const FAssetData& InAssetData)
	{
		if (bAreFilesLoaded && InAssetData.AssetClassPath == FTopLevelAssetPath(UAtomRackBase::StaticClass()))
		{
			AddSubmixAsset(InAssetData);
		}
	}

	void FSubmixProvider::OnAssetRemoved(const FAssetData& InAssetData)
	{
		if (InAssetData.AssetClassPath == FTopLevelAssetPath(UAtomRackBase::StaticClass()))
		{
			RemoveSubmixAsset(InAssetData);
		}
	}

	void FSubmixProvider::OnFilesLoaded()
	{
		bAreFilesLoaded = true;
		UpdateSubmixAssetNames();
	}

	void FSubmixProvider::OnActiveAtomRuntimeChanged()
	{
		UpdateSubmixAssetNames();
	}

	void FSubmixProvider::RequestEntriesUpdate()
	{
		UpdateSubmixAssetNames();
	}

	void FSubmixProvider::OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination)
	{
#if ATOM_PROFILERTRACE_ENABLED
		UpdateSubmixAssetNames();
#endif
	}

	void FSubmixProvider::AddSubmixAsset(const FAssetData& InAssetData)
	{
		const bool bIsSubmixAssetAlreadAdded = SubmixDataViewEntries.ContainsByPredicate(
			[AssetName = InAssetData.GetObjectPathString()](const TSharedPtr<FSubmixAssetDashboardEntry>& SoundSubmixAssetDashboardEntry)
			{
				return SoundSubmixAssetDashboardEntry->Name == AssetName;
			});

		if (!bIsSubmixAssetAlreadAdded)
		{
			const ICriWareAtomInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<ICriWareAtomInsightsModule>(ICriWareAtomInsightsModule::GetName());
			const FAtomRuntimeId RuntimeID = InsightsModule.GetRuntimeID();

			TSharedPtr<FSubmixAssetDashboardEntry> SubmixAssetDashboardEntry = SubmixDataViewEntries.Add_GetRef(MakeShared<FSubmixAssetDashboardEntry>());
			SubmixAssetDashboardEntry->RuntimeID = RuntimeID;
			SubmixAssetDashboardEntry->Name = InAssetData.GetObjectPathString();
			SubmixAssetDashboardEntry->SubmixID = GetTypeHash(SubmixAssetDashboardEntry->Name);

			OnSubmixAssetAdded.Broadcast(SubmixAssetDashboardEntry->SubmixID);

			// Get all bus assets associated with this rack
			if (InAssetData.GetClass() == UAtomRack::StaticClass())
			{
				if (UAtomRack* Rack = Cast<UAtomRack>(InAssetData.GetAsset()))
				{
					for (UAtomBus* Bus : Rack->Buses)
					{
						if (Bus && !Bus->IsMainBus())
						{
							TSharedPtr<FSubmixAssetDashboardEntry> BusSubmixAssetDashboardEntry = SubmixDataViewEntries.Add_GetRef(MakeShared<FSubmixAssetDashboardEntry>());
							BusSubmixAssetDashboardEntry->RuntimeID = RuntimeID;
							BusSubmixAssetDashboardEntry->Name = Bus->GetPathName();
							BusSubmixAssetDashboardEntry->SubmixID = GetTypeHash(BusSubmixAssetDashboardEntry->Name);
							BusSubmixAssetDashboardEntry->bIsBus = true;

							OnSubmixAssetAdded.Broadcast(BusSubmixAssetDashboardEntry->SubmixID);
						}
					}
				}
			}

			bAssetEntriesNeedRefreshing = true;
			++LastUpdateID;
		}
	}

	void FSubmixProvider::RemoveSubmixAsset(const FAssetData& InAssetData)
	{
		const int32 FoundSubmixAssetNameIndex = SubmixDataViewEntries.IndexOfByPredicate(
			[AssetName = InAssetData.GetObjectPathString()](const TSharedPtr<FSubmixAssetDashboardEntry>& SoundSubmixAssetDashboardEntry)
			{
				return SoundSubmixAssetDashboardEntry->Name == AssetName;
			});

		if (FoundSubmixAssetNameIndex != INDEX_NONE)
		{
			const ICriWareAtomInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<ICriWareAtomInsightsModule>(ICriWareAtomInsightsModule::GetName());
			const FAtomRuntimeId RuntimeID = InsightsModule.GetRuntimeID();

			const uint32 SubmixID = SubmixDataViewEntries[FoundSubmixAssetNameIndex]->SubmixID;

			RemoveRuntimeEntry(RuntimeID, SubmixID);

			SubmixDataViewEntries.RemoveAt(FoundSubmixAssetNameIndex);

			OnSubmixAssetRemoved.Broadcast(SubmixID);

			// Get all bus assets associated with this rack
			if (InAssetData.GetClass() == UAtomRack::StaticClass())
			{
				if (UAtomRack* Rack = Cast<UAtomRack>(InAssetData.GetAsset()))
				{
					for (UAtomBus* Bus : Rack->Buses)
					{
						if (Bus && !Bus->IsMainBus())
						{
							const int32 FoundBusSubmixAssetNameIndex = SubmixDataViewEntries.IndexOfByPredicate(
								[AssetName = Bus->GetPathName()](const TSharedPtr<FSubmixAssetDashboardEntry>& SoundSubmixAssetDashboardEntry)
								{
									return SoundSubmixAssetDashboardEntry->Name == AssetName;
								});

							if (FoundBusSubmixAssetNameIndex != INDEX_NONE)
							{
								const uint32 BusSubmixID = SubmixDataViewEntries[FoundBusSubmixAssetNameIndex]->SubmixID;

								RemoveRuntimeEntry(RuntimeID, BusSubmixID);

								SubmixDataViewEntries.RemoveAt(FoundBusSubmixAssetNameIndex);

								OnSubmixAssetRemoved.Broadcast(BusSubmixID);
							}
						}
					}
				}
			}

			bAssetEntriesNeedRefreshing = true;
			++LastUpdateID;
		}
	}

	void FSubmixProvider::UpdateSubmixAssetNames()
	{
		// Get all UAtomRackBase assets
		TArray<FAssetData> AssetDataArray;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().GetAssetsByClass(FTopLevelAssetPath(UAtomRackWithParentBase::StaticClass()), AssetDataArray, true);

		// Build SubmixDataViewEntries
		Reset();
		SubmixDataViewEntries.Empty();

		for (const FAssetData& AssetData : AssetDataArray)
		{
			AddSubmixAsset(AssetData);
		}

		SubmixDataViewEntries.Sort([](const TSharedPtr<FSubmixAssetDashboardEntry>& A, const TSharedPtr<FSubmixAssetDashboardEntry>& B)
		{
			return A->GetDisplayName().CompareToCaseIgnored(B->GetDisplayName()) < 0;
		});

		OnSubmixAssetListUpdated.Broadcast();
	}

	bool FSubmixProvider::ProcessMessages()
	{
		// Helper lambdas
		auto GetEntry = [this](const FSubmixMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.SubmixID);
		};

		// Process messages
		if (bAssetEntriesNeedRefreshing)
		{

			const ICriWareAtomInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<ICriWareAtomInsightsModule>(ICriWareAtomInsightsModule::GetName());
			const FAtomRuntimeId RuntimeID = InsightsModule.GetRuntimeID();

			for (const TSharedPtr<FSubmixAssetDashboardEntry>& SubmixDataViewEntry : SubmixDataViewEntries)
			{
				if (SubmixDataViewEntry.IsValid())
				{
					UpdateRuntimeEntry(RuntimeID, SubmixDataViewEntry->SubmixID, [&SubmixDataViewEntry](TSharedPtr<FSubmixAssetDashboardEntry>& Entry)
					{
						if (!Entry.IsValid())
						{
							Entry = SubmixDataViewEntry;
						}
					});
				}
			}

			bAssetEntriesNeedRefreshing = false;
		}

		ProcessMessageQueue<FSubmixHasActivityMessage>(TraceMessages.HasActivityMessages, GetEntry,
		[](const FSubmixHasActivityMessage& Msg, TSharedPtr<FSubmixAssetDashboardEntry>* OutEntry)
		{
			if (OutEntry)
			{
				FSubmixAssetDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.bHasActivity = Msg.bHasActivity;
				EntryRef.Timestamp = Msg.Timestamp;
			}
		});

		return true;
	}

	UE::Trace::IAnalyzer* FSubmixProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FSubmixTraceAnalyzer : public FTraceAnalyzerBase
		{
		public:
			FSubmixTraceAnalyzer(TSharedRef<FSubmixProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override
			{
				FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;

				Builder.RouteEvent(RouteId_HasActivity, "CriWareAtom", "SubmixHasActivity");
			}

			virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("Insights/Atom::FSubmixTraceAnalyzer"));

				FSubmixMessages& Messages = GetProvider<FSubmixProvider>().TraceMessages;

				switch (RouteId)
				{
					case RouteId_HasActivity:
					{
						Messages.HasActivityMessages.Enqueue(FSubmixHasActivityMessage{ Context });
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
				RouteId_HasActivity
			};

			TraceServices::IAnalysisSession& Session;
		};

		bAssetEntriesNeedRefreshing = true;

		return new FSubmixTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
