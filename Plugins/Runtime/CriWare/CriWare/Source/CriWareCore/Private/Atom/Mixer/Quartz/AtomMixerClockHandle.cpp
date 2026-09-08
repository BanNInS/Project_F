
#include "Atom/Mixer/Quartz/AtomMixerClockHandle.h"

#include "Engine/GameInstance.h"

#include "Atom/Mixer/Quartz/AtomMixerQuantizedCommands.h"
#include "Atom/AtomQuartzQuantizationUtilities.h"
#include "Atom/AtomRuntime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomMixerClockHandle)

// Clock Handle implementation
UAtomQuartzClockHandle::UAtomQuartzClockHandle()
{
}

UAtomQuartzClockHandle::~UAtomQuartzClockHandle()
{
}

void UAtomQuartzClockHandle::BeginDestroy()
{
	Super::BeginDestroy();

	auto Subscriber = GetQuartzSubscriber();
	RawHandle.SendCommandToClock([Subscriber](Atom::FQuartzClock* InClock) { InClock->UnsubscribeFromAllTimeDivisions(Subscriber); });
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
bool UAtomQuartzClockHandle::ShouldUnsubscribe()
{
	return !RawHandle.IsValid();
}
#endif

void UAtomQuartzClockHandle::StartClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;
	ResumeClock(WorldContextObject, ClockHandle);
}

void UAtomQuartzClockHandle::StopClock(const UObject* WorldContextObject, bool bCancelPendingEvents, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;
	RawHandle.SendCommandToClock([bCancelPendingEvents](Atom::FQuartzClock* InClock) { InClock->Stop(bCancelPendingEvents); });
}

void UAtomQuartzClockHandle::PauseClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;
	RawHandle.SendCommandToClock([](Atom::FQuartzClock* InClock) { InClock->Pause(); });
}

// Begin BP interface
void UAtomQuartzClockHandle::ResumeClock(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;
	RawHandle.SendCommandToClock([](Atom::FQuartzClock* InClock) { InClock->Resume(); });
}

void UAtomQuartzClockHandle::QueueQuantizedSound(const UObject* WorldContextObject, UAtomQuartzClockHandle*& InClockHandle, const FAtomComponentCommandInfo& InAtomComponentData, const FOnQuartzCommandEventBP& InDelegate, const FQuartzQuantizationBoundary& InTargetBoundary)
{
	InClockHandle = this;
	FName ClockName = GetClockName();

	//Create a Queue Command, and give it the additional data that it needs
	TSharedPtr<Atom::FQuantizedQueueCommand> QueueCommandPtr = MakeShared<Atom::FQuantizedQueueCommand>();
	QueueCommandPtr->SetQueueCommand(InAtomComponentData);

	//Set up initial command info
	Atom::FQuartzQuantizedRequestData CommandInitInfo = UAtomQuartzSubsystem::CreateRequestDataForSchedulePlaySound(InClockHandle, InDelegate, InTargetBoundary);

	//(Queue's setup is identical to PlaySound except for the command ptr, so fix that here)
	CommandInitInfo.QuantizedCommandPtr.Reset();
	CommandInitInfo.QuantizedCommandPtr = QueueCommandPtr;

	RawHandle.SendCommandToClock([CommandInitInfo](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(CommandInitInfo); });
}

// deprecated: use ResetTransportQuantized
void UAtomQuartzClockHandle::ResetTransport(const UObject* WorldContextObject, const FOnQuartzCommandEventBP& InDelegate)
{
	Atom::FQuartzQuantizedRequestData Data(UAtomQuartzSubsystem::CreateRequestDataForTransportReset(this, FQuartzQuantizationBoundary(EQuartzCommandQuantization::Bar), InDelegate));
	RawHandle.SendCommandToClock([Data](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(Data); });
}

void UAtomQuartzClockHandle::ResetTransportQuantized(const UObject* WorldContextObject, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;
	Atom::FQuartzQuantizedRequestData Data(UAtomQuartzSubsystem::CreateRequestDataForTransportReset(this, InQuantizationBoundary, InDelegate));
	RawHandle.SendCommandToClock([Data](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(Data); });
}

bool UAtomQuartzClockHandle::IsClockRunning(const UObject* WorldContextObject)
{
	return RawHandle.IsClockRunning();
}

void UAtomQuartzClockHandle::NotifyOnQuantizationBoundary(const UObject* WorldContextObject, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, float OffsetInMilliseconds)
{
	Atom::FQuartzQuantizedRequestData Data(UAtomQuartzSubsystem::CreateRequestDataForQuantizedNotify(this, InQuantizationBoundary, InDelegate, OffsetInMilliseconds));
	RawHandle.SendCommandToClock([Data](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(Data); });
}

float UAtomQuartzClockHandle::GetDurationOfQuantizationTypeInSeconds(const UObject* WorldContextObject, const EQuartzCommandQuantization& QuantizationType, float Multiplier)
{
	return RawHandle.GetDurationOfQuantizationTypeInSeconds(QuantizationType, Multiplier);
}

FQuartzTransportTimeStamp UAtomQuartzClockHandle::GetCurrentTimestamp(const UObject* WorldContextObject)
{
	return RawHandle.GetCurrentClockTimestamp();
}

float UAtomQuartzClockHandle::GetEstimatedRunTime(const UObject* WorldContextObject)
{
	return RawHandle.GetEstimatedClockRunTimeSeconds();
}

void UAtomQuartzClockHandle::StartOtherClock(const UObject* WorldContextObject, FName OtherClockName, FQuartzQuantizationBoundary InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate)
{
	if (OtherClockName == CurrentClockId)
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Clock: (%s) is attempting to start itself on a quantization boundary.  Ignoring command"), *CurrentClockId.ToString());
		return;
	}

	Atom::FQuartzQuantizedRequestData Data(UAtomQuartzSubsystem::CreateRequestDataForStartOtherClock(this, OtherClockName, InQuantizationBoundary, InDelegate));
	RawHandle.SendCommandToClock([Data](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(Data); });
}

// todo: Move the bulk of these functions to FQuartzTickableObject once lightweight clock handles are spun up.
void UAtomQuartzClockHandle::SubscribeToQuantizationEvent(const UObject* WorldContextObject, EQuartzCommandQuantization InQuantizationBoundary, const FOnQuartzMetronomeEventBP& OnQuantizationEvent, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;

	if (InQuantizationBoundary == EQuartzCommandQuantization::None)
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Clock: (%s) is attempting to subscribe to 'NONE' as a Quantization Boundary.  Ignoring request"), *CurrentClockId.ToString());
		return;
	}

	AddMetronomeBpDelegate(InQuantizationBoundary, OnQuantizationEvent);

	auto Subscriber = GetQuartzSubscriber();
	RawHandle.SendCommandToClock([Subscriber, InQuantizationBoundary](Atom::FQuartzClock* InClock) { InClock->SubscribeToTimeDivision(Subscriber, InQuantizationBoundary); });
}

void UAtomQuartzClockHandle::SubscribeToAllQuantizationEvents(const UObject* WorldContextObject, const FOnQuartzMetronomeEventBP& OnQuantizationEvent, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;

	for (int32 i = 0; i < static_cast<int32>(EQuartzCommandQuantization::Count) - 1; ++i)
	{
		AddMetronomeBpDelegate(static_cast<EQuartzCommandQuantization>(i), OnQuantizationEvent);
	}

	auto Subscriber = GetQuartzSubscriber();
	RawHandle.SendCommandToClock([Subscriber](Atom::FQuartzClock* InClock) { InClock->SubscribeToAllTimeDivisions(Subscriber); });
}

void UAtomQuartzClockHandle::UnsubscribeFromTimeDivision(const UObject* WorldContextObject, EQuartzCommandQuantization InQuantizationBoundary, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;

	auto Subscriber = GetQuartzSubscriber();
	RawHandle.SendCommandToClock([Subscriber, InQuantizationBoundary](Atom::FQuartzClock* InClock) { InClock->UnsubscribeFromTimeDivision(Subscriber, InQuantizationBoundary); });
}

void UAtomQuartzClockHandle::UnsubscribeFromAllTimeDivisions(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle)
{
	ClockHandle = this;

	auto Subscriber = GetQuartzSubscriber();
	RawHandle.SendCommandToClock([Subscriber](Atom::FQuartzClock* InClock) { InClock->UnsubscribeFromAllTimeDivisions(Subscriber); });
}

// Metronome Alteration (setters)
void UAtomQuartzClockHandle::SetMillisecondsPerTick(const UObject* WorldContextObject, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle, float MillisecondsPerTick)
{
	ClockHandle = this;
	if (MillisecondsPerTick < 0 || FMath::IsNearlyZero(MillisecondsPerTick))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Ignoring invalid request on Clock: %s: MillisecondsPerTick was %f"), *this->CurrentClockId.ToString(), MillisecondsPerTick);
		return;
	}

	Audio::FQuartzClockTickRate TickRate;
	TickRate.SetMillisecondsPerTick(MillisecondsPerTick);
	SetTickRateInternal(InQuantizationBoundary, InDelegate, TickRate);
}

void UAtomQuartzClockHandle::SetTicksPerSecond(const UObject* WorldContextObject, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle, float TicksPerSecond)
{
	ClockHandle = this;
	if (TicksPerSecond < 0 || FMath::IsNearlyZero(TicksPerSecond))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Ignoring invalid request on Clock: %s: TicksPerSecond was %f"), *this->CurrentClockId.ToString(), TicksPerSecond);
		return;
	}

	Audio::FQuartzClockTickRate TickRate;
	TickRate.SetSecondsPerTick(1.f / TicksPerSecond);
	SetTickRateInternal(InQuantizationBoundary, InDelegate, TickRate);
}

void UAtomQuartzClockHandle::SetSecondsPerTick(const UObject* WorldContextObject, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle, float SecondsPerTick)
{
	ClockHandle = this;
	if (SecondsPerTick < 0 || FMath::IsNearlyZero(SecondsPerTick))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Ignoring invalid request on Clock: %s: SecondsPerTick was %f"), *this->CurrentClockId.ToString(), SecondsPerTick);
		return;
	}

	Audio::FQuartzClockTickRate TickRate;
	TickRate.SetSecondsPerTick(SecondsPerTick);
	SetTickRateInternal(InQuantizationBoundary, InDelegate, TickRate);
}

void UAtomQuartzClockHandle::SetThirtySecondNotesPerMinute(const UObject* WorldContextObject, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle, float ThirtySecondsNotesPerMinute)
{
	ClockHandle = this;
	if (ThirtySecondsNotesPerMinute < 0 || FMath::IsNearlyZero(ThirtySecondsNotesPerMinute))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Ignoring invalid request on Clock: %s: ThirtySecondsNotesPerMinute was %f"), *this->CurrentClockId.ToString(), ThirtySecondsNotesPerMinute);
		return;
	}

	Audio::FQuartzClockTickRate TickRate;
	TickRate.SetThirtySecondNotesPerMinute(ThirtySecondsNotesPerMinute);
	SetTickRateInternal(InQuantizationBoundary, InDelegate, TickRate);
}

void UAtomQuartzClockHandle::SetBeatsPerMinute(const UObject* WorldContextObject, const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, UAtomQuartzClockHandle*& ClockHandle, float BeatsPerMinute)
{
	ClockHandle = this;
	if (BeatsPerMinute < 0 || FMath::IsNearlyZero(BeatsPerMinute))
	{
		UE_LOG(LogAudioQuartz, Warning, TEXT("Ignoring invalid request on Clock: %s: BeatsPerMinute was %f"), *this->CurrentClockId.ToString(), BeatsPerMinute);
		return;
	}

	Audio::FQuartzClockTickRate TickRate;
	TickRate.SetBeatsPerMinute(BeatsPerMinute);
	SetTickRateInternal(InQuantizationBoundary, InDelegate, TickRate);
}

void UAtomQuartzClockHandle::SetTickRateInternal(const FQuartzQuantizationBoundary& InQuantizationBoundary, const FOnQuartzCommandEventBP& InDelegate, const Audio::FQuartzClockTickRate& NewTickRate)
{
	Atom::FQuartzQuantizedRequestData Data(UAtomQuartzSubsystem::CreateRequestDataForTickRateChange(this, InDelegate, NewTickRate, InQuantizationBoundary));
	RawHandle.SendCommandToClock([Data](Atom::FQuartzClock* InClock) mutable { InClock->AddQuantizedCommand(Data); });
}

// Metronome getters
float UAtomQuartzClockHandle::GetMillisecondsPerTick(const UObject* WorldContextObject) const
{
	Audio::FQuartzClockTickRate OutTickRate;

	if (GetCurrentTickRate(WorldContextObject, OutTickRate))
	{
		return OutTickRate.GetMillisecondsPerTick();
	}

	return 0.f;
}

float UAtomQuartzClockHandle::GetTicksPerSecond(const UObject* WorldContextObject) const
{
	Audio::FQuartzClockTickRate OutTickRate;

	if (GetCurrentTickRate(WorldContextObject, OutTickRate))
	{
		const float SecondsPerTick = OutTickRate.GetSecondsPerTick();

		if (!FMath::IsNearlyZero(SecondsPerTick))
		{
			return 1.f / SecondsPerTick;
		}
	}

	return 0.f;
}

float UAtomQuartzClockHandle::GetSecondsPerTick(const UObject* WorldContextObject) const
{
	Audio::FQuartzClockTickRate OutTickRate;

	if (GetCurrentTickRate(WorldContextObject, OutTickRate))
	{
		return OutTickRate.GetSecondsPerTick();
	}

	return 0.f;
}

float UAtomQuartzClockHandle::GetThirtySecondNotesPerMinute(const UObject* WorldContextObject) const
{
	Audio::FQuartzClockTickRate OutTickRate;

	if (GetCurrentTickRate(WorldContextObject, OutTickRate))
	{
		return OutTickRate.GetThirtySecondNotesPerMinute();
	}

	return 0.f;
}

float UAtomQuartzClockHandle::GetBeatsPerMinute(const UObject* WorldContextObject) const
{
	Audio::FQuartzClockTickRate OutTickRate;

	if (GetCurrentTickRate(WorldContextObject, OutTickRate))
	{
		return OutTickRate.GetBeatsPerMinute();
	}

	return 0.f;
}

float UAtomQuartzClockHandle::GetBeatProgressPercent(EQuartzCommandQuantization QuantizationBoundary, float PhaseOffset, float MsOffset)
{
	if(RawHandle.IsValid() && QuantizationBoundary != EQuartzCommandQuantization::None)
	{
		constexpr float ToMilliseconds = 1000.f;
	    const float MsInQuantizationType = ToMilliseconds * RawHandle.GetDurationOfQuantizationTypeInSeconds(QuantizationBoundary, 1.f);
	    if(!FMath::IsNearlyZero(MsInQuantizationType))
	    {
		    PhaseOffset += MsOffset / MsInQuantizationType;
	    }

		return FMath::Wrap(PhaseOffset + RawHandle.GetBeatProgressPercent(QuantizationBoundary), 0.f, 1.f);
	}

	return 0.f;
}

// todo: un-comment when metronome events support the offset
// void UAtomQuartzClockHandle::SetNotificationAnticipationAmountInMilliseconds(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle, const double Milliseconds)
// {
// 	ClockHandle = this;
// 	if(Milliseconds < 0.0)
// 	{
// 		UE_LOG(LogAudioQuartz, Warning, TEXT("Setting a negative notification anticipation amount is not supported. (request ignored)"));
// 		return;
// 	}
//
// 	SetNotificationAnticipationAmountMilliseconds(Milliseconds);
// }
//
//
// void UAtomQuartzClockHandle::SetNotificationAnticipationAmountAsMusicalDuration(const UObject* WorldContextObject, UAtomQuartzClockHandle*& ClockHandle, const EQuartzCommandQuantization MusicalDuration, const double Multiplier)
// {
// 	ClockHandle = this;
// 	if(Multiplier < 0.0)
// 	{
// 		UE_LOG(LogAudioQuartz, Warning, TEXT("Setting a negative notification anticipation amount is not supported. (request ignored)"));
// 		return;
// 	}
//
// 	SetNotificationAnticipationAmountMusicalDuration(MusicalDuration, Multiplier);
// }

// End BP interface

UAtomQuartzClockHandle* UAtomQuartzClockHandle::SubscribeToClock(const UObject* WorldContextObject, FName ClockName, Atom::FQuartzClockProxy const* InHandlePtr)
{
	CurrentClockId = ClockName;

	if (InHandlePtr)
	{
		RawHandle = *InHandlePtr;
	}

	return this;
}

// returns true if OutTickRate is valid and was updated
bool UAtomQuartzClockHandle::GetCurrentTickRate(const UObject* WorldContextObject, Audio::FQuartzClockTickRate& OutTickRate) const
{
	if (RawHandle.IsValid())
	{
		OutTickRate = RawHandle.GetTickRate();
		return true;
	}

	OutTickRate = {};
	return false;
}

