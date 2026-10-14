
#pragma once

#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/ModuleService.h"
#include "UObject/NameTypes.h"

#include "IAtomInsightsTraceModule.h"

namespace Atom::Insights
{
	/*class FRewindDebugger : public IRewindDebuggerRuntimeExtension
	{
	public:
		virtual void RecordingStarted() override;
	};*/

	class CRIWAREATOMINSIGHTS_API FTraceModule : public IAtomInsightsTraceModule
	{
	public:

		FTraceModule();
		virtual ~FTraceModule() = default;

		//~ Begin TraceServices::IModule Interface
		virtual void GetModuleInfo(TraceServices::FModuleInfo& OutModuleInfo) override;
		virtual void OnAnalysisBegin(TraceServices::IAnalysisSession& Session) override;
		virtual void GetLoggers(TArray<const TCHAR *>& OutLoggers) override;
		virtual void GenerateReports(const TraceServices::IAnalysisSession& Session, const TCHAR* CmdLine, const TCHAR* OutputDirectory) override;
		virtual const TCHAR* GetCommandLineArgument() override { return TEXT("atomtrace"); }
		//~ End TraceServices::IModule Interface

		template <typename TraceProviderType>
		TSharedPtr<TraceProviderType> FindAtomTraceProvider() const
		{
			return StaticCastSharedPtr<TraceProviderType>(TraceProviders.FindRef(TraceProviderType::GetName_Static()));
		}

		virtual void AddTraceProvider(TSharedPtr<FTraceProviderBase> TraceProvider) override;

		virtual void StartTraceAnalysis(const bool bOnlyTraceAtomChannels) override;
		virtual bool IsTraceAnalysisActive() const override;
		virtual void StopTraceAnalysis() override;
		virtual void OnOnlyTraceAtomChannelsStateChanged(const bool bOnlyTraceAtomChannels) override;

		virtual bool AtomChannelsCanBeManuallyEnabled() const override;

#if !WITH_EDITOR
		virtual void InitializeSessionInfo(const TraceServices::FSessionInfo& SessionInfo) override;
		virtual void RequestChannelUpdate() override;
		virtual void ResetTicker() override;
		virtual bool TraceControllerIsAvailable() const override;
#endif

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnAnalysisStarting, const double /*Timestamp*/);
		FOnAnalysisStarting OnAnalysisStarting;

	private:
		static const FName GetName();

		static void DisableAllTraceChannels();
		bool EnableAtomInsightsTraceChannels();

		void DisableAtomInsightsTraceChannels() const;

		void CacheCurrentlyEnabledTraceChannels();
		void RestoreCachedChannels() const;

#if !WITH_EDITOR
		const UE::Trace::FStoreClient::FSessionInfo* GetCurrentAnalysisSessionInfo() const;
		bool GetAtomTracesAreEnabled() const;
		void SendDiscoveryRequestToTraceController() const;

		bool Tick(float DeltaTime);
#endif

		TMap<FName, TSharedPtr<FTraceProviderBase>> TraceProviders;
		TArray<FString> ChannelsToRestore;
		//FRewindDebugger RewindDebugger;

		bool bTraceAnalysisHasStarted = false;
		bool bStopTracingAfterAtomInsightsIsFinished = false;

#if !WITH_EDITOR
		FGuid InstanceID;

		FTickerDelegate OnTick;
		FTSTicker::FDelegateHandle OnTickHandle;

		const TArray<FString> AtomChannels;
		const TArray<FString> EmptyArray;
#endif
	};
} // namespace
