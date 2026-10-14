
#include "Atom/AtomQuartzQuantizationUtilities.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Mixer/Quartz/AtomMixerClock.h"

namespace Atom
{
	FQuartzQuantizedCommandInitInfo::FQuartzQuantizedCommandInitInfo(
		const FQuartzQuantizedRequestData& RHS
		, float InSampleRate
		, int32 InSourceID
	)
		: ClockName(RHS.ClockName)
		, OtherClockName(RHS.OtherClockName)
		, QuantizedCommandPtr(RHS.QuantizedCommandPtr)
		, QuantizationBoundary(RHS.QuantizationBoundary)
		, GameThreadSubscribers(RHS.GameThreadSubscribers)
		, GameThreadDelegateID(RHS.GameThreadDelegateID)
		, OwningClockPointer(nullptr)
		, SampleRate(InSampleRate)
		, SourceID(InSourceID)
	{
	}

	IQuartzQuantizedCommand::IQuartzQuantizedCommand() = default;
	IQuartzQuantizedCommand::~IQuartzQuantizedCommand() = default;

	TSharedPtr<IQuartzQuantizedCommand> IQuartzQuantizedCommand::GetDeepCopyOfDerivedObject() const
	{
		// implement this method to allow copies to be made from pointers to base class
		checkSlow(false);
		return nullptr;
	}

	void IQuartzQuantizedCommand::AddSubscriber(Audio::FQuartzGameThreadSubscriber InSubscriber)
	{
		GameThreadSubscribers.AddUnique(InSubscriber);
	}

	void IQuartzQuantizedCommand::OnQueued(const FQuartzQuantizedCommandInitInfo& InCommandInitInfo)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::OnQueued);

		if (FAtomRuntime* AtomRuntime = InCommandInitInfo.OwningClockPointer->GetAtomRuntime())
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
			AtomRuntime->QuantizedEventClockManager.PushLatencyTrackerResult(FQuartzCrossThreadMessage::RequestReceived());
#else
			AtomRuntime->QuantizedEventClockManager.PushLatencyTrackerResult(FQuartzCrossThreadMessage::RequestRecieved());
#endif
		}

		GameThreadSubscribers.Append(InCommandInitInfo.GameThreadSubscribers);
		GameThreadDelegateID = InCommandInitInfo.GameThreadDelegateID;

		if (GameThreadSubscribers.Num())
		{
			Audio::FQuartzQuantizedCommandDelegateData Data;

			Data.CommandType = GetCommandType();
			Data.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnQueued;
			Data.DelegateID = GameThreadDelegateID;

			for (auto& Subscriber : GameThreadSubscribers)
			{
				Subscriber.PushEvent(Data);
			}
		}

		UE_LOG(LogAudioQuartz, Verbose, TEXT("OnQueued() called for quantized event type: [%s]"), *GetCommandName().ToString());
		OnQueuedCustom(InCommandInitInfo);
	}

	void IQuartzQuantizedCommand::OnScheduled(const Audio::FQuartzClockTickRate& InTickRate)
	{
		for(auto& Subscriber : GameThreadSubscribers)
		{
			Subscriber.FinalizeOffset(InTickRate);
		}
	}

	void IQuartzQuantizedCommand::Update(int32 NumFramesUntilDeadline)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::Countdown);

		Audio::FQuartzQuantizedCommandDelegateData Data;
		Data.CommandType = GetCommandType();
		Data.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnAboutToStart;
		Data.DelegateID = GameThreadDelegateID;

		for(auto& Subscriber : GameThreadSubscribers)
		{
			// we only want to send this notification to the subscriber once
			const int32 NumFramesOfAnticipation = Subscriber.GetOffsetAsAudioFrames();
			if(!Subscriber.HasBeenNotifiedOfAboutToStart()
				&&  (NumFramesOfAnticipation >= NumFramesUntilDeadline))
			{
				Subscriber.PushEvent(Data);
			}
		}
	}

	void IQuartzQuantizedCommand::FailedToQueue(FQuartzQuantizedRequestData& InGameThreadData)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::FailedToQueue);

		GameThreadSubscribers.Append(InGameThreadData.GameThreadSubscribers);
		GameThreadDelegateID = InGameThreadData.GameThreadDelegateID;

		if (GameThreadSubscribers.Num())
		{
			Audio::FQuartzQuantizedCommandDelegateData Data;
			Data.CommandType = GetCommandType();
			Data.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnFailedToQueue;
			Data.DelegateID = GameThreadDelegateID;

			for (auto& Subscriber : GameThreadSubscribers)
			{
				Subscriber.PushEvent(Data);
			}
		}

		UE_LOG(LogAudioQuartz, Verbose, TEXT("FailedToQueue() called for quantized event type: [%s]"), *GetCommandName().ToString());
		FailedToQueueCustom();
	}

	void IQuartzQuantizedCommand::AboutToStart()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::AboutToStart);

		Audio::FQuartzQuantizedCommandDelegateData Data;
		Data.CommandType = GetCommandType();
		Data.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnAboutToStart;
		Data.DelegateID = GameThreadDelegateID;

		for(auto& Subscriber : GameThreadSubscribers)
		{
			// we only want to send this notification to the subscriber once
			if(!Subscriber.HasBeenNotifiedOfAboutToStart())
			{
				Subscriber.PushEvent(Data);
			}
		}

		UE_LOG(LogAudioQuartz, Verbose, TEXT("AboutToStart() called for quantized event type: [%s]"), *GetCommandName().ToString());
		AboutToStartCustom();
	}

	void IQuartzQuantizedCommand::OnFinalCallback(int32 InNumFramesLeft)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::OnFinalCallback);
		if (GameThreadSubscribers.Num())
		{
			Audio::FQuartzQuantizedCommandDelegateData OnStartedData;

			OnStartedData.CommandType = GetCommandType();
			OnStartedData.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnStarted;
			OnStartedData.DelegateID = GameThreadDelegateID;

			for (auto& Subscriber : GameThreadSubscribers)
			{
				Subscriber.PushEvent(OnStartedData);
			}
		}

		UE_LOG(LogAudioQuartz, Verbose, TEXT("OnFinalCallback() called for quantized event type: [%s]"), *GetCommandName().ToString());
		OnFinalCallbackCustom(InNumFramesLeft);
	}

	void IQuartzQuantizedCommand::OnClockPaused()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::OnClockPaused);
		UE_LOG(LogAudioQuartz, Verbose, TEXT("OnClockPaused() called for quantized event type: [%s]"), *GetCommandName().ToString());
		OnClockPausedCustom();
	}

	void IQuartzQuantizedCommand::OnClockStarted()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::OnClockStarted);
		UE_LOG(LogAudioQuartz, Verbose, TEXT("OnClockStarted() called for quantized event type: [%s]"), *GetCommandName().ToString());
		OnClockStartedCustom();
	}

	void IQuartzQuantizedCommand::Cancel()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(QuartzQuantizedCommand::Cancel);
		Audio::FQuartzQuantizedCommandDelegateData Data;

		Data.CommandType = GetCommandType();
		Data.DelegateSubType = EQuartzCommandDelegateSubType::CommandOnCanceled;
		Data.DelegateID = GameThreadDelegateID;

		for (auto& Subscriber : GameThreadSubscribers)
		{
			Subscriber.PushEvent(Data);
		}

		UE_LOG(LogAudioQuartz, Verbose, TEXT("Cancel() called for quantized event type: [%s]"), *GetCommandName().ToString());
		CancelCustom();
	}

	bool FQuartzQuantizedCommandHandle::Cancel()
	{
		checkSlow(AtomRuntime);
		//checkSlow(AtomRuntime->IsAudioRenderingThread());

		if (CommandPtr && !OwningClockName.IsNone())
		{
			UE_LOG(LogAudioQuartz, Verbose, TEXT("OnQueued() called for quantized event type: [%s]"), *CommandPtr->GetCommandName().ToString());
			return AtomRuntime->QuantizedEventClockManager.CancelCommandOnClock(OwningClockName, CommandPtr);
		}

		return false;
	}

	void FQuartzQuantizedCommandHandle::Reset()
	{
		AtomRuntime = nullptr;
		CommandPtr.Reset();
		OwningClockName = FName();
	}
} // namespace
