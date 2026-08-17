
#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"

#include "Sound/QuartzSubscription.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "Sound/QuartzInterfaces.h"
#include "Sound/QuartzCommandQueue.h"
#endif

#include "AtomQuartzMetronome.h"
#include "Atom/AtomQuartzQuantizationUtilities.h"

#define CRI_API CRIWARECORE_API

class FAtomRuntime;

namespace Atom
{
	// Forward Defintions
	class FQuartzClock;
	class FMixerSourceManager;
	class FQuartzClockManager;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	using FQuartzClockCommandQueueType = Audio::Quartz::PrivateDefs::TQuartzCommandQueue<IQuartzClock>;
	using FQuartzClockCommandQueuePtr = TSharedPtr<FQuartzClockCommandQueueType, ESPMode::ThreadSafe>;
	using FQuartzClockCommandQueueWeakPtr = TWeakPtr<FQuartzClockCommandQueueType, ESPMode::ThreadSafe>;
#else
	using FQuartzClockCommandQueuePtr = TSharedPtr<Audio::TQuartzShareableCommandQueue<FQuartzClock>, ESPMode::ThreadSafe>;
	using FQuartzClockCommandQueueWeakPtr = TWeakPtr<Audio::TQuartzShareableCommandQueue<FQuartzClock>, ESPMode::ThreadSafe>;
#endif

	/**
	 *	FQuartzClockProxy:
	 *
	 *		This class is a C++ handle to the underlying clock.
	 *		
	 *		It is mostly a wrapper around a TWeakPtr<FQuartzClock> and
	 *		FQuartzClockCommandQueueType
	 *		
	 *		The getters query the underlying FQuartzClock directly,
	 *		which returns values updated during the last audio-engine tick
	 *
	 *		If you need to add more getters, add copies of the members in question to
	 *		FQuartzClock::FQuartzClockState and update FQuartzClock::UpdateCachedState()
	 *		for thread-safe access (or manually protect access w/ CachedClockStateCritSec)
	 *
	 *		SendCommandToClock() can be used to execute lambdas at the beginning
	 *		of the next clock tick.  These lambdas can call FQuartzClock's public methods safely.
	 *
	 *		Your lambda will take an FQuartzClock* as an argument, which will be passed in by the
	 *		FQuartzClock itself when it pumps the command queue.
	 *
	 */
	class FQuartzClockProxy
	{
	public:
		// ctor
		FQuartzClockProxy() {}
		FQuartzClockProxy(const FName& Name) : ClockId(Name){ } // conv ctor from FName
		CRI_API FQuartzClockProxy(TSharedPtr<FQuartzClock, ESPMode::ThreadSafe> InClock);

		FName GetClockName() const { return ClockId; }

		CRI_API bool IsValid() const;
		operator bool() const { return IsValid(); }

		bool operator==(const FName& Name) const { return ClockId == Name; }

		CRI_API bool DoesClockExist() const;

		CRI_API bool IsClockRunning() const;

		CRI_API Audio::FQuartzClockTickRate GetTickRate() const;

		CRI_API float GetEstimatedClockRunTimeSeconds() const;

		CRI_API FQuartzTransportTimeStamp GetCurrentClockTimestamp() const;

		CRI_API float GetDurationOfQuantizationTypeInSeconds(const EQuartzCommandQuantization& QuantizationType, float Multiplier) const;

		CRI_API float GetBeatProgressPercent(const EQuartzCommandQuantization& QuantizationType) const;

		// returns false if the clock is not valid or has shut down
		CRI_API bool SendCommandToClock(TFunction<void(FQuartzClock*)> InCommand);

		// implicit cast to underlying ID (FName)
		operator const FName&() const { return ClockId; }

	private:
		FName ClockId;

		FQuartzClockCommandQueueWeakPtr SharedQueue;

	protected:
		TWeakPtr<FQuartzClock, ESPMode::ThreadSafe> ClockWeakPtr;

	}; // class FQuartzClockProxy


	
	/**
	 *	FQuartzClock:
	 *
	 *		This class receives, schedules, and fires quantized commands. 
	 *		The underlying FQuartzMetronome handles all counting / timing logic.
	 *
	 *		This class gets ticked externally (i.e. by some Clock Manager)
	 *		and counts down the time-to-fire the commands in audio frames.
	 *
	 *
	 *		UpdateCachedState() updates a game-thread copy of data accessed via FQuartzClockProxy
	 *		(see FQuartzClockState)
	 */

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	class FQuartzClock : public FQuartzClockCommandQueueType::TConsumerBase<Audio::Quartz::IQuartzClock>
#else
	class FQuartzClock
#endif
	{
	public:

		// ctor
		CRI_API FQuartzClock(const FName& InName, const FQuartzClockSettings& InClockSettings, FQuartzClockManager* InOwningClockManagerPtr = nullptr);

		// dtor
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		CRI_API virtual ~FQuartzClock() override;
#else
		CRI_API ~FQuartzClock();
#endif

		// Transport Control:
		// alter the tick rate (take by-value to make sample-rate adjustments in-place)
		CRI_API void ChangeTickRate(Audio::FQuartzClockTickRate InNewTickRate, int32 NumFramesLeft = 0);

		CRI_API void ChangeTimeSignature(const FQuartzTimeSignature& InNewTimeSignature);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		CRI_API virtual void Resume() override;

		CRI_API virtual void Pause() override;

		CRI_API virtual void Restart(bool bPause = true) override;

		CRI_API virtual void Stop(bool CancelPendingEvents) override;  // Pause + Restart
#else
		CRI_API void Resume();

		CRI_API void Pause();

		CRI_API void Restart(bool bPause = true);

		CRI_API void Stop(bool CancelPendingEvents);  // Pause + Restart
#endif
		CRI_API void SetSampleRate(float InNewSampleRate);

		CRI_API void ResetTransport(const int32 NumFramesToTickBeforeReset = 0);

		// (used for StartOtherClock command to handle the sub-tick as the target clock)
		CRI_API void AddToTickDelay(int32 NumFramesOfDelayToAdd);

		// (used for StartOtherClock command to handle the sub-tick as the target clock)
		CRI_API void SetTickDelay(int32 NumFramesOfDelay);

		CRI_API void Shutdown();

		// Getters:
		CRI_API Audio::FQuartzClockTickRate GetTickRate();

		CRI_API FName GetName() const;

		CRI_API bool IgnoresFlush() const;

		CRI_API bool DoesMatchSettings(const FQuartzClockSettings& InClockSettings) const;

		CRI_API bool HasPendingEvents() const;

		CRI_API int32 NumPendingEvents() const;

		CRI_API bool IsRunning() const;

		CRI_API float GetDurationOfQuantizationTypeInSeconds(const EQuartzCommandQuantization& QuantizationType, float Multiplier);

		CRI_API float GetBeatProgressPercent(const EQuartzCommandQuantization& QuantizationType) const;

		CRI_API FQuartzTransportTimeStamp GetCurrentTimestamp();

		CRI_API float GetEstimatedRunTime();

		CRI_API FAtomRuntime* GetAtomRuntime();

		CRI_API FMixerSourceManager* GetSourceManager();

		CRI_API FQuartzClockManager* GetClockManager();

		CRI_API FQuartzClockCommandQueueWeakPtr GetCommandQueue() const;

		// Metronome Event Subscription:
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		CRI_API virtual void SubscribeToTimeDivision(Audio::FQuartzGameThreadSubscriber InSubscriber, EQuartzCommandQuantization InQuantizationBoundary) override;

		CRI_API virtual void SubscribeToAllTimeDivisions(Audio::FQuartzGameThreadSubscriber InSubscriber) override;

		CRI_API virtual void UnsubscribeFromTimeDivision(Audio::FQuartzGameThreadSubscriber InSubscriber, EQuartzCommandQuantization InQuantizationBoundary) override;

		CRI_API virtual void UnsubscribeFromAllTimeDivisions(Audio::FQuartzGameThreadSubscriber InSubscriber) override;
#else
		CRI_API void SubscribeToTimeDivision(Audio::FQuartzGameThreadSubscriber InSubscriber, EQuartzCommandQuantization InQuantizationBoundary);

		CRI_API void SubscribeToAllTimeDivisions(Audio::FQuartzGameThreadSubscriber InSubscriber);

		CRI_API void UnsubscribeFromTimeDivision(Audio::FQuartzGameThreadSubscriber InSubscriber, EQuartzCommandQuantization InQuantizationBoundary);

		CRI_API void UnsubscribeFromAllTimeDivisions(Audio::FQuartzGameThreadSubscriber InSubscriber);
#endif

		// Quantized Command Management:
		CRI_API void AddQuantizedCommand(FQuartzQuantizedRequestData& InQuantizedRequestData);
		CRI_API void AddQuantizedCommand(FQuartzQuantizedCommandInitInfo& InQuantizationCommandInitInfo);
		CRI_API void AddQuantizedCommand(FQuartzQuantizationBoundary InQuantizationBoundary, TSharedPtr<Atom::IQuartzQuantizedCommand> InNewEvent);

		CRI_API bool CancelQuantizedCommand(TSharedPtr<IQuartzQuantizedCommand> InCommandPtr);
		
		// low-resolution clock update
		// (not sample-accurate!, useful when running without an Audio Device)
		CRI_API void LowResolutionTick(float InDeltaTimeSeconds);

		// sample accurate clock update
		CRI_API void Tick(int32 InNumFramesUntilNextTick);

	private:
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		// interface in AudioQuartz request this interface intsead of base class... 
		CRI_API virtual void AddQuantizedCommand(Audio::FQuartzQuantizedRequestData& InQuantizedRequestData) override {}
		CRI_API virtual void AddQuantizedCommand(Audio::FQuartzQuantizedCommandInitInfo& InQuantizationCommandInitInfo) override {}
		CRI_API virtual void AddQuantizedCommand(FQuartzQuantizationBoundary InQuantizationBoundary, TSharedPtr<Audio::IQuartzQuantizedCommand> InNewEvent) override {}
#endif
		// Contains the pending command and the number of frames it has to wait to fire
		struct PendingCommand
		{
			// ctor
			PendingCommand(TSharedPtr<IQuartzQuantizedCommand> InCommand, int32 InNumFramesUntilExec)
				: Command(InCommand)
				, NumFramesUntilExec(InNumFramesUntilExec)
			{
			}

			// Quantized Command Object
			TSharedPtr<IQuartzQuantizedCommand> Command;

			// Countdown to execution
			int32 NumFramesUntilExec{ 0 };
		}; // struct PendingCommand

		// mutex-protected update at the end of Tick()
		FCriticalSection CachedClockStateCritSec;
		void UpdateCachedState();

		// data is cached when an FQuartzClock is ticked
		struct FQuartzClockState
		{
			Audio::FQuartzClockTickRate TickRate;
			FQuartzTransportTimeStamp TimeStamp;
			float RunTimeInSeconds;
			float MusicalDurationPhases[static_cast<int32>(EQuartzCommandQuantization::Count)] { 0 };
			float MusicalDurationPhaseDeltas[static_cast<int32>(EQuartzCommandQuantization::Count)] { 0 };
			uint64 LastCacheTickCpuCycles64 = 0;
			uint64 LastCacheTickDeltaCpuCycles64 = 0;
			
		} CachedClockState;

		void TickInternal(int32 InNumFramesUntilNextTick, TArray<PendingCommand>& CommandsToTick, int32 FramesOfLatency = 0, int32 FramesOfDelay = 0);

		bool CancelQuantizedCommandInternal(TSharedPtr<IQuartzQuantizedCommand> InCommandPtr, TArray<PendingCommand>& CommandsToTick);

		// don't allow default ctor, a clock needs to be ready to be used
		// by the clock manager / FAtomRuntime once constructed
		FQuartzClock() = delete;

		FQuartzMetronome Metronome;

		FQuartzClockManager* OwningClockManagerPtr{ nullptr };

		FName Name;

		float ThreadLatencyInMilliseconds{ 40.f };

		// Command queue handed out to GameThread objects to queue commands. These get executed at the top of Tick()
		mutable FQuartzClockCommandQueuePtr PreTickCommands; // (mutable for lazy init in GetQuartzSubscriber())

		// Container of external commands to be executed (TUniquePointer<QuantizedAudioCommand>)
		TArray<PendingCommand> ClockAlteringPendingCommands;
		TArray<PendingCommand> PendingCommands;

		FThreadSafeBool bIsRunning{ true };

		bool bIgnoresFlush{ false };

		int32 TickDelayLengthInFrames{ 0 };

	}; // class FQuartzClock
} // namespace

#undef CRI_API
