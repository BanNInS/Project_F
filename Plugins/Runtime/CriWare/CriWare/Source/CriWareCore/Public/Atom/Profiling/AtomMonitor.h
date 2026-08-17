#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumClassFlags.h"
#include "HAL/CriticalSection.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Async/Async.h"

#include "CriWareDefines.h"
#include "Atom/Atom.h"

#if ENABLE_ATOM_MONITOR

/** Describe the connection in Atom Monitor. */
struct FAtomMonitorConnectionInfo
{
public:

	/** The TCP port to listen. */
	uint16 HostTCPPort = ATOM_MONITOR_DEFAULT_PORT;

	/** The monitor server address. */
	FIPv4Address HostAddr;

	/** The connected client address when connected. */
	FIPv4Address ClientAddr;
};

enum class EAtomMonitorLogMode : uint16
{
	Off					= 0x0000, // Not set
	
	Playback			= 0x0001,
	Error				= 0x0002,
	LowLevelPlayback	= 0x0004,
	SystemInfo			= 0x0008,
	HandleInfo			= 0x0010,
	CueLimit			= 0x0020,
	Probability			= 0x0040,
	Category			= 0x0080,
	ExecutingInfo		= 0x0100,
	Position3DInfo		= 0x0200, // 3DInfo
	User				= 0x0400,
	VoiceVolume			= 0x0800,

	All					= 0xFFFF
};
ENUM_CLASS_FLAGS(EAtomMonitorLogMode);

inline const TCHAR* ToString(EAtomMonitorLogMode LogMode)
{
	// todo : bitwise check ?
	switch (LogMode) {
	case EAtomMonitorLogMode::Off:				return TEXT("Off");
	case EAtomMonitorLogMode::Playback:			return TEXT("Playback");
	case EAtomMonitorLogMode::Error:			return TEXT("Error");
	case EAtomMonitorLogMode::LowLevelPlayback:	return TEXT("LowLevelPlayback");
	case EAtomMonitorLogMode::SystemInfo:		return TEXT("SystemInfo");
	case EAtomMonitorLogMode::HandleInfo:		return TEXT("HandleInfo");
	case EAtomMonitorLogMode::CueLimit:			return TEXT("CueLimit");
	case EAtomMonitorLogMode::Probability:		return TEXT("Probability");
	case EAtomMonitorLogMode::Category:			return TEXT("Category");
	case EAtomMonitorLogMode::ExecutingInfo:	return TEXT("ExecutingInfo");
	case EAtomMonitorLogMode::Position3DInfo:	return TEXT("Position3DInfo");
	case EAtomMonitorLogMode::User:				return TEXT("User");
	case EAtomMonitorLogMode::VoiceVolume:		return TEXT("VoiceVolume");
	case EAtomMonitorLogMode::All:				return TEXT("All");
	default:									return TEXT("Unknown");
	}
}

// List of delegates for the Atom Monitor server.
class CRIWARECORE_API FAtomMonitorDelegates
{
public:
	
	// This delegate is called when the Atom Monitor server is started or stopped.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAtomMonitorEvent, const FString& /* HostIP */, uint16 /* HostPost */);

	// This delegate is called when the client is connected or disconnected to Atom Monitor.
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAtomMonitorConnectionEvent, const FString& /* ClientIP */);

	// This delegate is called when a log message is emitted by Atom Monitor.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAtomMonitorLog, const FString& /* LogMessage */, EAtomMonitorLogMode /*LogMode*/);
	
	//
	static FOnAtomMonitorEvent OnAtomMonitorStarted;

	//
	static FOnAtomMonitorEvent OnAtomMonitorStopped;

	// This delegate is called when the client is connected.
	static FOnAtomMonitorConnectionEvent OnAtomMonitorConnected;

	// This delegate is called when the client is disconnected.
	static FOnAtomMonitorConnectionEvent OnAtomMonitorDisconnected;

	// 
	static FOnAtomMonitorLog OnAtomMonitorLog;
};

class CRIWARECORE_API FAtomMonitor
	: public TSharedFromThis<FAtomMonitor>
{
public:

	FAtomMonitor();
	~FAtomMonitor();

	/** Starts Atom Monitor monitoring and server listening for a client on given port.  */
	bool Start(const uint16 InListenPort = ATOM_MONITOR_DEFAULT_PORT);

	/** Closes connection if a client is connected and stops Atom Monitor. */
	void Stop();

	/** */
	void UpdateConnectionStatus();
	
	/** */
	FORCEINLINE bool IsStarted() const { return bIsStarted; }

	/** */
	FORCEINLINE bool IsListening() const { return !bIsConnected; }

	/** */
	FORCEINLINE bool IsClientConnected() const { return bIsConnected; }

	/** */
	FORCEINLINE const FAtomMonitorConnectionInfo& GetConnectionInfo() const { return ConnectionInfo; }

	/** */
	FORCEINLINE EAtomMonitorLogMode GetLogMode() const { return LogMode; }

	/** */
	bool SetLogMode(EAtomMonitorLogMode InLogMode, bool InbLogToUELog = true);

	/** */
	void SendUserLogMessage(const FString& LogMessage);

private:

	void OnRuntimeDestroyed(FAtomRuntimeId InRuntimeID);

	/** Calls a callable on the game thread. */
	template <typename CallableType>
	void ExecuteOnGameThread(CallableType&& Callable)
	{
		if (IsInGameThread())
		{
			Callable();
		}
		else
		{
			Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(Callable));
		}
	}

	FAtomMonitorConnectionInfo ConnectionInfo;
	EAtomMonitorLogMode LogMode = EAtomMonitorLogMode::All;
	bool bIsStarted = false;
	bool bIsConnected = false;
	bool bLogToUELog = false;
	FAtomRuntimeId RuntimeID = INDEX_NONE;

public:

	void HandleNativeOnMonitorLog(const TCHAR* LogMessage, uint32 LogMode);

	static FAtomMonitor& Get();
};

#endif // ENABLE_ATOM_MONITOR
