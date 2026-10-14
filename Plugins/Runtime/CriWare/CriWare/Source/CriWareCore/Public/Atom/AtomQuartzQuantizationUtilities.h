
#pragma once

#include "Sound/QuartzQuantizationUtilities.h"

class FAtomRuntime;

namespace Atom
{
	class FQuartzClock;
	class IQuartzQuantizedCommand;

	// data that is gathered by the AtomThread to get passed from FAtomActiveSound->FMixerSourceVoice
	// eventually converted to IQuartzQuantizedCommand for the Quantized Command itself
	struct FQuartzQuantizedRequestData
	{
		// shared with FQuartzQuantizedCommandInitInfo:
		FName ClockName;
		FName OtherClockName;
		TSharedPtr<IQuartzQuantizedCommand> QuantizedCommandPtr;
		FQuartzQuantizationBoundary QuantizationBoundary{ EQuartzCommandQuantization::Tick, 1.f, EQuarztQuantizationReference::BarRelative, true };
		TArray<Audio::FQuartzGameThreadSubscriber> GameThreadSubscribers;
		int32 GameThreadDelegateID = -1;
	};

	// data that is passed into IQuartzQuantizedCommand::OnQueued
	// info that derived classes need can be added here
	struct FQuartzQuantizedCommandInitInfo
	{
		FQuartzQuantizedCommandInitInfo() = default;

		// conversion ctor from FQuartzQuantizedRequestData
		CRIWARECORE_API FQuartzQuantizedCommandInitInfo(const FQuartzQuantizedRequestData& RHS, float InSampleRate, int32 InSourceID = INDEX_NONE);

		void SetOwningClockPtr(TSharedPtr<Atom::FQuartzClock> InClockPointer)
		{
			OwningClockPointer = InClockPointer;
			ensure(OwningClockPointer);
		}

		// shared with FQuartzQuantizedRequestData
		FName ClockName;
		FName OtherClockName;
		TSharedPtr<IQuartzQuantizedCommand> QuantizedCommandPtr{ nullptr };
		FQuartzQuantizationBoundary QuantizationBoundary;
		TArray<Audio::FQuartzGameThreadSubscriber> GameThreadSubscribers;
		int32 GameThreadDelegateID{ -1 };

		// Audio Render thread-specific data:
		TSharedPtr<Atom::FQuartzClock> OwningClockPointer{ nullptr };
		float SampleRate{ 0 };
		int32 SourceID{ -1 };

		// Number of frames used for any FramesTilExec overrides
		int32 FrameOverrideAmount{ 0 };
	};

	// base class for quantized commands. Virtual methods called by owning clock.
	class IQuartzQuantizedCommand : public Audio::FQuartzCrossThreadMessage
	{
	public:

		// ctor
		CRIWARECORE_API IQuartzQuantizedCommand();

		// dtor
		CRIWARECORE_API virtual ~IQuartzQuantizedCommand();

		// allocate a copy of the derived class
		CRIWARECORE_API virtual TSharedPtr<IQuartzQuantizedCommand> GetDeepCopyOfDerivedObject() const;

		CRIWARECORE_API void AddSubscriber(Audio::FQuartzGameThreadSubscriber InSubscriber);

		// Command has reached the AudioRenderThread
		CRIWARECORE_API void OnQueued(const FQuartzQuantizedCommandInitInfo& InCommandInitInfo);

		// scheduled (finalize subscriber offsets) - called by FQuartzClock
		CRIWARECORE_API void OnScheduled(const Audio::FQuartzClockTickRate& InTickRate);

		// called during FQuartzClock::Tick() to let us call AboutToStart
		// at different times for different subscribers
		CRIWARECORE_API void Update(int32 NumFramesUntilDeadline);

		// Perhaps the associated sound failed concurrency and will not be playing
		CRIWARECORE_API void FailedToQueue(FQuartzQuantizedRequestData& InGameThreadData);

		// Called 2x Assumed thread latency before OnFinalCallback()
		CRIWARECORE_API void AboutToStart();

		// Called on the final callback of this event boundary.
		// InNumFramesLeft is the number of frames into the callback the exact quantized event should take place
		CRIWARECORE_API void OnFinalCallback(int32 InNumFramesLeft);

		// Called if the owning clock gets stopped
		CRIWARECORE_API void OnClockPaused();

		// Called if the owning clock gets started
		CRIWARECORE_API void OnClockStarted();

		// Called if the event is cancelled before OnFinalCallback() is called
		CRIWARECORE_API void Cancel();


		//Called if the event type uses an altered amount of frames
		virtual int32 OverrideFramesUntilExec(int32 NumFramesUntilExec) { return NumFramesUntilExec; }


		virtual bool IsClockAltering() { return false; }
		virtual bool ShouldDeadlineIgnoresBpmChanges() { return false; }
		virtual bool RequiresAtomRuntime() const { return false; }

		virtual FName GetCommandName() const = 0;
		virtual EQuartzCommandType GetCommandType() const = 0;


	private:
		// derived classes can override these to add extra functionality
		virtual void OnQueuedCustom(const FQuartzQuantizedCommandInitInfo& InCommandInitInfo) {}
		virtual void FailedToQueueCustom() {}
		virtual void AboutToStartCustom() {}
		virtual void OnFinalCallbackCustom(int32 InNumFramesLeft) {}
		virtual void OnClockPausedCustom() {}
		virtual void OnClockStartedCustom() {}
		virtual void CancelCustom() {}

		TArray<Audio::FQuartzGameThreadSubscriber> GameThreadSubscribers;

		int32 GameThreadDelegateID{ -1 };
	};

	// Atom Render Thread Handle to a queued command
	// Used by AtomMixerSourceVoices to access a pending associated command
	struct FQuartzQuantizedCommandHandle
	{
		FName OwningClockName;
		TSharedPtr<IQuartzQuantizedCommand> CommandPtr = nullptr;
		FAtomRuntime* AtomRuntime = nullptr;

		// Attempts to cancel the command. Returns true if the cancellation was successful.
		CRIWARECORE_API bool Cancel();

		// Resets the handle to initial state.
		CRIWARECORE_API void Reset();
	};
} // namespace

using FAtomComponentCommandInfo = FAudioComponentCommandInfo;
