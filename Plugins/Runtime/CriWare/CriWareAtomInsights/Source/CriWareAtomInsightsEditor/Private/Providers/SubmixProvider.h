
#pragma once

#include "Delegates/Delegate.h"
#include "UObject/NameTypes.h"
#include "ProfilingDebugging/TraceAuxiliary.h"

#include "Messages/SubmixMessages.h"

namespace Atom::Insights
{
	class FSubmixProvider : public TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FSubmixAssetDashboardEntry>>, public TSharedFromThis<FSubmixProvider>
	{
	public:
		FSubmixProvider();
		virtual ~FSubmixProvider();

		static FName GetName_Static();

		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) override;
		
		void RequestEntriesUpdate();

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnSubmixAssetAdded, const uint32 /*SubmixID*/);
		inline static FOnSubmixAssetAdded OnSubmixAssetAdded;

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnSubmixAssetRemoved, const uint32 /*SubmixID*/);
		inline static FOnSubmixAssetRemoved OnSubmixAssetRemoved;

		DECLARE_MULTICAST_DELEGATE(FOnSubmixAssetListUpdated);
		inline static FOnSubmixAssetListUpdated OnSubmixAssetListUpdated;

	private:
		void OnAssetAdded(const FAssetData& InAssetData);
		void OnAssetRemoved(const FAssetData& InAssetData);
		void OnFilesLoaded();
		void OnActiveAtomRuntimeChanged();
		void OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination);

		void AddSubmixAsset(const FAssetData& InAssetData);
		void RemoveSubmixAsset(const FAssetData& InAssetData);

		void UpdateSubmixAssetNames();

		virtual bool ProcessMessages() override;

		bool bAreFilesLoaded = false;
		bool bAssetEntriesNeedRefreshing = false;

		TArray<TSharedPtr<FSubmixAssetDashboardEntry>> SubmixDataViewEntries;

		FSubmixMessages TraceMessages;
	};
} // namespace
