
#include "AtomInsightsTraceModule.h"

#include "CoreGlobals.h"
#include "Insights/IInsightsManager.h"
#include "Insights/IUnrealInsightsModule.h"
#include "ISessionServicesModule.h"
#include "Misc/App.h"
#include "ProfilingDebugging/TraceAuxiliary.h"
#include "Templates/SharedPointer.h"
#include "Trace/Trace.h"
#include "TraceServices/Model/Channel.h"
#include "TraceServices/Model/Diagnostics.h"

#include "Providers/MixerSourceTraceProvider.h"
#include "Providers/VirtualLoopTraceProvider.h"

namespace Atom::Insights
{
	namespace FTraceModulePrivate
	{
		static const TCHAR* CPUChannelName = TEXT("Cpu");
		static const TCHAR* AtomChannelName = TEXT("Atom");
		static const TCHAR* AtomMixerChannelName = TEXT("AtomMixer");
	}

	/*void FRewindDebugger::RecordingStarted()
	{
		using namespace FTraceModulePrivate;

		UE::Trace::ToggleChannel(AtomChannelName, true);
		UE::Trace::ToggleChannel(AtomMixerChannelName, true);
	}*/

	FTraceModule::FTraceModule()
#if !WITH_EDITOR
		: AtomChannels({ FTraceModulePrivate::AtomChannelName, FTraceModulePrivate::AtomMixerChannelName })
		, EmptyArray()
#endif
	{

	}

	void FTraceModule::GetModuleInfo(TraceServices::FModuleInfo& OutModuleInfo)
	{
		OutModuleInfo.Name = GetName();
		OutModuleInfo.DisplayName = TEXT("Atom");
	}

	void FTraceModule::AddTraceProvider(TSharedPtr<FTraceProviderBase> TraceProvider)
	{
		TraceProviders.Add(TraceProvider->GetName(), TraceProvider);
	}

	const FName FTraceModule::GetName()
	{
		const FLazyName TraceName = { "TraceModule_AtomTrace" };
		return TraceName.Resolve();
	}

	void FTraceModule::DisableAllTraceChannels()
	{
		UE::Trace::EnumerateChannels([](const UE::Trace::FChannelInfo& ChannelInfo, void* User)
		{
			// Only disable channels that are not read only (i.e. set from the command line)
			if (!ChannelInfo.bIsReadOnly && ChannelInfo.bIsEnabled)
			{
				UE::Trace::ToggleChannel(ANSI_TO_TCHAR(ChannelInfo.Name), false);
			}
			return true;
		}
		, nullptr);
	}

	bool FTraceModule::EnableAtomInsightsTraceChannels()
	{
		using namespace FTraceModulePrivate;

#if WITH_EDITOR
		const bool bCPUOn = UE::Trace::ToggleChannel(CPUChannelName, true);
		const bool bAtomOn = UE::Trace::ToggleChannel(AtomChannelName, true);
		const bool bAtomMixerOn = UE::Trace::ToggleChannel(AtomMixerChannelName, true);

		return bCPUOn && bAtomOn && bAtomMixerOn;
#else
		if (!InstanceID.IsValid())
		{
			return false;
		}

		ISessionServicesModule& SessionServicesModule = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices");
		TSharedPtr<ITraceController> TraceController = SessionServicesModule.GetTraceController();
		if (!TraceController.IsValid())
		{
			return false;
		}

		// Requires ITraceController to have received a response to discovering the session and channel info
		if (TraceController->HasAvailableInstance(InstanceID))
		{
			TraceController->WithInstance(InstanceID, [&](const FTraceStatus& Status, ITraceControllerCommands& Commands)
			{
				if (Status.bIsTracing)
				{
					Commands.SetChannels(AtomChannels, EmptyArray);
				}
			});

			return true;
		}
		return false;
#endif // WITH_EDITOR
	}

	void FTraceModule::DisableAtomInsightsTraceChannels() const
	{
		using namespace FTraceModulePrivate;

		auto DisableIfNotMarkedToRestore = [this](const TCHAR* ChannelName)
		{
			if (!ChannelsToRestore.Contains(ChannelName))
			{
				UE::Trace::ToggleChannel(ChannelName, false);
			}
		};

		DisableIfNotMarkedToRestore(CPUChannelName);
		DisableIfNotMarkedToRestore(AtomChannelName);
		DisableIfNotMarkedToRestore(AtomMixerChannelName);
	}

	void FTraceModule::CacheCurrentlyEnabledTraceChannels()
	{	
		ChannelsToRestore.Empty();

		UE::Trace::EnumerateChannels([](const UE::Trace::FChannelInfo& ChannelInfo, void* User)
		{
			TArray<FString>* EnabledChannels = static_cast<TArray<FString>*>(User);
			if (!ChannelInfo.bIsReadOnly && ChannelInfo.bIsEnabled)
			{
				EnabledChannels->Emplace(ANSI_TO_TCHAR(ChannelInfo.Name));
			}

			return true;
		}, &ChannelsToRestore);
	}

	void FTraceModule::RestoreCachedChannels() const
	{
		for (const FString& Channel : ChannelsToRestore)
		{
			UE::Trace::ToggleChannel(Channel.GetCharArray().GetData(), true);
		}
	}

#if !WITH_EDITOR
	const UE::Trace::FStoreClient::FSessionInfo* FTraceModule::GetCurrentAnalysisSessionInfo() const
	{
		IUnrealInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<IUnrealInsightsModule>("TraceInsights");

		TSharedPtr<const TraceServices::IAnalysisSession> AnalysisSession = InsightsModule.GetAnalysisSession();
		if (!AnalysisSession.IsValid())
		{
			return nullptr;
		}

		UE::Trace::FStoreClient* StoreClient = InsightsModule.GetStoreClient();
		if (StoreClient == nullptr)
		{
			return nullptr;
		}

		return StoreClient->GetSessionInfoByTraceId(AnalysisSession->GetTraceId());
	}

	bool FTraceModule::GetAtomTracesAreEnabled() const
	{
		using namespace FTraceModulePrivate;

		const IUnrealInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<IUnrealInsightsModule>("TraceInsights");
		const TSharedPtr<const TraceServices::IAnalysisSession> AnalysisSession = InsightsModule.GetAnalysisSession();

		if (!AnalysisSession.IsValid())
		{
			return false;
		}

		const TraceServices::IChannelProvider* ChannelProvider = AnalysisSession->ReadProvider<TraceServices::IChannelProvider>("ChannelProvider");
		if (ChannelProvider == nullptr)
		{
			return false;
		}
		
		bool bAtomChannelIsEnabled = false;
		bool bAtomMixerChannelIsEnabled = false;

		const TArray<TraceServices::FChannelEntry>& Channels = ChannelProvider->GetChannels();
		for (int32 ChannelIndex = 0; ChannelIndex < Channels.Num(); ++ChannelIndex)
		{
			if (Channels[ChannelIndex].bIsEnabled)
			{
				if (!bAtomChannelIsEnabled && Channels[ChannelIndex].Name.Compare(AtomChannelName) == 0)
				{
					bAtomChannelIsEnabled = true;
				}
				else if (!bAtomMixerChannelIsEnabled && Channels[ChannelIndex].Name.Compare(AtomMixerChannelName) == 0)
				{
					bAtomMixerChannelIsEnabled = true;
				}

				if (bAtomChannelIsEnabled && bAtomMixerChannelIsEnabled)
				{
					return true;
				}
			}
		}

		return false;
	}

	void FTraceModule::SendDiscoveryRequestToTraceController() const
	{
		// We need to send discovery requests to ITraceController so we can update the active channels later
		ISessionServicesModule& SessionServicesModule = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices");

		TSharedPtr<ISessionManager> SessionManager = SessionServicesModule.GetSessionManager();
		TSharedPtr<ITraceController> TraceController = SessionServicesModule.GetTraceController();
		if (SessionManager.IsValid() && TraceController.IsValid())
		{
			const FGuid SessionId = FApp::GetSessionId();

			if (TraceController.IsValid())
			{
				TraceController->SendDiscoveryRequest(SessionId, InstanceID);
				TraceController->SendStatusUpdateRequest();
			}
		}
	}

	bool FTraceModule::Tick(float DeltaTime)
	{
		if (TraceControllerIsAvailable())
		{
			RequestChannelUpdate();
			ResetTicker();
		}
		else
		{
			SendDiscoveryRequestToTraceController();
		}

		return true;
	}
#endif // !WITH_EDITOR

	void FTraceModule::OnAnalysisBegin(TraceServices::IAnalysisSession& InSession)
	{
		for (const auto& [ProviderName, Provider] : TraceProviders)
		{
#if !WITH_EDITOR
			Provider->InitSessionCachedMessages(InSession);
#endif // !WITH_EDITOR

			InSession.AddProvider(ProviderName, Provider, Provider);
			InSession.AddAnalyzer(Provider->ConstructAnalyzer(InSession));
		}

		OnAnalysisStarting.Broadcast(FPlatformTime::Seconds());
	}

	void FTraceModule::StartTraceAnalysis(const bool bOnlyTraceAtomChannels)
	{
		if (bTraceAnalysisHasStarted)
		{
			return;
		}

#if WITH_EDITOR
		// We don't want to stop any traces that were started prior to running Atom Insights
		// Check the original state here
		bStopTracingAfterAtomInsightsIsFinished = !FTraceAuxiliary::IsConnected();

		CacheCurrentlyEnabledTraceChannels();

		if (bOnlyTraceAtomChannels)
		{
			DisableAllTraceChannels();
		}

		bTraceAnalysisHasStarted = EnableAtomInsightsTraceChannels();

		if (!FTraceAuxiliary::IsConnected())
		{
			// Clear all buffered data and prevent data from previous recordings from leaking into the new recording
			FTraceAuxiliary::FOptions Options;
			Options.bExcludeTail = true;

			FTraceAuxiliary::Start(FTraceAuxiliary::EConnectionType::Network, TEXT("localhost"), TEXT(""), &Options);
		}

		// Even if a trace was already active, we still need to notify Atom Insights that analysis has begun
		IUnrealInsightsModule& InsightsModule = FModuleManager::GetModuleChecked<IUnrealInsightsModule>("TraceInsights");
		InsightsModule.StartAnalysisForLastLiveSession();
#else
		bTraceAnalysisHasStarted = EnableAtomInsightsTraceChannels();
#endif // WITH_EDITOR
	}

	void FTraceModule::StopTraceAnalysis()
	{
		if (FTraceAuxiliary::IsConnected())
		{
			DisableAtomInsightsTraceChannels();

			if (bStopTracingAfterAtomInsightsIsFinished)
			{
				FTraceAuxiliary::Stop();
			}

			RestoreCachedChannels();
		}

		ChannelsToRestore.Empty();
		bTraceAnalysisHasStarted = false;
	}

	void FTraceModule::OnOnlyTraceAtomChannelsStateChanged(const bool bOnlyTraceAtomChannels)
	{
		if (!bTraceAnalysisHasStarted)
		{
			return;
		}

		if (bOnlyTraceAtomChannels)
		{
			// Re-cache the current settings for enabled channels. This may have changed since Atom Insights began.
			CacheCurrentlyEnabledTraceChannels();
			DisableAllTraceChannels();
			EnableAtomInsightsTraceChannels();
		}
		else
		{
			RestoreCachedChannels();
		}
	}

	bool FTraceModule::IsTraceAnalysisActive() const
	{
		return FTraceAuxiliary::IsConnected() && bTraceAnalysisHasStarted;
	}

	bool FTraceModule::AtomChannelsCanBeManuallyEnabled() const
	{
#if WITH_EDITOR
		return false;
#else
		// When attaching to a packaged build, we need to check whether we're connected and the Atom channels are enabled
		return !bTraceAnalysisHasStarted && GetCurrentAnalysisSessionInfo() != nullptr && !GetAtomTracesAreEnabled();
#endif // WITH_EDITOR
	}

	#if !WITH_EDITOR
	void FTraceModule::InitializeSessionInfo(const TraceServices::FSessionInfo& SessionInfo)
	{
		InstanceID = SessionInfo.InstanceId;

		OnTick = FTickerDelegate::CreateRaw(this, &FTraceModule::Tick);

		constexpr float TickDelay = 0.5f; // 500 ms. delay between ticks
		OnTickHandle = FTSTicker::GetCoreTicker().AddTicker(OnTick, TickDelay);
	}

	void FTraceModule::RequestChannelUpdate()
	{
		// Request for ITraceController to get the latest active channels list
		ISessionServicesModule& SessionServicesModule = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices");
		TSharedPtr<ITraceController> TraceController = SessionServicesModule.GetTraceController();
		if (!TraceController.IsValid())
		{
			return;
		}

		TraceController->SendChannelUpdateRequest();
	}

	void FTraceModule::ResetTicker()
	{
		if (OnTickHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(OnTickHandle);
		}
	}

	bool FTraceModule::TraceControllerIsAvailable() const
	{
		if (!InstanceID.IsValid())
		{
			return false;
		}

		ISessionServicesModule& SessionServicesModule = FModuleManager::LoadModuleChecked<ISessionServicesModule>("SessionServices");
		TSharedPtr<ITraceController> TraceController = SessionServicesModule.GetTraceController();
		if (!TraceController.IsValid())
		{
			return false;
		}

		return TraceController->HasAvailableInstance(InstanceID);
	}
#endif // !WITH_EDITOR

	void FTraceModule::GetLoggers(TArray<const TCHAR *>& OutLoggers)
	{
		OutLoggers.Add(TEXT("Atom"));
	}

	void FTraceModule::GenerateReports(const TraceServices::IAnalysisSession& Session, const TCHAR* CmdLine, const TCHAR* OutputDirectory)
	{

	}
} // namespace
