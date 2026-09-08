
#include "Atom/Mixer/Quartz/AtomQuartzSubsystem.h"

#include "Engine/World.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Stats/Stats.h"

#include "Atom/Mixer/Quartz/AtomMixerClockHandle.h"
#include "Atom/Mixer/Quartz/AtomMixerClockManager.h"
#include "Atom/Mixer/Quartz/AtomMixerQuantizedCommands.h"
#include "Atom/Mixer/Quartz/AtomQuartzMetronome.h"
#include "Atom/AtomQuartzQuantizationUtilities.h"

#include "Atom/AtomRuntime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomQuartzSubsystem)

static int32 MaxAtomQuartzSubscribersToUpdatePerTickCvar = -1;
FAutoConsoleVariableRef CVarMaxAtomQuartzSubscribersToUpdatePerTick(
	TEXT("atom.Quartz.MaxSubscribersToUpdatePerTick"),
	MaxAtomQuartzSubscribersToUpdatePerTickCvar,
	TEXT("Limits the number of Quartz subscribers to update per Tick.\n")
	TEXT("<= 0: No Limit, >= 1: Limit"),
	ECVF_Default);

static int32 SimulateNoAtomRuntimeCvar = 0;
FAutoConsoleVariableRef CVarSimulateNoAtomRuntime(
	TEXT("atom.Quartz.SimulateNoAtomRuntime"),
	SimulateNoAtomRuntimeCvar,
	TEXT("If enabled, the QuartzSubsystem will assume no Atom runtime, and will run new clocks in headless mode.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static FAtomRuntime* GetAtomRuntime(const UWorld* InWorld)
{
	if (!InWorld || SimulateNoAtomRuntimeCvar)
	{
		return nullptr;
	}

	if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
	{
		return AtomRuntimeManager->GetAtomRuntimeRawFromWorld(InWorld);;
	}

	return nullptr;
}

static TUniquePtr<FScopeLock> GetPersistentStateScopeLock(const UWorld* InWorld)
{
	if (FAtomRuntime* AtomRuntimePtr = GetAtomRuntime(InWorld))
	{
		return MakeUnique<FScopeLock>(&AtomRuntimePtr->QuartzPersistentStateCritSec);
	}

	return {};
}

void UAtomQuartzSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));

	if (const UWorld* WorldPtr = GetWorld())
	{
		if (FAtomRuntime* AtomRuntime = GetAtomRuntime(WorldPtr))
		{
			ClockManagerDataPtr = AtomRuntime->QuartzSubsystemData;
		}
	}

	if (!ClockManagerDataPtr)
	{
		ClockManagerDataPtr = MakeShared<Atom::FPersistentQuartzSubsystemData>();
	}
}

void UAtomQuartzSubsystem::Deinitialize()
{
	Super::Deinitialize();
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));

	if (FAtomRuntime* AtomRuntime = GetAtomRuntime(GetWorld()))
	{
		AtomRuntime->QuartzSubsystemData = ClockManagerDataPtr;
	}
}

void UAtomQuartzSubsystem::BeginDestroy()
{
	Super::BeginDestroy();

	// force un-subscribe all Quartz tickable objects
	if (ClockManagerDataPtr)
	{
		ClockManagerDataPtr->SubsystemClockManager.Flush();
	}
}

void FAtomQuartzTickableObjectsManager::Tick(float DeltaTime)
{
	const int32 NumSubscribers = QuartzTickSubscribers.Num();
	if (MaxAtomQuartzSubscribersToUpdatePerTickCvar <= 0 || NumSubscribers <= MaxAtomQuartzSubscribersToUpdatePerTickCvar)
	{
		TArray<FQuartzTickableObject*> SubscribersCopy = QuartzTickSubscribers;

		// we can afford to update ALL subscribers
		for (FQuartzTickableObject* Entry : SubscribersCopy)
		{
			if (Entry && Entry->QuartzIsTickable())
			{
				Entry->QuartzTick(DeltaTime);
			}
		}

		UpdateIndex = 0;
	}
	else
	{
		// only update up to our limit
		for (int i = 0; i < MaxAtomQuartzSubscribersToUpdatePerTickCvar; ++i)
		{
			FQuartzTickableObject* CurrentSubscriber = QuartzTickSubscribers[UpdateIndex];
			if (!ensure(CurrentSubscriber))
			{
				continue;
			}

			if (CurrentSubscriber->QuartzIsTickable())
			{
				CurrentSubscriber->QuartzTick(DeltaTime);
			}

			if (++UpdateIndex == NumSubscribers)
			{
				UpdateIndex = 0;
			}
		}
	}
}

bool FAtomQuartzTickableObjectsManager::IsTickable() const
{
	const int32 NumSubscribers = QuartzTickSubscribers.Num();
	const bool bHasTickSubscribers = NumSubscribers > 0;
	TRACE_INT_VALUE(TEXT("QuartzSubsystem::NumSubscribers"), NumSubscribers);

	// if our manager has no clocks, and we have no ClockHandle subscribers, we don't need to tick
	if (!bHasTickSubscribers)
	{
		return false;
	}

	// if our manager has no clocks, and none of our subscribers are tickable, we don't need to tick
	for (const FQuartzTickableObject * Entry : QuartzTickSubscribers)
	{
		if (Entry && Entry->QuartzIsTickable())
		{
			return true;
		}
	}

	return false;
}

void FAtomQuartzTickableObjectsManager::SubscribeToQuartzTick(FQuartzTickableObject* InObjectToTick)
{
	if (!InObjectToTick)
	{
		return;
	}

	QuartzTickSubscribers.AddUnique(InObjectToTick);
}

void FAtomQuartzTickableObjectsManager::UnsubscribeFromQuartzTick(FQuartzTickableObject* InObjectToTick)
{
	if (!InObjectToTick)
	{
		return;
	}

	QuartzTickSubscribers.RemoveSingleSwap(InObjectToTick);
}


bool UAtomQuartzSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return Super::DoesSupportWorldType(WorldType) || WorldType == EWorldType::EditorPreview;
}


void UAtomQuartzSubsystem::Tick(float DeltaTime)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);
	Super::Tick(DeltaTime);
	TRACE_CPUPROFILER_EVENT_SCOPE(AtomQuartzSubsystem::Tick);

	check(TickableObjectManagerPtr);
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));

	PruneStaleProxies();
	ClockManagerDataPtr->SubsystemClockManager.LowResoultionUpdate(DeltaTime);
	TickableObjectManagerPtr->Tick(DeltaTime);
}

bool UAtomQuartzSubsystem::IsTickableWhenPaused() const
{
	return bTickEvenWhenPaused;
}

bool UAtomQuartzSubsystem::IsTickable() const
{
	check(TickableObjectManagerPtr);
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));

	const int32 NumClocks = ClockManagerDataPtr->SubsystemClockManager.GetNumClocks();
	TRACE_INT_VALUE(TEXT("QuartzSubsystem::NumClocks"), NumClocks);

	// IsTickable() updates unreal insights values
	const bool bSubscribersNeedUpdate = TickableObjectManagerPtr->IsTickable();
	const bool bIsManagingClocks = NumClocks > 0;

	return bIsManagingClocks || bSubscribersNeedUpdate;
}

TStatId UAtomQuartzSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAtomQuartzSubsystem, STATGROUP_Tickables);
}

 void UAtomQuartzSubsystem::SubscribeToQuartzTick(FQuartzTickableObject * InObjectToTick)
 {
	check(TickableObjectManagerPtr.IsValid());
 	TickableObjectManagerPtr->SubscribeToQuartzTick(InObjectToTick);
 }


 void UAtomQuartzSubsystem::UnsubscribeFromQuartzTick(FQuartzTickableObject * InObjectToTick)
 {
	check(TickableObjectManagerPtr.IsValid());
 	TickableObjectManagerPtr->UnsubscribeFromQuartzTick(InObjectToTick);
 }


UAtomQuartzSubsystem* UAtomQuartzSubsystem::Get(const UWorld* const World)
{
	if (World)
	{
		return World->GetSubsystem<UAtomQuartzSubsystem>();
	}

	return nullptr;
}


Atom::FQuartzQuantizedRequestData UAtomQuartzSubsystem::CreateRequestDataForSchedulePlaySound(UAtomQuartzClockHandle* InClockHandle, const FOnQuartzCommandEventBP& InDelegate, const FQuartzQuantizationBoundary& InQuantizationBoundary)
{
	Atom::FQuartzQuantizedRequestData CommandInitInfo;

	if (!InClockHandle)
	{
		return {};
	}

	CommandInitInfo.ClockName = InClockHandle->GetClockName();
	CommandInitInfo.QuantizationBoundary = InQuantizationBoundary;
	CommandInitInfo.QuantizedCommandPtr = MakeShared<Atom::FQuantizedPlayCommand>();
	CommandInitInfo.GameThreadSubscribers.Append(InQuantizationBoundary.GameThreadSubscribers);
	CommandInitInfo.GameThreadSubscribers.Add(InClockHandle->GetQuartzSubscriber());

	if (InDelegate.IsBound())
	{
		CommandInitInfo.GameThreadDelegateID = InClockHandle->AddCommandDelegate(InDelegate);
	}

	return CommandInitInfo;
}

bool UAtomQuartzSubsystem::IsQuartzEnabled()
{
	return true;
}


Atom::FQuartzQuantizedRequestData UAtomQuartzSubsystem::CreateRequestDataForTickRateChange(UAtomQuartzClockHandle* InClockHandle, const FOnQuartzCommandEventBP& InDelegate, const Audio::FQuartzClockTickRate& InNewTickRate, const FQuartzQuantizationBoundary& InQuantizationBoundary)
{
	if (!ensure(InClockHandle))
	{
		return { };
	}

	const TSharedPtr<Atom::FQuantizedTickRateChange> TickRateChangeCommandPtr = MakeShared<Atom::FQuantizedTickRateChange>();
	TickRateChangeCommandPtr->SetTickRate(InNewTickRate);

	Atom::FQuartzQuantizedRequestData CommandInitInfo;

	CommandInitInfo.ClockName = InClockHandle->GetClockName();
	CommandInitInfo.QuantizationBoundary = InQuantizationBoundary;
	CommandInitInfo.QuantizedCommandPtr = TickRateChangeCommandPtr;
	CommandInitInfo.GameThreadSubscribers.Append(InQuantizationBoundary.GameThreadSubscribers);
	CommandInitInfo.GameThreadSubscribers.Add(InClockHandle->GetQuartzSubscriber());

	if (InDelegate.IsBound())
	{
		CommandInitInfo.GameThreadDelegateID = InClockHandle->AddCommandDelegate(InDelegate);
	}

	return CommandInitInfo;
}

Atom::FQuartzQuantizedRequestData UAtomQuartzSubsystem::CreateRequestDataForTransportReset(UAtomQuartzClockHandle* InClockHandle, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate)
{
	if (!ensure(InClockHandle))
	{
		return { };
	}

	const TSharedPtr<Atom::FQuantizedTransportReset> TransportResetCommandPtr = MakeShared<Atom::FQuantizedTransportReset>();

	Atom::FQuartzQuantizedRequestData CommandInitInfo;

	CommandInitInfo.ClockName = InClockHandle->GetClockName();
	CommandInitInfo.QuantizationBoundary = InQuantizationBoundary;
	CommandInitInfo.QuantizedCommandPtr = TransportResetCommandPtr;
	CommandInitInfo.GameThreadSubscribers.Append(InQuantizationBoundary.GameThreadSubscribers);
	CommandInitInfo.GameThreadSubscribers.Add(InClockHandle->GetQuartzSubscriber());

	if (InDelegate.IsBound())
	{
		CommandInitInfo.GameThreadDelegateID = InClockHandle->AddCommandDelegate(InDelegate);
	}

	return CommandInitInfo;
}

Atom::FQuartzQuantizedRequestData UAtomQuartzSubsystem::CreateRequestDataForStartOtherClock(UAtomQuartzClockHandle* InClockHandle, FName InClockToStart, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate)
{
	if (!ensure(InClockHandle))
	{
		return { };
	}

	const TSharedPtr<Atom::FQuantizedOtherClockStart> TransportResetCommandPtr = MakeShared<Atom::FQuantizedOtherClockStart>();

	Atom::FQuartzQuantizedRequestData CommandInitInfo;

	CommandInitInfo.ClockName = InClockHandle->GetClockName();
	CommandInitInfo.OtherClockName = InClockToStart;
	CommandInitInfo.QuantizationBoundary = InQuantizationBoundary;
	CommandInitInfo.QuantizedCommandPtr = TransportResetCommandPtr;
	CommandInitInfo.GameThreadSubscribers.Append(InQuantizationBoundary.GameThreadSubscribers);
	CommandInitInfo.GameThreadSubscribers.Add(InClockHandle->GetQuartzSubscriber());

	if (InDelegate.IsBound())
	{
		CommandInitInfo.GameThreadDelegateID = InClockHandle->AddCommandDelegate(InDelegate);
	}

	return CommandInitInfo;
}

Atom::FQuartzQuantizedRequestData UAtomQuartzSubsystem::CreateRequestDataForQuantizedNotify(UAtomQuartzClockHandle* InClockHandle, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, float InMsOffset)
{
	if (!ensure(InClockHandle))
	{
		return { };
	}

	const TSharedPtr<Atom::FQuantizedNotify> NotifyCommandPtr = MakeShared<Atom::FQuantizedNotify>(InMsOffset);

	Atom::FQuartzQuantizedRequestData CommandInitInfo;

	CommandInitInfo.ClockName = InClockHandle->GetClockName();
	CommandInitInfo.QuantizationBoundary = InQuantizationBoundary;
	CommandInitInfo.QuantizedCommandPtr = NotifyCommandPtr;
	CommandInitInfo.GameThreadSubscribers.Append(InQuantizationBoundary.GameThreadSubscribers);
	CommandInitInfo.GameThreadSubscribers.Add(InClockHandle->GetQuartzSubscriber());

	if (InDelegate.IsBound())
	{
		CommandInitInfo.GameThreadDelegateID = InClockHandle->AddCommandDelegate(InDelegate);
	}

	return CommandInitInfo;
}

Atom::FQuartzClockManager* UAtomQuartzSubsystem::GetClockManager(const UObject* WorldContextObject, bool bUseAtomAudioEngineClockManager)
{
	// decide if the clock should be managed by the AtomRuntime (audio engine) or the Subsystem (this object)
	Atom::FQuartzClockManager* ClockManager;
	FAtomRuntime* AtomRuntime = GetAtomRuntime(WorldContextObject->GetWorld());
	if (!bUseAtomAudioEngineClockManager || !AtomRuntime)
	{
		check(ClockManagerDataPtr);
		TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));
		ClockManager = &ClockManagerDataPtr->SubsystemClockManager;
	}
	else
	{
		ClockManager = &AtomRuntime->QuantizedEventClockManager;
	}

	// we should have fallen back to this object
	return ClockManager;
}

UAtomQuartzClockHandle* UAtomQuartzSubsystem::CreateNewClock(const UObject* WorldContextObject, FName ClockName, FQuartzClockSettings InSettings, bool bOverrideSettingsIfClockExists, bool bUseAtomAudioEngineClockManager)
{
	if (ClockName.IsNone() || !WorldContextObject)
	{
		return nullptr;
	}

	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject, bUseAtomAudioEngineClockManager);
	check(ClockManager); // should have at least fallen back to "this" object as a manager

	// numerator of time signature must be >= 1
	if (InSettings.TimeSignature.NumBeats < 1)
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Clock: (%s) is attempting to set a time signature with a Numerator < 1.  Clamping to 1 beat per bar"), *ClockName.ToString());
		InSettings.TimeSignature.NumBeats = 1;
	}

	Atom::FQuartzClockProxy ClockProxy = ClockManager->GetOrCreateClock(ClockName, InSettings, bOverrideSettingsIfClockExists);
	UAtomQuartzClockHandle* ClockHandle = static_cast<UAtomQuartzClockHandle*>(NewObject<UAtomQuartzClockHandle>()->Init(WorldContextObject->GetWorld()));
	ClockHandle->SubscribeToClock(WorldContextObject, ClockName, &ClockProxy);

	// if we are not the manager for the clock, it means the FAtomRuntime is,
	// so we hold onto our own copy of the proxy
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));
	ClockManagerDataPtr->ActiveAudioMixerClockProxies.Add(ClockProxy);

	return ClockHandle;
}


void UAtomQuartzSubsystem::DeleteClockByName(const UObject* WorldContextObject, FName ClockName)
{
	// first look for the clock on the audio device's clock manager
	bool bShouldDeleteSynchronous = false;
	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject, /*bUseAudioEngineClockManager*/ true);

	if (ClockManager && !ClockManager->DoesClockExist(ClockName))
	{
		// if we didn't find it, assume the clock is on the subsystem's clock manager
		ClockManager = GetClockManager(WorldContextObject, /*bUseAudioEngineClockManager*/ false);
		bShouldDeleteSynchronous = true;
	}

	// if the clock is managed by the audio mixer device,
	// bShouldDeleteSynchronous is false, and it will be deleted on the correct thread.
	// if its managed by the subsystem, bShouldDeleteSynchronous will be true
	if (ClockManager)
	{
		ClockManager->RemoveClock(ClockName, bShouldDeleteSynchronous);
		PruneStaleProxies();
	}
}

void UAtomQuartzSubsystem::DeleteClockByHandle(const UObject* WorldContextObject, UAtomQuartzClockHandle*& InClockHandle)
{
	if (InClockHandle)
	{
		DeleteClockByName(WorldContextObject, InClockHandle->GetClockName());
	}
}

UAtomQuartzClockHandle* UAtomQuartzSubsystem::GetHandleForClock(const UObject* WorldContextObject, FName ClockName)
{
	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject);
	if (!ClockManager || !ClockManager->DoesClockExist(ClockName))
	{
		return nullptr;
	}

	Atom::FQuartzClockProxy ClockHandle = ClockManager->GetClock(ClockName);

	UAtomQuartzClockHandle* ClockHandlePtr = static_cast<UAtomQuartzClockHandle*>(NewObject<UAtomQuartzClockHandle>()->Init(WorldContextObject->GetWorld()));
	return ClockHandlePtr->SubscribeToClock(WorldContextObject, ClockName, &ClockHandle);
}


Atom::FQuartzClockProxy UAtomQuartzSubsystem::GetProxyForClock(FName ClockName) const
{
	if (Atom::FQuartzClockProxy const* ProxyPtr = FindProxyByName(ClockName))
	{
		return *ProxyPtr; // caller gets their own copy
	}

	return {};
}

void UAtomQuartzSubsystem::AddProxyForExternalClock(const Atom::FQuartzClockProxy& InProxy)
{
	// make sure we aren't adding a duplicate name
	if (FindProxyByName(InProxy.GetClockName()))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Received request to add external Clock: (%s) when a clock of that name already exists (Ignoring Request)"), *InProxy.GetClockName().ToString());
		return;
	}
}


bool UAtomQuartzSubsystem::DoesClockExist(const UObject* WorldContextObject, FName ClockName)
{
	Atom::FQuartzClockProxy const* Proxy = FindProxyByName(ClockName);
	if (Proxy && Proxy->IsValid())
	{
		return Proxy->DoesClockExist();
	}

	return {};
}

bool UAtomQuartzSubsystem::IsClockRunning(const UObject* WorldContextObject, FName ClockName)
{
	Atom::FQuartzClockProxy const* Proxy = FindProxyByName(ClockName);
	if (Proxy && Proxy->IsValid())
	{
		return Proxy->IsClockRunning();
	}

	return {};
}

float UAtomQuartzSubsystem::GetDurationOfQuantizationTypeInSeconds(const UObject* WorldContextObject, FName ClockName, const EQuartzCommandQuantization& QuantizationType, float Multiplier)
{
	Atom::FQuartzClockProxy const* Proxy = FindProxyByName(ClockName);
	if (Proxy && Proxy->IsValid())
	{
		return Proxy->GetDurationOfQuantizationTypeInSeconds(QuantizationType, Multiplier);
	}

	return {};
}

FQuartzTransportTimeStamp UAtomQuartzSubsystem::GetCurrentClockTimestamp(const UObject* WorldContextObject, const FName& InClockName)
{
	Atom::FQuartzClockProxy const* Proxy = FindProxyByName(InClockName);
	if (Proxy && Proxy->IsValid())
	{
		return Proxy->GetCurrentClockTimestamp();
	}

	return {};
}

float UAtomQuartzSubsystem::GetEstimatedClockRunTime(const UObject* WorldContextObject, const FName& InClockName)
{
	Atom::FQuartzClockProxy const* Proxy = FindProxyByName(InClockName);
	if (Proxy && Proxy->IsValid())
	{
		return Proxy->GetEstimatedClockRunTimeSeconds();
	}

	return {};
}

// todo: move FQuartLatencyTracker off the AudioMixerClockManager? (GameThread->AtomRenderThread tracking)
float UAtomQuartzSubsystem::GetGameThreadToAtomRenderThreadAverageLatency(const UObject* WorldContextObject)
{
	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject);
	if (!ClockManager)
	{
		return { };
	}
	return ClockManager->GetLifetimeAverageLatency();
}


float UAtomQuartzSubsystem::GetGameThreadToAtomRenderThreadMinLatency(const UObject* WorldContextObject)
{
	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject);
	if (!ClockManager)
	{
		return { };
	}
	return ClockManager->GetMinLatency();
}


float UAtomQuartzSubsystem::GetGameThreadToAtomRenderThreadMaxLatency(const UObject* WorldContextObject)
{
	Atom::FQuartzClockManager* ClockManager = GetClockManager(WorldContextObject);
	if (!ClockManager)
	{
		return { };
	}
	return ClockManager->GetMinLatency();
}


float UAtomQuartzSubsystem::GetAtomRenderThreadToGameThreadAverageLatency()
{
	check(TickableObjectManagerPtr.IsValid());
	return TickableObjectManagerPtr->GetLifetimeAverageLatency();
}


float UAtomQuartzSubsystem::GetAtomRenderThreadToGameThreadMinLatency()
{
	check(TickableObjectManagerPtr.IsValid());
	return TickableObjectManagerPtr->GetMinLatency();
}


float UAtomQuartzSubsystem::GetAtomRenderThreadToGameThreadMaxLatency()
{
	check(TickableObjectManagerPtr.IsValid());
	return TickableObjectManagerPtr->GetMaxLatency();
}


float UAtomQuartzSubsystem::GetRoundTripAverageLatency(const UObject* WorldContextObject)
{
	// very much an estimate
	return GetAtomRenderThreadToGameThreadAverageLatency() + GetGameThreadToAtomRenderThreadAverageLatency(WorldContextObject);
}


float UAtomQuartzSubsystem::GetRoundTripMinLatency(const UObject* WorldContextObject)
{
	return GetAtomRenderThreadToGameThreadMaxLatency() + GetGameThreadToAtomRenderThreadMaxLatency(WorldContextObject);
}


float UAtomQuartzSubsystem::GetRoundTripMaxLatency(const UObject* WorldContextObject)
{
	return GetAtomRenderThreadToGameThreadMinLatency() + GetGameThreadToAtomRenderThreadMinLatency(WorldContextObject);
}

void UAtomQuartzSubsystem::SetQuartzSubsystemTickableWhenPaused(const bool bInTickableWhenPaused)
{
	bTickEvenWhenPaused = bInTickableWhenPaused;
}

TWeakPtr<FAtomQuartzTickableObjectsManager> UAtomQuartzSubsystem::GetTickableObjectManager() const
{
	return TickableObjectManagerPtr;
}

void UAtomQuartzSubsystem::PruneStaleProxies()
{
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));
	PruneStaleProxiesInternal(ClockManagerDataPtr->ActiveExternalClockProxies);
	PruneStaleProxiesInternal(ClockManagerDataPtr->ActiveAudioMixerClockProxies);
}

void UAtomQuartzSubsystem::PruneStaleProxiesInternal(TArray<Atom::FQuartzClockProxy>& ContainerToPrune)
{
	 for(int32 i = 0; i < ContainerToPrune.Num(); ++i)
	 {
		 if (ContainerToPrune[i].IsValid() == false)
		 {
		 	ContainerToPrune.RemoveAtSwap(i--, EAllowShrinking::No);
		 }
	 }
}

Atom::FQuartzClockProxy* UAtomQuartzSubsystem::FindProxyByName(const FName& ClockName)
{
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));
	Atom::FQuartzClockProxy* Result = ClockManagerDataPtr->ActiveAudioMixerClockProxies.FindByKey(ClockName);

	// if the subsystem doesn't have a match, check the externally-registered clock proxies
	if (!Result)
	{
		Result = ClockManagerDataPtr->ActiveExternalClockProxies.FindByKey(ClockName);
	}

	return Result;
}

Atom::FQuartzClockProxy const* UAtomQuartzSubsystem::FindProxyByName(const FName& ClockName) const
{
	check(ClockManagerDataPtr);
	TUniquePtr<FScopeLock> Lock(GetPersistentStateScopeLock(GetWorld()));

	Atom::FQuartzClockProxy const* Result = ClockManagerDataPtr->ActiveAudioMixerClockProxies.FindByKey(ClockName);

	// if the subsystem doesn't have a match, check the externally-registered clock proxies
	if (!Result)
	{
		Result = ClockManagerDataPtr->ActiveExternalClockProxies.FindByKey(ClockName);
	}

	return Result;
}

