
#include "Atom/Profiling/AtomMonitor.h"

#include "Async/Async.h"
#include "Misc/OutputDeviceArchiveWrapper.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "CriWareCoreSettings.h"

#ifndef ENABLE_ATOM_MONITOR
#error "Please define ENABLE_ATOM_MONITOR"
#endif //ENABLE_ATOM_DEBUG

#if ENABLE_ATOM_MONITOR

namespace FAtomMonitor_NativeCallbacks
{
	void CRIAPI OnMonitorLog(void* Obj, const CriChar8* LogString, CriUint32 LogMode)
	{
		if (FAtomMonitor* Self = static_cast<FAtomMonitor*>(Obj))
		{
			Self->HandleNativeOnMonitorLog(UTF8_TO_TCHAR(LogString), (uint32)LogMode);
		}
	}
}

// Atom Monitor Delegates Implementation
FAtomMonitorDelegates::FOnAtomMonitorEvent FAtomMonitorDelegates::OnAtomMonitorStarted;
FAtomMonitorDelegates::FOnAtomMonitorEvent FAtomMonitorDelegates::OnAtomMonitorStopped;
FAtomMonitorDelegates::FOnAtomMonitorConnectionEvent FAtomMonitorDelegates::OnAtomMonitorConnected;
FAtomMonitorDelegates::FOnAtomMonitorConnectionEvent FAtomMonitorDelegates::OnAtomMonitorDisconnected;
FAtomMonitorDelegates::FOnAtomMonitorLog FAtomMonitorDelegates::OnAtomMonitorLog;

namespace Atom
{
	static FAutoConsoleCommand GAtomMonitorStart(
		TEXT("atom.Monitor.Start"),
		TEXT("Start Atom monitoring and listening for In-game Preview and profiling connections. Optional Argument: Port=<host TCP port number>"),
		FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				FString Cmd = FString::Join(Args, TEXT(" "));
				uint16 Port;
				if (FParse::Value(*Cmd, TEXT("Port="), Port))
				{
					RuntimeManager->GetAtomMonitor().Start(Port);
				}
				else
				{
					RuntimeManager->GetAtomMonitor().Start();
				}
			}
		}), ECVF_Cheat);

	static FAutoConsoleCommand GAtomMonitorStop(
		TEXT("atom.Monitor.Stop"),
		TEXT("Stop Atom monitoring and listening for In-game Preview connection."),
		FConsoleCommandDelegate::CreateStatic([]()
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				RuntimeManager->GetAtomMonitor().Stop();
			}
		}), ECVF_Cheat);

	static FAutoConsoleCommandWithOutputDevice GAtomMonitorLogStatus(
		TEXT("atom.Monitor.LogStatus"),
		TEXT("Log the current status and connections of the Atom Monitor."),
		FConsoleCommandWithOutputDeviceDelegate::CreateStatic([](FOutputDevice& Ar)
		{
			if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
			{
				bool bIsStarted = RuntimeManager->GetAtomMonitor().IsStarted();

				if (bIsStarted)
				{
					auto Info = RuntimeManager->GetAtomMonitor().GetConnectionInfo();
					Ar.Logf(TEXT("Atom Monitor started on %s:%d"), *Info.HostAddr.ToString(), Info.HostTCPPort);

					bool bIsConnected = RuntimeManager->GetAtomMonitor().IsClientConnected();
					if (bIsConnected)
					{
						Ar.Logf(TEXT(" - Connected to In-Game Preview %s"), *Info.ClientAddr.ToString());
					}
					else
					{
						Ar.Logf(TEXT(" - Listening for client..."));
					}
				}
				else
				{
					Ar.Logf(TEXT("Atom Monitor not started."));
				}
			}
		}), ECVF_Cheat);

	static int32 AtomMonitorEnableOutputLogCVar = 0;
	FAutoConsoleVariableRef CVarAtomMonitorEnableOutputLog(
		TEXT("atom.Monitor.EnableOutputLog"),
		AtomMonitorEnableOutputLogCVar,
		TEXT("Visualization mode for Atom active sounds. \n")
		TEXT("0: Not Enabled, 1: Enabled"),
		ECVF_Default);

}// namespace

// This occur on atom monitor thread
void FAtomMonitor::HandleNativeOnMonitorLog(const TCHAR* LogMessage, uint32 InLogMode)
{
	// execute on next game tick
	Async(EAsyncExecution::TaskGraphMainThread, [WeakThis = AsWeak(), Msg = FString(LogMessage), InLogMode]()
	{
		auto StrongThis = WeakThis.Pin();
		if (StrongThis.IsValid() && (StrongThis->bLogToUELog || Atom::AtomMonitorEnableOutputLogCVar))
		{
			auto& Info = StrongThis->GetConnectionInfo();
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom Monitor - %s - %s"), ToString((EAtomMonitorLogMode)InLogMode), *Msg);
		}

		FAtomMonitorDelegates::OnAtomMonitorLog.Broadcast(Msg, (EAtomMonitorLogMode)InLogMode);
	});
}

FAtomMonitor::FAtomMonitor()
{
}

FAtomMonitor::~FAtomMonitor()
{
	Stop();
}

bool FAtomMonitor::Start(const uint16 InListenPort)
{
	if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
	{
		FAtomRuntimeHandle Runtime = RuntimeManager->GetActiveAtomRuntime();
		if (!Runtime.IsValid())
		{
			return false;
		}

		RuntimeID = Runtime.GetRuntimeID();
		FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddSP(this, &FAtomMonitor::OnRuntimeDestroyed);

			auto Settings = GetDefault<UCriWareCoreSettings>();

		CriAtomExMonitorConfig MonitorConfig;
		criAtomExMonitor_SetDefaultConfig(&MonitorConfig);
		MonitorConfig.communication_buffer_size = Settings->MonitorCommunicationBufferSize;
		//MonitorConfig.listen_port = (CriUint16)InListenPort;
		//MonitorConfig.max_preivew_object = Settings->MonitorMaxPreviewObject;
		//MonitorConfig.playback_position_update_interval = Settings->MonitorPlaybackpPositionUpdateInterval;
		FCriWareApi::criAtomExMonitor_Initialize(&MonitorConfig, nullptr, 0);

		const CriChar8* ServerIpStr = FCriWareApi::criAtomExMonitor_GetServerIpString();
		bIsStarted = FIPv4Address::Parse(UTF8_TO_TCHAR(ServerIpStr), ConnectionInfo.HostAddr);

		ConnectionInfo.HostTCPPort = InListenPort;

		if (bIsStarted)
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Started Atom Monitor service on host address %s on port %d."), *ConnectionInfo.HostAddr.ToString(), ConnectionInfo.HostTCPPort);

			//if (Settings->MonitorEnableLog)
			//{
			FCriWareApi::criAtomExMonitor_SetLogCallbackWithLogMode(&FAtomMonitor_NativeCallbacks::OnMonitorLog, this);
			//}

			//if (Settings->MonitorEnableLog)
			//{
			// FCriWareApi::criAtomExMonitor_SetDataUpdateNotificationCallback(&FAtomMonitor_NativeCallbacks::OnMonitorDataUpdateNofitication, this);
			//}

			ExecuteOnGameThread([InAddr = ConnectionInfo.HostAddr, InPort = ConnectionInfo.HostTCPPort]
			{
				FAtomMonitorDelegates::OnAtomMonitorStarted.Broadcast(InAddr.ToString(), InPort);
			});

			SetLogMode(LogMode, bLogToUELog);
		}

		return bIsStarted;
	}

	return false;
}

void FAtomMonitor::Stop()
{
	if (bIsStarted)
	{
		if (bIsConnected)
		{
			// connection ended
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom Monitor - Client Disconnected from %s."), *ConnectionInfo.ClientAddr.ToString());

			ExecuteOnGameThread([InAddr = ConnectionInfo.ClientAddr]
			{
				FAtomMonitorDelegates::OnAtomMonitorDisconnected.Broadcast(InAddr.ToString());
			});

			ConnectionInfo.ClientAddr = FIPv4Address::Any;
		}

		FCriWareApi::criAtomExMonitor_Finalize();

		UE_LOG(LogCriWareAtom, Display, TEXT("Stopped Atom Monitor service."), *ConnectionInfo.HostAddr.ToString(), ConnectionInfo.HostTCPPort);

		ExecuteOnGameThread([InAddr = ConnectionInfo.HostAddr, InPort = ConnectionInfo.HostTCPPort]
		{
			FAtomMonitorDelegates::OnAtomMonitorStopped.Broadcast(InAddr.ToString(), InPort);
		});
	}

	bIsStarted = false;
	bIsConnected = false;
	RuntimeID = INDEX_NONE;
}

bool FAtomMonitor::SetLogMode(EAtomMonitorLogMode InLogMode, bool InbLogToUELog)
{
	bLogToUELog = InbLogToUELog;
	LogMode = InLogMode;

	if (bIsStarted)
	{
		CriUint32 MonitorLogMode = LogMode == EAtomMonitorLogMode::All ? CRIATOMEX_MONITOR_LOG_MODE_ALL : (CriUint32)LogMode;
		FCriWareApi::criAtomExMonitor_SetLogMode(MonitorLogMode);
		return true;
	}

	return false;
}

void FAtomMonitor::SendUserLogMessage(const FString& LogMessage)
{
	if (bIsStarted)
	{
		FCriWareApi::criAtomExMonitor_OutputUserLog(TCHAR_TO_UTF8(*LogMessage));
	}
}

void FAtomMonitor::UpdateConnectionStatus()
{
	if (bIsStarted)
	{
		bool bConnected = FCriWareApi::criAtomExMonitor_IsConnected() == CRI_TRUE;
		if (bConnected && !bIsConnected)
		{
			// new connection
			const CriChar8* ClientIpStr = FCriWareApi::criAtomExMonitor_GetClientIpString();
			if (FIPv4Address::Parse(UTF8_TO_TCHAR(ClientIpStr), ConnectionInfo.ClientAddr))
			{
				bIsConnected = bConnected;

				UE_LOG(LogCriWareAtom, Display, TEXT("Atom Monitor - Client Connected from %s."), *ConnectionInfo.ClientAddr.ToString());

				ExecuteOnGameThread([InAddr = ConnectionInfo.ClientAddr]
				{
					FAtomMonitorDelegates::OnAtomMonitorConnected.Broadcast(InAddr.ToString());
				});
			}
		}
		else if (!bConnected && bIsConnected)
		{
			bIsConnected = bConnected;

			// connection ended
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom Monitor - Client Disconnected from %s."), *ConnectionInfo.ClientAddr.ToString());

			ExecuteOnGameThread([InAddr = ConnectionInfo.ClientAddr]
			{
				FAtomMonitorDelegates::OnAtomMonitorDisconnected.Broadcast(InAddr.ToString());
			});

			ConnectionInfo.ClientAddr = FIPv4Address::Any;
		}
	}
}

void FAtomMonitor::OnRuntimeDestroyed(FAtomRuntimeId InRuntimeID)
{
	if (InRuntimeID != INDEX_NONE && InRuntimeID == RuntimeID)
	{
		// in case not stopped by the runtime.
		Stop();
	}

	FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.RemoveAll(this);
}

FAtomMonitor& FAtomMonitor::Get()
{ 
	if (FAtomRuntimeManager* Manager = FAtomRuntimeManager::Get())
	{
		return Manager->GetAtomMonitor();
	}

	static auto Empty = FAtomMonitor();

	return Empty; // not invalid but without runtime impossible to start-it.
}

#endif // ENABLE_ATOM_MONITOR
