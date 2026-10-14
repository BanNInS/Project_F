
#include "Providers/ControlBusTraceProvider.h"

#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/ModuleService.h"


namespace Atom::Insights
{
	FName FControlBusTraceProvider::GetName_Static()
	{
		return "AtomModulationControlBusProvider";
	}

	bool FControlBusTraceProvider::ProcessMessages()
	{
		auto BumpEntryFunc = [this](const FControlBusMessageBase& Msg)
		{
			TSharedPtr<FControlBusDashboardEntry>* ToReturn = nullptr;
			UpdateRuntimeEntry(Msg.RuntimeID, Msg.ControlBusID, [&ToReturn, &Msg](TSharedPtr<FControlBusDashboardEntry>& Entry)
			{
				if (!Entry.IsValid())
				{
					Entry = MakeShared<FControlBusDashboardEntry>();
					Entry->RuntimeID = Msg.RuntimeID;
					Entry->ControlBusID = Msg.ControlBusID;
				}
				ToReturn = &Entry;
			});

			return ToReturn;
		};

		ProcessMessageQueue<FControlBusActivateMessage>(TraceMessages.ActivateMessages, BumpEntryFunc,
		[](const FControlBusActivateMessage& Msg, TSharedPtr<FControlBusDashboardEntry>* OutEntry)
		{
			FControlBusDashboardEntry& EntryRef = *OutEntry->Get();
			EntryRef.Name = *Msg.BusName;
			EntryRef.ControlBusID = Msg.ControlBusID;
			EntryRef.ParamName = Msg.ParamName;
		});

		ProcessMessageQueue<FControlBusUpdateMessage>(TraceMessages.UpdateMessages, BumpEntryFunc,
		[](const FControlBusUpdateMessage& Msg, TSharedPtr<FControlBusDashboardEntry>* OutEntry)
		{
			FControlBusDashboardEntry& EntryRef = *OutEntry->Get();
			EntryRef.Name = *Msg.BusName;
			EntryRef.ParamName = Msg.ParamName;
			EntryRef.Value = Msg.Value;
		});

		auto GetEntry = [this](const FControlBusMessageBase& Msg)
		{
			return FindRuntimeEntry(Msg.RuntimeID, Msg.ControlBusID);
		};

		ProcessMessageQueue<FControlBusDeactivateMessage>(TraceMessages.DeactivateMessages, GetEntry,
		[this](const FControlBusDeactivateMessage& Msg, TSharedPtr<FControlBusDashboardEntry>* OutEntry)
		{
			if (OutEntry && (*OutEntry)->Timestamp < Msg.Timestamp)
			{
				RemoveRuntimeEntry(Msg.RuntimeID, Msg.ControlBusID);
			}
		});

		return true;
	}

	UE::Trace::IAnalyzer* FControlBusTraceProvider::ConstructAnalyzer(TraceServices::IAnalysisSession& InSession)
	{
		class FControlBusTraceAnalyzer : public FTraceProviderBase::FTraceAnalyzerBase
		{
		public:
			FControlBusTraceAnalyzer(TSharedRef<FControlBusTraceProvider> InProvider, TraceServices::IAnalysisSession& InSession)
				: FTraceProviderBase::FTraceAnalyzerBase(InProvider)
				, Session(InSession)
			{
			}

			virtual void OnAnalysisBegin(const UE::Trace::IAnalyzer::FOnAnalysisContext& Context) override
			{
				FTraceProviderBase::FTraceAnalyzerBase::OnAnalysisBegin(Context);

				UE::Trace::IAnalyzer::FInterfaceBuilder& Builder = Context.InterfaceBuilder;
				Builder.RouteEvent(RouteId_Activate, "CriWareAtom", "ControlBusActivate");
				Builder.RouteEvent(RouteId_Deactivate, "CriWareAtom", "ControlBusDeactivate");
				Builder.RouteEvent(RouteId_Update, "CriWareAtom", "ControlBusUpdate");
			}

			virtual bool OnEvent(uint16 RouteId, UE::Trace::IAnalyzer::EStyle Style, const UE::Trace::IAnalyzer::FOnEventContext& Context) override
			{
				LLM_SCOPE_BYNAME(TEXT("AtomInsights/FControlBusTraceAnalyzer"));

				FControlBusMessages& Messages = GetProvider<FControlBusTraceProvider>().TraceMessages;
				switch (RouteId)
				{
					case RouteId_Activate:
					{
						Messages.ActivateMessages.Enqueue(FControlBusActivateMessage { Context });
						break;
					}

					case RouteId_Deactivate:
					{
						Messages.DeactivateMessages.Enqueue(FControlBusDeactivateMessage{ Context });
						break;
					}

					case RouteId_Update:
					{
						Messages.UpdateMessages.Enqueue(FControlBusUpdateMessage { Context });
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
				RouteId_Activate,
				RouteId_Deactivate,
				RouteId_Update
			};

			TraceServices::IAnalysisSession& Session;
		};

		return new FControlBusTraceAnalyzer(AsShared(), InSession);
	}
} // namespace
