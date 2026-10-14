
#pragma once

#include <atomic>
#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Trace/Analyzer.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "UObject/NameTypes.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "ICriWareAtomInsightsModule.h"
#include "IAtomInsightsTraceModule.h"
#include "Messages/AnalyzerMessageQueue.h"

namespace Atom::Insights
{
	class CRIWAREATOMINSIGHTS_API FTraceProviderBase : public TraceServices::IProvider, public TraceServices::IEditableProvider
	{
	public:

		FTraceProviderBase() = delete;
		explicit FTraceProviderBase(FName InName);

		virtual ~FTraceProviderBase();

		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) = 0;
		FName GetName() const;

		virtual void Reset()
		{
			LastUpdateID = 0;
			LastMessageID = 0;
		}

		virtual bool ProcessMessages()
		{
			LastUpdateID = LastMessageID;
			return true;
		}

		virtual bool ProcessManuallyUpdatedEntries()
		{
			return false;
		};

		uint64 GetLastUpdateID() const
		{
			return LastUpdateID;
		}

		bool IsUpdated() const
		{
			return GetLastMessageID() == LastUpdateID;
		}

		bool ShouldForceUpdate() const
		{
			return bForceUpdate;
		}

		void ResetShouldForceUpdate()
		{
			bForceUpdate = false;
		}

#if !WITH_EDITOR
		virtual void InitSessionCachedMessages(TraceServices::IAnalysisSession& InSession) {}
		virtual void OnTimingViewTimeMarkerChanged(double TimeMarker) { ++LastMessageId; };
#endif // !WITH_EDITOR

	protected:

		class CRIWAREATOMINSIGHTS_API FTraceAnalyzerBase : public UE::Trace::IAnalyzer
		{
		public:

			FTraceAnalyzerBase(TSharedRef<FTraceProviderBase> InProvider);
			virtual ~FTraceAnalyzerBase() = default;

			virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override;

		protected:

			virtual bool OnEventSuccess(uint16 RouteId, EStyle Style, const FOnEventContext& Context);
			virtual bool OnEventFailure(uint16 RouteId, EStyle Style, const FOnEventContext& Context);

			template <typename TProviderType>
			TProviderType& GetProvider()
			{
				return *StaticCastSharedRef<TProviderType>(Provider);
			}

		private:

			TSharedRef<FTraceProviderBase> Provider;
		};

		uint64 GetLastMessageID() const
		{
			return LastMessageID;
		}

		uint64 LastUpdateID = 0;
		bool bForceUpdate = false;

	private:

		std::atomic<uint64> LastMessageID { 0 };
		FName Name;

		friend class FTraceAnalyzerBase;
	};

	template<typename EntryKey, typename EntryType /* = IDashboardDataViewEntry */>
	class TRuntimeDataMapTraceProvider : public FTraceProviderBase
	{
	public:

		using KeyType = EntryKey;
		using ValueType = EntryType;

		using FRuntimeData = TSortedMap<EntryKey, EntryType>;
		using FEntryPair = TPair<EntryKey, EntryType>;

		TRuntimeDataMapTraceProvider(FName InName)
			: FTraceProviderBase(InName)
		{
			TickerHandle = FTSTicker::GetCoreTicker().AddTicker(*InName.ToString(), 0.0f, [this](float DeltaTime)
			{
				if (LastUpdateID != GetLastMessageID())
				{
					ProcessMessages();
				}
				LastUpdateID = GetLastMessageID();

				const bool bShouldForceUpdate = ProcessManuallyUpdatedEntries();
				if (bShouldForceUpdate)
				{
					bForceUpdate = true;
				}

				return true;
			});

#if WITH_EDITOR
			RegisterDelegates();
#endif // WITH_EDITOR
		}

		virtual ~TRuntimeDataMapTraceProvider()
		{
#if WITH_EDITOR
			UnregisterDelegates();
#endif // WITH_EDITOR

			FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		}

		const TMap<FAtomRuntimeId, FRuntimeData>& GetRuntimeDataMap() const
		{
			return RuntimeDataMap;
		}

		const FRuntimeData* FindFilteredRuntimeData() const
		{
#if WITH_EDITOR
			const ICriWareAtomInsightsModule& AtomInsightsModule = ICriWareAtomInsightsModule::GetEditorChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsModule.GetRuntimeID();
#else
			const ICriWareAtomInsightsModule& AtomInsightsModule = ICriWareAtomInsightsModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsModule.GetRuntimeID();
#endif // WITH_EDITOR

			return RuntimeDataMap.Find(AtomRuntimeID);
		}

	protected:

		using Super = TRuntimeDataMapTraceProvider<EntryKey, EntryType>;

		TMap<FAtomRuntimeId, FRuntimeData> RuntimeDataMap;

		virtual void Reset() override
		{
			RuntimeDataMap.Empty();
			FTraceProviderBase::Reset();
		}

		template <typename TMsgType>
		void ProcessMessageQueue(
			TAnalyzerMessageQueue<TMsgType>& InQueue,
			TFunctionRef<EntryType*(const TMsgType&)> GetEntry,
			TFunctionRef<void(const TMsgType&, EntryType*)> ProcessEntry
		)
		{
#if WITH_EDITOR
			if (bIsTraceActive)
#endif // WITH_EDITOR
			{
				TArray<TMsgType> Messages = InQueue.DequeueAll();
				for (const TMsgType& Msg : Messages)
				{
					EntryType* Entry = GetEntry(Msg);
					ProcessEntry(Msg, Entry);
				};
			}
		}

		EntryType* FindRuntimeEntry(FAtomRuntimeId InRuntimeID, const EntryKey& InKey)
		{
			if (FRuntimeData* Entry = RuntimeDataMap.Find(InRuntimeID))
			{
				return Entry->Find(InKey);
			}

			return nullptr;
		}

		const EntryType* FindRuntimeEntry(FAtomRuntimeId InRuntimeID, const EntryKey& InKey) const
		{
			if (const FRuntimeData* Entry = RuntimeDataMap.Find(InRuntimeID))
			{
				return Entry->Find(InKey);
			}

			return nullptr;
		}

		FRuntimeData* FindFilteredRuntimeData()
		{
#if WITH_EDITOR
			const ICriWareAtomInsightsModule& AtomInsightsModule = ICriWareAtomInsightsModule::GetEditorChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsModule.GetRuntimeID();
#else
			const ICriWareAtomInsightsModule& AtomInsightsModule = ICriWareAtomInsightsModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsModule.GetRuntimeID();
#endif // WITH_EDITOR

			return RuntimeDataMap.Find(AtomRuntimeID);
		}

		bool RemoveRuntimeEntry(FAtomRuntimeId InRuntimeID, const EntryKey& InKey)
		{
			if (FRuntimeData* RuntimeData = RuntimeDataMap.Find(InRuntimeID))
			{
				if (RuntimeData->Remove(InKey) > 0)
				{
					if (RuntimeData->IsEmpty())
					{
						RuntimeDataMap.Remove(InRuntimeID);
					}

					return true;
				}
			}

			return false;
		}

		void UpdateRuntimeEntry(FAtomRuntimeId InRuntimeID, const EntryKey& InKey, TFunctionRef<void(EntryType&)> InEntryMutator)
		{
			FRuntimeData& RuntimeData = RuntimeDataMap.FindOrAdd(InRuntimeID);
			EntryType& Entry = RuntimeData.FindOrAdd(InKey);
			InEntryMutator(Entry);
		}

	private:

#if WITH_EDITOR
		void RegisterDelegates()
		{
			FTraceAuxiliary::OnTraceStarted.AddRaw(this, &TRuntimeDataMapTraceProvider::OnTraceStarted);
			FTraceAuxiliary::OnTraceStopped.AddRaw(this, &TRuntimeDataMapTraceProvider::OnTraceStopped);
			FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddRaw(this, &TRuntimeDataMapTraceProvider::OnAtomRuntimeDestroyed);
		}

		void UnregisterDelegates()
		{
			FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.RemoveAll(this);
			FTraceAuxiliary::OnTraceStopped.RemoveAll(this);
			FTraceAuxiliary::OnTraceStarted.RemoveAll(this);
		}

		void OnTraceStarted(FTraceAuxiliary::EConnectionType InTraceType, const FString& InTraceDestination)
		{
			bIsTraceActive = true;
		}

		void OnTraceStopped(FTraceAuxiliary::EConnectionType InTraceType, const FString& InTraceDestination)
		{
			bIsTraceActive = false;
			Reset();
		}

		void OnAtomRuntimeDestroyed(FAtomRuntimeId InRuntimeID)
		{
			RuntimeDataMap.Remove(InRuntimeID);
		}

		bool bIsTraceActive = false;
#endif // WITH_EDITOR

		FTSTicker::FDelegateHandle TickerHandle;
	};
} // namespace
