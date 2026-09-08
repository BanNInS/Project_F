
#pragma once

#include "Delegates/Delegate.h"
#include "UObject/NameTypes.h"
#include "ProfilingDebugging/TraceAuxiliary.h"

#include "Messages/AudioBusMessages.h"

namespace Atom::Insights
{
	class FAudioBusProvider : public TRuntimeDataMapTraceProvider<uint32, TSharedPtr<FAudioBusAssetDashboardEntry>>, public TSharedFromThis<FAudioBusProvider>
	{
	public:
		FAudioBusProvider();
		virtual ~FAudioBusProvider();

		static FName GetName_Static();

		virtual UE::Trace::IAnalyzer* ConstructAnalyzer(TraceServices::IAnalysisSession& InSession) override;

		void RequestEntriesUpdate();
		
		DECLARE_MULTICAST_DELEGATE_OneParam(FOnAudioBusAssetAdded, const TWeakObjectPtr<UObject> /*Asset*/);
		inline static FOnAudioBusAssetAdded OnAudioBusAssetAdded;

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnAudioBusAssetRemoved, const TWeakObjectPtr<UObject> /*Asset*/);
		inline static FOnAudioBusAssetRemoved OnAudioBusAssetRemoved;

		DECLARE_MULTICAST_DELEGATE(FOnAudioBusAssetListUpdated);
		inline static FOnAudioBusAssetListUpdated OnAudioBusAssetListUpdated;

	private:
		void OnAssetAdded(const FAssetData& InAssetData);
		void OnAssetRemoved(const FAssetData& InAssetData);
		void OnFilesLoaded();
		void OnActiveAtomRuntimeChanged();
		void OnTraceStarted(FTraceAuxiliary::EConnectionType TraceType, const FString& TraceDestination);

		void AddAudioBusAsset(const FAssetData& InAssetData);
		void RemoveAudioBusAsset(const FAssetData& InAssetData);

		void UpdateAudioBusAssetNames();

		virtual bool ProcessMessages() override;

		bool bAreFilesLoaded = false;
		bool bAssetEntriesNeedRefreshing = false;

		TArray<TSharedPtr<FAudioBusAssetDashboardEntry>> AudioBusDataViewEntries;

		FAudioBusMessages TraceMessages;
	};
} // namespace
