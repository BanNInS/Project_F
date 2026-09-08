
#include "Providers/ModulationMatrixTraceProvider.h"

#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"

#include "Atom/AtomRuntimeManager.h"

namespace Atom::Insights
{
	FModulationMatrixTraceProvider::FModulationMatrixTraceProvider()
		: TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FModulationMatrixDashboardEntry>>(GetName_Static())
	{
		FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddRaw(this, &FModulationMatrixTraceProvider::OnAtomRuntimeDestroyed);
	}

	FModulationMatrixTraceProvider::~FModulationMatrixTraceProvider()
	{
		FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.RemoveAll(this);
	}

	FName FModulationMatrixTraceProvider::GetName_Static()
	{
		static const FName ModulationMatrixProviderName = "AtomModulationMatrixProvider";
		return ModulationMatrixProviderName;
	}

	void FModulationMatrixTraceProvider::UpdateActiveControlBusesToAdd(const TMap<FBusId, FString>& InBusIdToBusNameMap)
	{
		for (const auto& [BusId, BusName] : InBusIdToBusNameMap)
		{
			if (BusInfo* FoundBusInfoPtr = ActiveControlBuses.Find(BusId))
			{
				BusInfo& FoundBusInfo = *FoundBusInfoPtr;
				FoundBusInfo.RefCount++;
			}
			else
			{
				ActiveControlBuses.Emplace(BusId, { BusName, 1 });
			}
		}

		ActiveControlBuses.ValueSort([](const BusInfo& A, const BusInfo& B)
		{
			return A.BusName.ToLower() < B.BusName.ToLower();
		});
	}

	void FModulationMatrixTraceProvider::UpdateActiveControlBusesToRemove(const TMap<FBusId, float>& InBusIdToValueMap)
	{
		TArray<FBusId> BusesPendingToRemove;

		for (const auto& [BusId, BusValue] : InBusIdToValueMap)
		{
			if (BusInfo* FoundBusInfoPtr = ActiveControlBuses.Find(BusId))
			{
				BusInfo& FoundBusInfo = *FoundBusInfoPtr;
				FoundBusInfo.RefCount--;

				if (FoundBusInfo.RefCount <= 0)
				{
					RemovedControlBusesNames.Emplace(FoundBusInfo.BusName);
					BusesPendingToRemove.Emplace(BusId);
				}
			}
		}

		for (const FBusId& BusId : BusesPendingToRemove)
		{
			ActiveControlBuses.Remove(BusId);
		}
	}

	bool FModulationMatrixTraceProvider::ProcessMessages()
	{
		// Helper lambdas
		auto BumpEntryFunc = [this](const FModulationMatrixMessageBase& Msg)
		{
			TSharedPtr<FModulationMatrixDashboardEntry>* ToReturn = nullptr;

			UpdateRuntimeEntry(Msg.RuntimeID, Msg.SourceID, [&ToReturn, &Msg](TSharedPtr<FModulationMatrixDashboardEntry>& Entry)
			{
				if (!Entry.IsValid())
				{
					Entry = MakeShared<FModulationMatrixDashboardEntry>();
					Entry->RuntimeID = Msg.RuntimeID;
					Entry->SourceID = Msg.SourceID;
				}

				Entry->Timestamp = Msg.Timestamp;

				ToReturn = &Entry;
			});

			return ToReturn;
		};

		auto GetEntry = [this](const FModulationMatrixMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.SourceID);
		};

		// Process messages
		uint32 NumActiveBuses = ActiveControlBuses.Num();

		if (!(TraceMessages.RegisterBusMessages.IsEmpty() && (TraceMessages.BusMixActivateMessages.IsEmpty() || TraceMessages.GeneratorActivateMessages.IsEmpty())))
		{
			TMap<FSourceId, TMap<FBusId, FString>> ModulatingSourceIDToBusesInfoMap;

			auto UpdateActiveModulatorSourceIds = [&](const Audio::FDeviceId DeviceId, const FSourceId SourceId)
			{
				TSet<uint32>& ActiveModulatorSourceIds = RuntimeIDToActiveModulatorSourceIDsMap.FindOrAdd(DeviceId);

				if (!ActiveModulatorSourceIds.Contains(SourceId))
				{
					ActiveModulatorSourceIds.Add(SourceId);

					if (const TMap<FBusId, FString>* FoundBusInfoPtr = ModulatingSourceIDToBusesInfoMap.Find(SourceId))
					{
						UpdateActiveControlBusesToAdd(*FoundBusInfoPtr);
					}
				}
			};

			ProcessMessageQueue<FModulationMatrixRegisterBusMessage>(TraceMessages.RegisterBusMessages,
			[](const FModulationMatrixMessageBase& Msg)	{ return nullptr; }, // No entry operation
			[this, &ModulatingSourceIDToBusesInfoMap](const FModulationMatrixRegisterBusMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
			{
					UE_LOG(LogTemp, Warning, TEXT("RegisterBus or gene"));

				if (TMap<FBusId, FString>* FoundBusInfoPtr = ModulatingSourceIDToBusesInfoMap.Find(Msg.ModulatingSourceID))
				{
					FoundBusInfoPtr->FindOrAdd(Msg.SourceID, Msg.BusName);
				}
				else
				{
					ModulatingSourceIDToBusesInfoMap.Emplace(Msg.ModulatingSourceID).Emplace(Msg.SourceID, Msg.BusName);
				}
			});

			ProcessMessageQueue<FBusMixActivateMessage>(TraceMessages.BusMixActivateMessages, BumpEntryFunc,
			[this, &UpdateActiveModulatorSourceIds](const FBusMixActivateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
			{
				FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.Name = *Msg.Name;
				EntryRef.EntryType = EModulationMatrixEntryType::BusMix;

				UpdateActiveModulatorSourceIds(Msg.RuntimeID, EntryRef.SourceID);
			});

			ProcessMessageQueue<FGeneratorActivateMessage>(TraceMessages.GeneratorActivateMessages, BumpEntryFunc,
			[this, &UpdateActiveModulatorSourceIds](const FGeneratorActivateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
			{
				UE_LOG(LogTemp, Warning, TEXT("GeneratorActivate"));

				FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.Name = *Msg.Name;
				EntryRef.EntryType = EModulationMatrixEntryType::Generator;

				UpdateActiveModulatorSourceIds(Msg.RuntimeID, EntryRef.SourceID);
			});
		}

		ProcessMessageQueue<FModulationMatrixDeactivateMessage>(TraceMessages.DeactivateMessages, GetEntry,
		[this](const FModulationMatrixDeactivateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
		{
			if (OutEntry && (*OutEntry)->Timestamp < Msg.Timestamp)
			{
				const FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();
				UpdateActiveControlBusesToRemove(EntryRef.BusIDToValueMap);

				if (RuntimeIDToActiveModulatorSourceIDsMap.Contains(Msg.RuntimeID))
				{
					RuntimeIDToActiveModulatorSourceIDsMap[Msg.RuntimeID].Remove(EntryRef.SourceID);
				}

				RemoveRuntimeEntry(Msg.RuntimeID, Msg.SourceID);
			}
		});

		ProcessMessageQueue<FBusMixUpdateMessage>(TraceMessages.BusMixUpdateMessages, GetEntry,
		[this](const FBusMixUpdateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
		{
			if (OutEntry)
			{
				FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.BusIDToValueMap = Msg.BusIDToValueMap;
				EntryRef.Timestamp = Msg.Timestamp;
			}
		});

		ProcessMessageQueue<FGeneratorUpdateMessage>(TraceMessages.GeneratorUpdateMessages, GetEntry,
		[this](const FGeneratorUpdateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
		{
				UE_LOG(LogTemp, Warning, TEXT("GeneratorUpdate"));
			if (OutEntry)
			{
				UE_LOG(LogTemp, Warning, TEXT("GeneratorUpdate"));
				FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();
				EntryRef.BusIDToValueMap = Msg.BusIDToValueMap;
				EntryRef.Timestamp = Msg.Timestamp;
			}
		});

		ProcessMessageQueue<FBusFinalValuesUpdateMessage>(TraceMessages.BusFinalValuesUpdateMessages, BumpEntryFunc,
		[this](const FBusFinalValuesUpdateMessage& Msg, TSharedPtr<FModulationMatrixDashboardEntry>* OutEntry)
		{
			FModulationMatrixDashboardEntry& EntryRef = *OutEntry->Get();

			if (EntryRef.Name.IsEmpty())
			{
				static const FString BusFinalValuesEntryName("Final Values: ");
				EntryRef.Name      = BusFinalValuesEntryName;
				EntryRef.EntryType = EModulationMatrixEntryType::BusFinalValues;
			}

			EntryRef.BusIDToValueMap = Msg.BusIDToValueMap;
		});

		// Notify Control Buses update
		if (NumActiveBuses != ActiveControlBuses.Num() && !ActiveControlBuses.IsEmpty())
		{
			OnControlBusesAdded.ExecuteIfBound(ActiveControlBuses);
		}

		if (!RemovedControlBusesNames.IsEmpty())
		{
			OnControlBusesRemoved.ExecuteIfBound(RemovedControlBusesNames);
			RemovedControlBusesNames.Reset();
		}

		return true;
	}

	void FModulationMatrixTraceProvider::OnAtomRuntimeDestroyed(FAtomRuntimeId InRuntimeID)
	{
		RuntimeIDToActiveModulatorSourceIDsMap.Reset();
		ActiveControlBuses.Reset();
	}

	UE::Trace::IAnalyzer* FModulationMatrixTraceProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FModulationMatrixTraceAnalyzer : public FTraceProviderBase::FTraceAnalyzerBase
		{
		public:
			FModulationMatrixTraceAnalyzer(TSharedRef<FModulationMatrixTraceProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceProviderBase::FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const UE::Trace::IAnalyzer::FOnAnalysisContext& Context) override
			{
				FTraceProviderBase::FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;

				Builder.RouteEvent(RouteId_BusMixRegisterBus,    "CriWareAtom", "BusMixRegisterBus");
				Builder.RouteEvent(RouteId_GeneratorRegisterBus, "CriWareAtom", "GeneratorRegisterBus");

				Builder.RouteEvent(RouteId_BusMixActivate,       "CriWareAtom", "BusMixActivate");
				Builder.RouteEvent(RouteId_GeneratorActivate,    "CriWareAtom", "GeneratorActivate");

				Builder.RouteEvent(RouteId_BusMixUpdate,         "CriWareAtom", "BusMixUpdate");
				Builder.RouteEvent(RouteId_GeneratorUpdate,      "CriWareAtom", "GeneratorUpdate");
				Builder.RouteEvent(RouteId_BusFinalValuesUpdate, "CriWareAtom", "BusFinalValuesUpdate");

				Builder.RouteEvent(RouteId_Deactivate, "CriWareAtom", "ModulatingSourceDeactivate");
			}

			virtual bool OnEvent(uint16 RouteId, UE::Trace::IAnalyzer::EStyle Style, const UE::Trace::IAnalyzer::FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("AtomInsights/FModulationMatrixTraceAnalyzer"));

				FModulationMatrixMessages& Messages = GetProvider<FModulationMatrixTraceProvider>().TraceMessages;

				switch (RouteId)
				{
					case RouteId_BusMixRegisterBus:
					case RouteId_GeneratorRegisterBus:
					{
						Messages.RegisterBusMessages.Enqueue(FModulationMatrixRegisterBusMessage{ Context });
						break;
					}

					case RouteId_BusMixActivate:
					{
						Messages.BusMixActivateMessages.Enqueue(FBusMixActivateMessage{ Context });
						break;
					}

					case RouteId_GeneratorActivate:
					{
						Messages.GeneratorActivateMessages.Enqueue(FGeneratorActivateMessage{ Context });
						break;
					}

					case RouteId_BusMixUpdate:
					{
						Messages.BusMixUpdateMessages.Enqueue(FBusMixUpdateMessage{ Context });
						break;
					}

					case RouteId_GeneratorUpdate:
					{
						Messages.GeneratorUpdateMessages.Enqueue(FGeneratorUpdateMessage{ Context });
						break;
					}

					case RouteId_BusFinalValuesUpdate:
					{
						Messages.BusFinalValuesUpdateMessages.Enqueue(FBusFinalValuesUpdateMessage{ Context });
						break;
					}

					case RouteId_Deactivate:
					{
						Messages.DeactivateMessages.Enqueue(FModulationMatrixDeactivateMessage{ Context });
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
				RouteId_BusMixRegisterBus,
				RouteId_GeneratorRegisterBus,

				RouteId_BusMixActivate,
				RouteId_GeneratorActivate,

				RouteId_BusMixUpdate,
				RouteId_GeneratorUpdate,
				RouteId_BusFinalValuesUpdate,

				RouteId_Deactivate
			};

			TraceServices::IAnalysisSession& Session;
		};

		return new FModulationMatrixTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
