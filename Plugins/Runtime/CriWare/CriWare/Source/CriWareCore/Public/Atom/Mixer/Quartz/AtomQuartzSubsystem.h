
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Atom/AtomQuartzQuantizationUtilities.h"
#include "AtomMixerClockManager.h"

#include "AtomQuartzSubsystem.generated.h"

#define CRI_API CRIWARECORE_API

// Forward Definitions
namespace Atom
{
	class FQuartzClockManager;
	class FQuartzShareableCommandQueue;
}

class FQuartzTickableObject;
class UAtomQuartzClockHandle;
using MetronomeCommandQueuePtr = TSharedPtr<Atom::FQuartzShareableCommandQueue, ESPMode::ThreadSafe>;

struct FAtomQuartzTickableObjectsManager : public FQuartLatencyTracker
{
public:
	CRI_API void Tick(float DeltaTime);
	CRI_API bool IsTickable() const;
	CRI_API void SubscribeToQuartzTick(FQuartzTickableObject* InObjectToTick);
	CRI_API void UnsubscribeFromQuartzTick(FQuartzTickableObject* InObjectToTick);

private:
	// list of objects needing to be ticked by Quartz
	TArray<FQuartzTickableObject *> QuartzTickSubscribers;

	// index to track the next subscriber to tick (if updates are being amortized across multiple UObject Ticks)
	int32 UpdateIndex{ 0 };
};

UCLASS(DisplayName = "Atom Quartz", MinimalAPI)
class UAtomQuartzSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// ctor/dtor
	UAtomQuartzSubsystem() = default;
	virtual ~UAtomQuartzSubsystem() override = default;

	//~ Begin UWorldSubsystem Interface
	CRI_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	CRI_API virtual void Deinitialize() override;
	CRI_API virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	CRI_API void virtual BeginDestroy() override;
	//~ End UWorldSubsystem Interface

	//~ Begin FTickableGameObject Interface
	CRI_API virtual void Tick(float DeltaTime) override;
	CRI_API virtual bool IsTickableWhenPaused() const override;
	CRI_API virtual bool IsTickable() const override;
	CRI_API virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

	// these calls are forwarded to the internal FQuartzTickableObjectsManager
	CRI_API void SubscribeToQuartzTick(FQuartzTickableObject* InObjectToTick);
	CRI_API void UnsubscribeFromQuartzTick(FQuartzTickableObject* InObjectToTick);

	// get C++ handle (proxy) to a clock if it exists
	CRI_API Atom::FQuartzClockProxy GetProxyForClock(FName ClockName) const;

	// allow an external clock (not ticked by the Atom runtime or AtomQuartzSubsystem) to be accessible via this subsystem
	CRI_API void AddProxyForExternalClock(const Atom::FQuartzClockProxy& InProxy);

	// static methods
	static CRI_API UAtomQuartzSubsystem* Get(const UWorld* const World);

	// Helper functions for initializing quantized command initialization struct (to consolidate eyesore)
	static CRI_API Atom::FQuartzQuantizedRequestData CreateRequestDataForTickRateChange(UAtomQuartzClockHandle* InClockHandle, const FOnQuartzCommandEventBP& InDelegate, const Audio::FQuartzClockTickRate& InNewTickRate, const FQuartzQuantizationBoundary& InQuantizationBoundary);
	static CRI_API Atom::FQuartzQuantizedRequestData CreateRequestDataForTransportReset(UAtomQuartzClockHandle* InClockHandle, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate);
	static CRI_API Atom::FQuartzQuantizedRequestData CreateRequestDataForStartOtherClock(UAtomQuartzClockHandle* InClockHandle, FName InClockToStart, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate);
	static CRI_API Atom::FQuartzQuantizedRequestData CreateRequestDataForSchedulePlaySound(UAtomQuartzClockHandle* InClockHandle, const FOnQuartzCommandEventBP& InDelegate, const FQuartzQuantizationBoundary& InQuantizationBoundary);
	static CRI_API Atom::FQuartzQuantizedRequestData CreateRequestDataForQuantizedNotify(UAtomQuartzClockHandle* InClockHandle, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, float InMsOffset = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle")
	CRI_API bool IsQuartzEnabled();

	// Clock Creation
	// create a new clock (or return handle if clock already exists)
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bUseAtomAudioEngineClockManager"))
	CRI_API UAtomQuartzClockHandle* CreateNewClock(const UObject* WorldContextObject, FName ClockName, FQuartzClockSettings InSettings, bool bOverrideSettingsIfClockExists = false, bool bUseAtomAudioEngineClockManager = true);

	// delete an existing clock given its name
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API void DeleteClockByName(const UObject* WorldContextObject, FName ClockName);

	// delete an existing clock given its clock handle
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API void DeleteClockByHandle(const UObject* WorldContextObject, UPARAM(ref) UAtomQuartzClockHandle*& InClockHandle);

	// get handle for existing clock
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API UAtomQuartzClockHandle* GetHandleForClock(const UObject* WorldContextObject, FName ClockName);

	// returns true if the clock exists
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API bool DoesClockExist(const UObject* WorldContextObject, FName ClockName);

	// returns true if the clock is running
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API bool IsClockRunning(const UObject* WorldContextObject, FName ClockName);

	// Returns the duration in seconds of the given Quantization Type
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetDurationOfQuantizationTypeInSeconds(const UObject* WorldContextObject, FName ClockName, const EQuartzCommandQuantization& QuantizationType, float Multiplier = 1.0f);

	// Retrieves a timestamp for the clock
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API FQuartzTransportTimeStamp GetCurrentClockTimestamp(const UObject* WorldContextObject, const FName& InClockName);

	// Returns the amount of time, in seconds, the clock has been running. Caution: due to latency, this will not be perfectly accurate
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetEstimatedClockRunTime(const UObject* WorldContextObject, const FName& InClockName);

	// latency data (Game thread -> Audio Render Thread)
	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetGameThreadToAtomRenderThreadAverageLatency(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetGameThreadToAtomRenderThreadMinLatency(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetGameThreadToAtomRenderThreadMaxLatency(const UObject* WorldContextObject);

	// latency data (Audio Render Thread -> Game thread)
	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem")
	CRI_API float GetAtomRenderThreadToGameThreadAverageLatency();

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem")
	CRI_API float GetAtomRenderThreadToGameThreadMinLatency();

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem")
	CRI_API float GetAtomRenderThreadToGameThreadMaxLatency();

	// latency data (Round trip)
	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetRoundTripAverageLatency(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetRoundTripMinLatency(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetRoundTripMaxLatency(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem")
	CRI_API void SetQuartzSubsystemTickableWhenPaused(const bool bInTickableWhenPaused);

	// sharable to allow non-UObjects to un-subscribe if the Subsystem is going to outlive them
	CRI_API TWeakPtr<FAtomQuartzTickableObjectsManager> GetTickableObjectManager() const;

private:
	// deletes proxies to clocks that no longer exists
	CRI_API void PruneStaleProxies();
	static CRI_API void PruneStaleProxiesInternal(TArray<Atom::FQuartzClockProxy>& ContainerToPrune);

	// sharable tickable object manager to allow for non-UObject subscription / un-subscription
	TSharedPtr<FAtomQuartzTickableObjectsManager> TickableObjectManagerPtr { MakeShared<FAtomQuartzTickableObjectsManager>() };

	// Clock manager/proxy-related data that lives on the AudioDevice for persistence.
	TSharedPtr<Atom::FPersistentQuartzSubsystemData> ClockManagerDataPtr { nullptr };

	bool bTickEvenWhenPaused = false;

	// helpers
	CRI_API Atom::FQuartzClockProxy* FindProxyByName(const FName& ClockName);
	CRI_API Atom::FQuartzClockProxy const* FindProxyByName(const FName& ClockName) const;
	CRI_API Atom::FQuartzClockManager* GetClockManager(const UObject* WorldContextObject, bool bUseAtomAudioEngineClockManager = true);
};

#undef CRI_API
