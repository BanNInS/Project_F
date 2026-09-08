
#include "Atom/AtomVirtualLoop.h"

#include "Atom/AtomActiveSound.h"
#include "Atom/AtomDebug.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundBase.h"

static int32 bAtomVirtualLoopsEnabledCVar = 1;
FAutoConsoleVariableRef CVarAtomVirtualLoopsEnabled(
	TEXT("atom.VirtualLoops.Enabled"),
	bAtomVirtualLoopsEnabledCVar,
	TEXT("Enables or disables whether virtualizing is supported for audio loops.\n"),
	ECVF_Default);

static float AtomVirtualLoopsPerfDistanceCVar = 15000.0f;
FAutoConsoleVariableRef CVarAtomVirtualLoopsPerfDistance(
	TEXT("atom.VirtualLoops.PerfDistance"),
	AtomVirtualLoopsPerfDistanceCVar,
	TEXT("Sets virtual loop distance to scale update rate between min and max beyond max audible distance of sound.\n"),
	ECVF_Default);

static float AtomVirtualLoopsForceUpdateListenerMoveDistanceCVar = 2500.0f;
FAutoConsoleVariableRef CVarAtomVirtualLoopsForceUpdateListenerMoveDistance(
	TEXT("atom.VirtualLoops.ForceUpdateListenerMoveDistance"),
	AtomVirtualLoopsForceUpdateListenerMoveDistanceCVar,
	TEXT("Sets distance threshold required to force an update on virtualized sounds to check for if listener moves in a single frame over the given distance.\n"),
	ECVF_Default);

static float AtomVirtualLoopsUpdateRateMinCVar = 0.1f;
FAutoConsoleVariableRef CVarAtomVirtualLoopsUpdateRateMin(
	TEXT("atom.VirtualLoops.UpdateRate.Min"),
	AtomVirtualLoopsUpdateRateMinCVar,
	TEXT("Sets minimum rate to check if sound becomes audible again at sound's max audible distance.\n"),
	ECVF_Default);

static float AtomVirtualLoopsUpdateRateMaxCVar = 3.0f;
FAutoConsoleVariableRef CVarAtomVirtualLoopsUpdateRateMax(
	TEXT("atom.VirtualLoops.UpdateRate.Max"),
	AtomVirtualLoopsUpdateRateMaxCVar,
	TEXT("Sets maximum rate to check if sound becomes audible again (at beyond sound's max audible distance + perf scaling distance).\n"),
	ECVF_Default);

#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN(CriWareAtom, VirtualLoopVirtualize)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(uint64, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(uint64, ComponentId)
	UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, VirtualLoopUpdate)
	UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
	UE_TRACE_EVENT_FIELD(double, Timestamp)
	UE_TRACE_EVENT_FIELD(uint32, PlayOrder)
	UE_TRACE_EVENT_FIELD(float, TimeVirtualized)
	UE_TRACE_EVENT_FIELD(float, PlaybackTime)
	UE_TRACE_EVENT_FIELD(float, UpdateInterval)
	UE_TRACE_EVENT_FIELD(double, LocationX)
	UE_TRACE_EVENT_FIELD(double, LocationY)
	UE_TRACE_EVENT_FIELD(double, LocationZ)
	UE_TRACE_EVENT_FIELD(double, RotatorPitch)
	UE_TRACE_EVENT_FIELD(double, RotatorYaw)
	UE_TRACE_EVENT_FIELD(double, RotatorRoll)
UE_TRACE_EVENT_END()
#endif // ATOM_PROFILERTRACE_ENABLED

FAtomVirtualLoop::FAtomVirtualLoop()
	: TimeSinceLastUpdate(0.0f)
	, TimeVirtualized(0.0f)
	, UpdateInterval(0.0f)
	, ActiveSound(nullptr)
{
}

bool FAtomVirtualLoop::Virtualize(FAtomActiveSound& InActiveSound, bool bDoRangeCheck, FAtomVirtualLoop& OutVirtualLoop)
{
	FAtomRuntime* AtomRuntime = InActiveSound.GetAtomRuntime();
	check(AtomRuntime);

	return Virtualize(InActiveSound, *AtomRuntime, bDoRangeCheck, OutVirtualLoop);
}

bool FAtomVirtualLoop::Virtualize(FAtomActiveSound& InActiveSound, FAtomRuntime& InAtomRuntime, bool bDoRangeCheck, FAtomVirtualLoop& OutVirtualLoop)
{
	UAtomSoundBase* Sound = InActiveSound.GetSound();
	check(Sound);

	const EAtomVirtualizationMode VirtualizationMode = Sound->GetVirtualizationMode();
	if (VirtualizationMode == EAtomVirtualizationMode::Normal
		|| VirtualizationMode == EAtomVirtualizationMode::StopWhenSilent)
	{
		return false;
	}

	if (!bAtomVirtualLoopsEnabledCVar || InActiveSound.bIsPreviewSound /*|| !InActiveSound.IsLooping()*/)
	{
		return false;
	}

	if ((InActiveSound.FadeOut != FAtomActiveSound::EFadeOut::None && InActiveSound.FadeOut != FAtomActiveSound::EFadeOut::Concurrency) || InActiveSound.bIsStopping)
	{
		return false;
	}

	/*if (InAtomRuntime.CanHaveMultipleActiveSounds(InActiveSound.GetAtomComponentID()))
	{
		return false;
	}*/

	if (bDoRangeCheck && IsInAudibleRange(InActiveSound, &InAtomRuntime))
	{
		return false;
	}

	if (!InActiveSound.CanVirtualizeWhenSilent())
	{
		// Create a virtual copy (original active sound will be stopped)
		FAtomActiveSound* ActiveSound = FAtomActiveSound::CreateVirtualCopy(InActiveSound, InAtomRuntime);
		OutVirtualLoop.ActiveSound = ActiveSound;
	}
	else
	{
		// Set active sound as virtualized
		InActiveSound.SetVirtualized(true);
		OutVirtualLoop.ActiveSound = &InActiveSound;
	}

	OutVirtualLoop.CalculateUpdateInterval();
	
#if ATOM_PROFILERTRACE_ENABLED
	const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
	if (bChannelEnabled)
	{
		const FAtomActiveSound* ActiveSound = OutVirtualLoop.ActiveSound;
		if (const FAtomRuntime* AtomRuntime = ActiveSound ? ActiveSound->GetAtomRuntime() : nullptr)
		{
			UE_TRACE_LOG(CriWareAtom, VirtualLoopVirtualize, AtomChannel)
				<< VirtualLoopVirtualize.RuntimeID(static_cast<uint32>(AtomRuntime->GetAtomRuntimeID()))
				<< VirtualLoopVirtualize.Timestamp(FPlatformTime::Cycles64())
				<< VirtualLoopVirtualize.PlayOrder(ActiveSound->GetPlayOrder())
				<< VirtualLoopVirtualize.ComponentId(ActiveSound->GetAtomComponentID())
				<< VirtualLoopVirtualize.Name(Sound ? *Sound->GetPathName() : TEXT("N/A"));
		}
	}
#endif // ATOM_PROFILERTRACE_ENABLED
	return true;
}

void FAtomVirtualLoop::CalculateUpdateInterval()
{
	check(ActiveSound);
	FAtomRuntime* AtomRuntime = ActiveSound->GetAtomRuntime();
	check(AtomRuntime);

	const float DistanceToListener = AtomRuntime->GetDistanceToNearestListener(ActiveSound->Transform.GetLocation());
	const float DistanceRatio = (DistanceToListener - ActiveSound->MaxDistance) / FMath::Max(AtomVirtualLoopsPerfDistanceCVar, 1.0f);
	const float DistanceRatioClamped = FMath::Clamp(DistanceRatio, 0.0f, 1.0f);
	UpdateInterval = FMath::Lerp(AtomVirtualLoopsUpdateRateMinCVar, AtomVirtualLoopsUpdateRateMaxCVar, DistanceRatioClamped);
}

float FAtomVirtualLoop::GetTimeVirtualized() const
{
	return TimeVirtualized;
}

float FAtomVirtualLoop::GetUpdateInterval() const
{
	return UpdateInterval;
}

FAtomActiveSound& FAtomVirtualLoop::GetActiveSound()
{
	check(ActiveSound);
	return *ActiveSound;
}

const FAtomActiveSound& FAtomVirtualLoop::GetActiveSound() const
{
	check(ActiveSound);
	return *ActiveSound;
}

bool FAtomVirtualLoop::IsEnabled()
{
	return bAtomVirtualLoopsEnabledCVar != 0;
}

bool FAtomVirtualLoop::IsInAudibleRange(const FAtomActiveSound& InActiveSound, const FAtomRuntime* InAtomRuntime)
{
	if (!InActiveSound.bAllowSpatialization)
	{
		return true;
	}

	const FAtomRuntime* AtomRuntime = InAtomRuntime;
	if (!AtomRuntime)
	{
		AtomRuntime = InActiveSound.GetAtomRuntime();
	}
	check(AtomRuntime);

	if (InActiveSound.IsPlayWhenSilent())
	{
		return true;
	}

	float DistanceScale = 1.0f;
	if (InActiveSound.bHasAttenuationSettings)
	{
		// If we are not using distance-based attenuation, this sound will be audible regardless of distance.
		if (!InActiveSound.AttenuationSettings.bAttenuate)
		{
			return true;
		}

		DistanceScale = InActiveSound.FocusData.DistanceScale;
	}

	DistanceScale = FMath::Max(DistanceScale, UE_KINDA_SMALL_NUMBER);
	const FVector Location = InActiveSound.Transform.GetLocation();
	return AtomRuntime->LocationIsAudible(Location, InActiveSound.MaxDistance / DistanceScale);
}

void FAtomVirtualLoop::UpdateFocusData(float DeltaTime)
{
	check(ActiveSound);

	if (!ActiveSound->bHasAttenuationSettings)
	{
		return;
	}

	// If we are not using distance-based attenuation, this sound will be audible regardless of distance.
	if (!ActiveSound->AttenuationSettings.bAttenuate)
	{
		return;
	}

	check(ActiveSound->GetAtomRuntime());
	const FAtomRuntime& AtomRuntime = *ActiveSound->GetAtomRuntime();
	const int32 ClosestListenerIndex = AtomRuntime.FindClosestListenerIndex(ActiveSound->Transform);

	FAtomAttenuationListenerData ListenerData = FAtomAttenuationListenerData::Create(AtomRuntime, ClosestListenerIndex, ActiveSound->Transform, ActiveSound->AttenuationSettings);
	ActiveSound->UpdateFocusData(DeltaTime, ListenerData);
}

bool FAtomVirtualLoop::Update(float DeltaTime, bool bForceUpdate)
{
	// Keep playback time up-to-date as it may be used to evaluate whether or
	// not virtual sound is eligible for playback when compared against
	// actively playing sounds in concurrency checks.
	const float DeltaTimePitchCorrected = DeltaTime * ActiveSound->MinCurrentPitch;
	ActiveSound->PlaybackTime += DeltaTimePitchCorrected;
	TimeVirtualized += DeltaTimePitchCorrected;

	const float UpdateDelta = TimeSinceLastUpdate + DeltaTime;
	if (bForceUpdate)
	{
		TimeSinceLastUpdate = 0.0f;
	}
	else if (UpdateInterval > 0.0f)
	{
		TimeSinceLastUpdate = UpdateDelta;
		if (UpdateInterval > TimeSinceLastUpdate)
		{
			return false;
		}
		TimeSinceLastUpdate = 0.0f;
	}

#if ENABLE_ATOM_DEBUG
	Atom::FAtomDebugger::DrawDebugInfo(*this);
#endif // ENABLE_ATOM_DEBUG

#if ATOM_PROFILERTRACE_ENABLED
	const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
	if (bChannelEnabled)
	{
		if (const FAtomRuntime* AtomRuntime = ActiveSound->GetAtomRuntime())
		{
			const UAtomSoundBase* Sound = ActiveSound->GetSound();

			const FTransform& Transform = ActiveSound->Transform;
			const FVector& Location = Transform.GetLocation();
			const FRotator Rotator = Transform.GetRotation().Rotator();
			UE_TRACE_LOG(CriWareAtom, VirtualLoopUpdate, AtomChannel)
				<< VirtualLoopUpdate.RuntimeID(static_cast<uint32>(AtomRuntime->GetAtomRuntimeID()))
				<< VirtualLoopUpdate.Timestamp(FPlatformTime::Cycles64())
				<< VirtualLoopUpdate.PlayOrder(ActiveSound->GetPlayOrder())
				<< VirtualLoopUpdate.TimeVirtualized(TimeVirtualized)
				<< VirtualLoopUpdate.PlaybackTime(ActiveSound->PlaybackTime)
				<< VirtualLoopUpdate.UpdateInterval(UpdateInterval)
				<< VirtualLoopUpdate.LocationX(Location.X)
				<< VirtualLoopUpdate.LocationY(Location.Y)
				<< VirtualLoopUpdate.LocationZ(Location.Z)
				<< VirtualLoopUpdate.RotatorPitch(Rotator.Pitch)
				<< VirtualLoopUpdate.RotatorYaw(Rotator.Yaw)
				<< VirtualLoopUpdate.RotatorRoll(Rotator.Roll);
		}
	}
#endif // ATOM_PROFILERTRACE_ENABLED

	UpdateFocusData(UpdateDelta);

	// if sound was virtualized by Atom SDK, audiblity when will be checked by Atom SDK, so return false here.
	if (!ActiveSound->bVirtualizedDueToMaxConcurrency && ActiveSound->CanVirtualizeWhenSilent())
	{
		return false;
	}

	// If not audible, update when will be checked again and return false
	if (!IsInAudibleRange(*ActiveSound))
	{
		CalculateUpdateInterval();
		return false;
	}

	return true;
}

#if ATOM_PROFILERTRACE_ENABLED
void FAtomVirtualLoop::OnTraceStarted()
{
	// Make sure we send virtual loop data to Audio Insights if it's opened in the middle of a PIE session
	const bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(AtomChannel);
	if (bChannelEnabled && ActiveSound)
	{
		check(ActiveSound);
		const UAtomSoundBase* Sound = ActiveSound->GetSound();
		check(Sound);

		if (const FAtomRuntime* AtomRuntme = ActiveSound->GetAtomRuntime())
		{
			UE_TRACE_LOG(CriWareAtom, VirtualLoopVirtualize, AtomChannel)
				<< VirtualLoopVirtualize.RuntimeID(static_cast<uint32>(AtomRuntme->GetAtomRuntimeID()))
				<< VirtualLoopVirtualize.Timestamp(FPlatformTime::Cycles64())
				<< VirtualLoopVirtualize.PlayOrder(ActiveSound->GetPlayOrder())
				<< VirtualLoopVirtualize.ComponentId(ActiveSound->GetAtomComponentID())
				<< VirtualLoopVirtualize.Name(Sound ? *Sound->GetPathName() : TEXT("N/A"));
		}
	}
}
#endif

bool FAtomVirtualLoop::ShouldListenerMoveForceUpdate(const FTransform& LastTransform, const FTransform& CurrentTransform)
{
	const float DistanceSq = FVector::DistSquared(LastTransform.GetTranslation(), CurrentTransform.GetTranslation());
	const float ForceUpdateDistSq = AtomVirtualLoopsForceUpdateListenerMoveDistanceCVar * AtomVirtualLoopsForceUpdateListenerMoveDistanceCVar;
	return DistanceSq > ForceUpdateDistSq;
}

bool FAtomVirtualLoop::IsUsingVirtualVoice() const
{
	return ActiveSound && ActiveSound->CanVirtualizeWhenSilent();
}

bool FAtomVirtualLoop::IsVirtualizedDueToConcurrency() const
{
	return ActiveSound && ActiveSound->bVirtualizedDueToMaxConcurrency;
}
