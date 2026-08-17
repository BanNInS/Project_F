
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Curves/RichCurve.h"
#include "Curves/CurveBase.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "Sound/QuartzSubscription.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomQuartzSubsystem.h"
#include "AtomQuartzMetronome.h"

#include "AtomMixerClockHandle.generated.h"

#define CRI_API CRIWARECORE_API

/**
 *  This class is a BP / Game thread wrapper around FQuartzClockProxy
 *	(to talk to the underlying clock)
 
 *  ...and inherits from FQuartzTickableObject
 *	(to listen to the underlying clock)
 *  
 *  It can subscribe to Quantized Event & Metronome delegates to synchronize
 *  gameplay & VFX to Quartz events fired from the Audio Engine
 */
UCLASS(BlueprintType, Blueprintable, Transient, ClassGroup = Quartz, meta = (BlueprintSpawnableComponent), MinimalAPI)
class UAtomQuartzClockHandle : public UObject, public FQuartzTickableObject
{
	GENERATED_BODY()

public:
	// ctor
	CRI_API UAtomQuartzClockHandle();

	// dtor
	CRI_API ~UAtomQuartzClockHandle();

	// begin UObject interface
	CRI_API void BeginDestroy() override;
	// end UObject interface

private:
	// begin FQuartzTickableObject interface
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	CRI_API virtual bool ShouldUnsubscribe() override;
#endif
	// end FQuartzTickableObject interface
	
public:

// Begin Blueprint Interface

	// Clock manipulation
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void StartClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API void StopClock(const UObject* WorldContextObject, bool CancelPendingEvents, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Subsystem", meta = (WorldContext = "WorldContextObject"))
	CRI_API void PauseClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void ResumeClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "Transport, Counter"))
	CRI_API void ResetTransport(const UObject* WorldContextObject, const FOnQuartzCommandEventBP& InDelegate);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "Transport, Counter"))
	CRI_API void ResetTransportQuantized(const UObject* WorldContextObject, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject", Keywords = "Transport, Counter"))
	CRI_API bool IsClockRunning(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "Transport, Counter"))
	CRI_API void NotifyOnQuantizationBoundary(const UObject* WorldContextObject, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, float InMsOffset = 0.f);

	/** Returns the duration in seconds of the given Quantization Type
	 *
	 * @param The Quantization type to measure
	 * @param The quantity of the Quantization Type to calculate the time of
	 * @return The duration, in seconds, of a multiplier amount of the Quantization Type, or -1 in the case the clock is invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetDurationOfQuantizationTypeInSeconds(const UObject* WorldContextObject, const EQuartzCommandQuantization& QuantizationType, float Multiplier = 1.0f);

	//Retrieves a timestamp for the clock
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API FQuartzTransportTimeStamp GetCurrentTimestamp(const UObject* WorldContextObject);

	// Returns the amount of time, in seconds, the clock has been running. Caution: due to latency, this will not be perfectly accurate
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock Handle", meta = (WorldContext = "WorldContextObject"))
	CRI_API float GetEstimatedRunTime(const UObject* WorldContextObject);

	// "other" clock manipulation
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "Transport, Counter"))
	CRI_API void StartOtherClock(const UObject* WorldContextObject, FName OtherClockName, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate);

	// Metronome subscription
	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void SubscribeToQuantizationEvent(const UObject* WorldContextObject, EQuartzCommandQuantization InQuantizationBoundary, const FOnQuartzMetronomeEventBP& OnQuantizationEvent, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void SubscribeToAllQuantizationEvents(const UObject* WorldContextObject, const FOnQuartzMetronomeEventBP& OnQuantizationEvent, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void UnsubscribeFromTimeDivision(const UObject* WorldContextObject, EQuartzCommandQuantization InQuantizationBoundary, UAtomQuartzClockHandle*& ClockHandle);

	UFUNCTION(BlueprintCallable, Category = "Quartz Clock", meta = (WorldContext = "WorldContextObject"))
	CRI_API void UnsubscribeFromAllTimeDivisions(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle);

	// Metronome Alteration (setters)
	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "QuantizationBoundary, Delegate", AutoCreateRefTerm = "QuantizationBoundary, Delegate", Keywords = "BPM, Tempo"))
	CRI_API void SetMillisecondsPerTick(const UObject* WorldContextObject, UPARAM(ref) const FQuartzQuantizationBoundary& QuantizationBoundary, const FOnQuartzCommandEventBP& Delegate, UAtomQuartzClockHandle*& ClockHandle, float MillisecondsPerTick = 100.f);

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "QuantizationBoundary, Delegate", AutoCreateRefTerm = "QuantizationBoundary, Delegate", Keywords = "BPM, Tempo"))
	CRI_API void SetTicksPerSecond(const UObject* WorldContextObject, UPARAM(ref) const FQuartzQuantizationBoundary& QuantizationBoundary, const FOnQuartzCommandEventBP& Delegate, UAtomQuartzClockHandle*& ClockHandle, float TicksPerSecond = 10.f);

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "QuantizationBoundary, Delegate", AutoCreateRefTerm = "QuantizationBoundary, Delegate", Keywords = "BPM, Tempo"))
	CRI_API void SetSecondsPerTick(const UObject* WorldContextObject, UPARAM(ref) const FQuartzQuantizationBoundary& QuantizationBoundary, const FOnQuartzCommandEventBP& Delegate, UAtomQuartzClockHandle*& ClockHandle, float SecondsPerTick = 0.25f);

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "QuantizationBoundary, Delegate", AutoCreateRefTerm = "QuantizationBoundary, Delegate", Keywords = "BPM, Tempo"))
	CRI_API void SetThirtySecondNotesPerMinute(const UObject* WorldContextObject, UPARAM(ref) const FQuartzQuantizationBoundary& QuantizationBoundary, const FOnQuartzCommandEventBP& Delegate, UAtomQuartzClockHandle*& ClockHandle, float ThirtySecondsNotesPerMinute = 960.f);

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "QuantizationBoundary, Delegate", AutoCreateRefTerm = "QuantizationBoundary, Delegate", Keywords = "BPM, Tempo"))
	CRI_API void SetBeatsPerMinute(const UObject* WorldContextObject, UPARAM(ref) const FQuartzQuantizationBoundary& QuantizationBoundary, const FOnQuartzCommandEventBP& Delegate, UAtomQuartzClockHandle*& ClockHandle, float BeatsPerMinute = 60.f);

	// Metronome getters
	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	CRI_API float GetMillisecondsPerTick(const UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	CRI_API float GetTicksPerSecond(const UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	CRI_API float GetSecondsPerTick(const UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	CRI_API float GetThirtySecondNotesPerMinute(const UObject* WorldContextObject) const;

	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	CRI_API float GetBeatsPerMinute(const UObject* WorldContextObject) const;

	/**
	 * Returns the current progress until the next occurrence of the provided musical duration as a float value from 0 (previous beat) to 1 (next beat).
	 * This is useful for indexing into curves to animate parameters to musical time.
	 * Ms and Phase offsets are combined internally.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quantization", meta = ( AutoCreateRefTerm = "PhaseOffset", Keywords = "BPM, Tempo"))
	CRI_API float GetBeatProgressPercent(EQuartzCommandQuantization QuantizationBoundary = EQuartzCommandQuantization::Beat, float PhaseOffset = 0.f, float MsOffset = 0.f);

	// todo: un-comment when metronome events support the offset
	// Set how early we would like to receive Metronome (not yet supported) and "About To Start" Delegates. (all other command delegates will execute as normal)
	// UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "InDelegate", Keywords = "BPM, Tempo"))
	// void SetNotificationAnticipationAmountInMilliseconds(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle, const double Milliseconds = 0.0);

	// // Set how early we would like to receive Metronome (not yet supported) and "About To Start" Delegates. (all other command delegates will execute as normal)
	// UFUNCTION(BlueprintCallable, Category = "Quantization", meta = (WorldContext = "WorldContextObject", Keywords = "BPM, Tempo"))
	// void SetNotificationAnticipationAmountAsMusicalDuration(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle, const EQuartzCommandQuantization MusicalDuration = EQuartzCommandQuantization::QuarterNote, const double Multiplier = 1.0);

// End Blueprint Interface
	CRI_API void QueueQuantizedSound(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle, const FAtomComponentCommandInfo& AudioComponentData, const FOnQuartzCommandEventBP& InDelegate, const FQuartzQuantizationBoundary& InTargetBoundary);

	CRI_API UAtomQuartzClockHandle* SubscribeToClock(const UObject* WorldContextObject, FName ClockName, Atom::FQuartzClockProxy const* InHandlePtr = nullptr);

	FName GetClockName() const { return CurrentClockId; }

	bool DoesClockExist(const UObject* WorldContextObject) const
	{
		return RawHandle.DoesClockExist();
	}

	CRI_API bool GetCurrentTickRate(const UObject* WorldContextObject, Audio::FQuartzClockTickRate& OutTickRate) const;

private:
	CRI_API void SetTickRateInternal(const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, const Audio::FQuartzClockTickRate& NewTickRate);

	Atom::FQuartzClockProxy RawHandle;

	FName CurrentClockId;
};

#undef CRI_API
