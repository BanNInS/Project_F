
#include "Atom/Modulation/AtomModulationDestination.h"

#include "Containers/Map.h"
#include "Algo/AnyOf.h"
#include "Algo/Transform.h"
#include "Math/TransformCalculus.h"
#include "Async/Async.h"
#include "DSP/FloatArrayMath.h"

#include "Atom/Atom.h"
#include "Atom/AtomThread.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationDestination)

FAtomSoundModulationDefaultSettings::FAtomSoundModulationDefaultSettings()
{
	VolumeModulation.Value = 0.0f;
	PitchModulation.Value = 0.0f;
	HighpassModulation.Value = ATOM_MIN_FILTER_FREQUENCY;
	LowpassModulation.Value = ATOM_MAX_FILTER_FREQUENCY;
}

FAtomSoundModulationRoutingSettings::FAtomSoundModulationRoutingSettings()
	: FAtomSoundModulationDefaultSettings()
{
}

FAtomSoundModulationSettings FAtomSoundModulationDefaultSettings::GetSettingsForDestination(EAtomModulationDestination Destination) const
{
	switch (Destination)
	{
	case EAtomModulationDestination::Volume:
		return VolumeModulation;
	case EAtomModulationDestination::Pitch:
		return PitchModulation;
	case EAtomModulationDestination::Lowpass:
		return LowpassModulation;
	case EAtomModulationDestination::Highpass:
		return HighpassModulation;
	default:
		break;
	}

	return FAtomSoundModulationSettings();
}

EAtomModulationRouting FAtomSoundModulationRoutingSettings::GetRoutingForDestination(EAtomModulationDestination Destination) const
{
	switch (Destination)
	{
	case EAtomModulationDestination::Volume:
		return VolumeRouting;
	case EAtomModulationDestination::Pitch:
		return PitchRouting;
	case EAtomModulationDestination::Lowpass:
		return LowpassRouting;
	case EAtomModulationDestination::Highpass:
		return HighpassRouting;
	default:
		break;
	}
	return EAtomModulationRouting::Disable;
}

namespace Atom
{
	FModulationDestination::FModulationDestination(const FModulationDestination& InModulationDestination)
	{
		FScopeLock OtherLock(&InModulationDestination.DestinationData->HandleCritSection);
		*DestinationData = *InModulationDestination.DestinationData;
	}

	FModulationDestination::FModulationDestination(FModulationDestination&& InModulationDestination)
	{
		FScopeLock OtherLock(&InModulationDestination.DestinationData->HandleCritSection);
		*DestinationData = MoveTemp(*InModulationDestination.DestinationData);
	}

	FModulationDestination& FModulationDestination::operator=(const FModulationDestination& InModulationDestination)
	{
		*DestinationData = *(InModulationDestination.DestinationData);
		return *this;
	}

	FModulationDestination& FModulationDestination::operator=(FModulationDestination&& InModulationDestination)
	{
		*DestinationData = MoveTemp(*InModulationDestination.DestinationData);
		return *this;
	}

	void FModulationDestination::FModulationDestinationData::ResetHandles()
	{
		Atom::FModulationParameter ParameterCopy = Parameter;

		FScopeLock Lock(&HandleCritSection);
		Handles.Reset();
		Handles.Add(FModulatorHandle{ MoveTemp(ParameterCopy) });
	}

	void FModulationDestination::Init(FAtomRuntimeId InRuntimeID, bool bInIsBuffered, bool bInValueNormalized)
	{
		Init(InRuntimeID, FName(), bInIsBuffered, bInValueNormalized);
	}

	void FModulationDestination::Init(FAtomRuntimeId InRuntimeID, FName InParameterName, bool bInIsBuffered, bool bInValueNormalized)
	{
		DestinationData->RuntimeID = InRuntimeID;
		DestinationData->bIsBuffered = bInIsBuffered;
		DestinationData->bValueNormalized = bInValueNormalized;

		DestinationData->OutputBuffer.Reset();
		if (const FModulationParameter* Param = GetModulationParameterPtr(InParameterName))
		{
			DestinationData->Parameter = *Param;
		}
		else
		{
			if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
			{
				if (const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(InRuntimeID))
				{
					if (AtomRuntime->GetAtomModulationSystem())
					{
						// Even with Modulation enabled, we could not find the parameter, so likely user error
						UE_LOG(LogCriWareAtom, Warning, TEXT("Modulation Parameter %s not found while Initializing Modulation Destination. Using default parameter instead."), *InParameterName.ToString());
					}
				}
			}

			DestinationData->Parameter = Atom::GetDefaultModulationParameter();
		}

		DestinationData->ResetHandles();
	}

	bool FModulationDestination::IsActive()
	{
		FScopeLock Lock(&DestinationData->HandleCritSection);
		return Algo::AnyOf(DestinationData->Handles, [](const FModulatorHandle& Handle) { return Handle.IsValid(); });
	}

	bool FModulationDestination::ProcessControl(float InValueUnitBase, int32 InNumSamples)
	{
		FModulationParameter& Parameter = DestinationData->Parameter;
		float& ValueTarget = DestinationData->ValueTarget;
		FAlignedFloatBuffer& OutputBuffer = DestinationData->OutputBuffer;

		DestinationData->bHasProcessed = true;
		float LastTarget = ValueTarget;


		float NewTargetNormalized = Parameter.DefaultValue;
		if (Parameter.bRequiresConversion)
		{
			Parameter.NormalizedFunction(NewTargetNormalized);
		}

		FScopeLock Lock(&DestinationData->HandleCritSection);
		{
			for (const Atom::FModulatorHandle& Handle : DestinationData->Handles)
			{
				if (Handle.IsValid())
				{
					float NewHandleValue = 1.0f;
					Handle.GetValue(NewHandleValue);
					Parameter.MixFunction(NewTargetNormalized, NewHandleValue);
				}
			}
		}

		// Convert base to linear space
		float InValueBaseNormalized = InValueUnitBase;
		if (Parameter.bRequiresConversion)
		{
			Parameter.NormalizedFunction(InValueBaseNormalized);
		}

		// Mix in base value
		Parameter.MixFunction(NewTargetNormalized, InValueBaseNormalized);
		ValueTarget = NewTargetNormalized;

		// Convert target to unit space if required
		if (Parameter.bRequiresConversion && !DestinationData->bValueNormalized)
		{
			Parameter.UnitFunction(ValueTarget);
		}

		if (DestinationData->bIsBuffered)
		{
			if (OutputBuffer.Num() != InNumSamples)
			{
				OutputBuffer.Reset();
				OutputBuffer.AddZeroed(InNumSamples);
			}
		}

		// Fade from last target to new if output buffer is active
		if (!OutputBuffer.IsEmpty())
		{
			if (OutputBuffer.Num() % AUDIO_NUM_FLOATS_PER_VECTOR_REGISTER == 0)
			{
				if (FMath::IsNearlyEqual(LastTarget, ValueTarget))
				{
					Audio::ArraySetToConstantInplace(OutputBuffer, ValueTarget);
				}
				else
				{
					Audio::ArraySetToConstantInplace(OutputBuffer, 1.0f);
					Audio::ArrayFade(OutputBuffer, LastTarget, ValueTarget);
				}
			}
			else
			{
				if (FMath::IsNearlyEqual(LastTarget, ValueTarget))
				{
					OutputBuffer.Init(ValueTarget, InNumSamples);
				}
				else
				{
					float SampleValue = LastTarget;
					const float DeltaValue = (ValueTarget - LastTarget) / OutputBuffer.Num();
					for (int32 i = 0; i < OutputBuffer.Num(); ++i)
					{
						OutputBuffer[i] = SampleValue;
						SampleValue += DeltaValue;
					}
				}
			}
		}

		return !FMath::IsNearlyEqual(LastTarget, ValueTarget);
	}

	void FModulationDestination::UpdateModulators(const TSet<TObjectPtr<UAtomModulatorBase>>& InModulators)
	{
		TArray<TUniquePtr<Atom::IModulatorSettings>> ProxySettings;
		Algo::TransformIf(
			InModulators,
			ProxySettings,
			[](const UAtomModulatorBase* Mod) { return Mod != nullptr; },
			[](const UAtomModulatorBase* Mod) { return Mod->CreateProxySettings(); }
		);

		UpdateModulatorsInternal(MoveTemp(ProxySettings));
	}

	void FModulationDestination::UpdateModulators(const TSet<UAtomModulatorBase*>& InModulators)
	{
		TArray<TUniquePtr<Atom::IModulatorSettings>> ProxySettings;
		Algo::TransformIf(
			InModulators,
			ProxySettings,
			[](const UAtomModulatorBase* Mod) { return Mod != nullptr; },
			[](const UAtomModulatorBase* Mod) { return Mod->CreateProxySettings(); }
		);

		UpdateModulatorsInternal(MoveTemp(ProxySettings));
	}

	void FModulationDestination::UpdateModulators(const TSet<const UAtomModulatorBase*>& InModulators)
	{
		TArray<TUniquePtr<Atom::IModulatorSettings>> ProxySettings;
		Algo::TransformIf(
			InModulators,
			ProxySettings,
			[](const UAtomModulatorBase* Mod) { return Mod != nullptr; },
			[](const UAtomModulatorBase* Mod) { return Mod->CreateProxySettings(); }
		);

		UpdateModulatorsInternal(MoveTemp(ProxySettings));
	}

	void FModulationDestination::FModulationDestinationData::SetHandles(TSet<FModulatorHandle>&& NewHandles)
	{
		FScopeLock Lock(&HandleCritSection);
		Handles = MoveTemp(NewHandles);
	}

	FModulationDestination::FModulationDestinationData& FModulationDestination::FModulationDestinationData::operator=(const FModulationDestinationData& InDestinationInfo)
	{
		RuntimeID = InDestinationInfo.RuntimeID;
		ValueTarget = InDestinationInfo.ValueTarget;
		bIsBuffered = InDestinationInfo.bIsBuffered;
		bValueNormalized = InDestinationInfo.bValueNormalized;
		OutputBuffer = InDestinationInfo.OutputBuffer;

		TSet<FModulatorHandle> NewHandles;
		{
			FScopeLock OtherLock(&(InDestinationInfo.HandleCritSection));
			NewHandles = InDestinationInfo.Handles;
		}

		{
			FScopeLock Lock(&HandleCritSection);
			Handles = MoveTemp(NewHandles);
		}

		Parameter = InDestinationInfo.Parameter;

		return *this;
	}

	FModulationDestination::FModulationDestinationData& FModulationDestination::FModulationDestinationData::operator=(FModulationDestinationData&& InDestinationInfo)
	{
		RuntimeID = MoveTemp(InDestinationInfo.RuntimeID);
		ValueTarget = MoveTemp(InDestinationInfo.ValueTarget);
		bIsBuffered = MoveTemp(InDestinationInfo.bIsBuffered);
		bValueNormalized = MoveTemp(InDestinationInfo.bValueNormalized);
		bHasProcessed = MoveTemp(InDestinationInfo.bHasProcessed);
		OutputBuffer = MoveTemp(InDestinationInfo.OutputBuffer);

		TSet<FModulatorHandle> NewHandles;
		{
			FScopeLock OtherLock(&InDestinationInfo.HandleCritSection);
			NewHandles = MoveTemp(InDestinationInfo.Handles);
		}
		{
			FScopeLock Lock(&HandleCritSection);
			Handles = MoveTemp(NewHandles);
		}

		Parameter = MoveTemp(InDestinationInfo.Parameter);

		return *this;
	}

	const FAtomRuntimeId& FModulationDestination::FModulationDestinationData::GetRuntimeID() const
	{
		return RuntimeID;
	}

	const FModulationParameter& FModulationDestination::FModulationDestinationData::GetParameter() const
	{
		return Parameter;
	}

	void FModulationDestination::UpdateModulatorsInternal(TArray<TUniquePtr<Atom::IModulatorSettings>>&& ProxySettings)
	{
		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const FAtomRuntimeId RuntimeID = DestinationData->GetRuntimeID();
	
		FAtomThread::RunCommandOnAtomThread(
		[
			DestinationDataPtr = TWeakPtr<FModulationDestinationData>(DestinationData),
			RuntimeID = RuntimeID,
			ModSettings = MoveTemp(ProxySettings)
		]() mutable
		{
			TSharedPtr<FModulationDestinationData> DestDataPtr = DestinationDataPtr.Pin();
			if (DestDataPtr.IsValid())
			{
				if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::Get()->GetAtomRuntimeRaw(RuntimeID))
				{
					if (AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem())
					{
						TSet<FModulatorHandle> NewHandles;
						for (TUniquePtr<IModulatorSettings>& ModSetting : ModSettings)
						{
							FModulationParameter HandleParam = DestDataPtr->GetParameter();
							NewHandles.Add(FModulatorHandle{ *Modulation, *ModSetting.Get(), MoveTemp(HandleParam) });
						}

						DestDataPtr->SetHandles(MoveTemp(NewHandles));
						return;
					}
				}
				DestDataPtr->ResetHandles();
			}
		});
	}
} // namespace

namespace AtomModulation::DestinationPrivate
{
	FAtomRuntimeHandle GetAtomRuntime(const UAtomModulationDestination& InDestination)
	{
		if (GEngine && GCriWare)
		{
			if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
			{
				UWorld* World = GEngine->GetWorldFromContextObject(&InDestination, EGetWorldErrorMode::ReturnNull);
				return RuntimeManager->GetAtomRuntimeFromWorld(World);
			}
		}

		return { };
	}

	AtomModulation::FAtomModulationSystem* GetModulationSystem(const UAtomModulationDestination& InDestination)
	{
		FAtomRuntimeHandle AtomRuntime = DestinationPrivate::GetAtomRuntime(InDestination);

		if (AtomRuntime.IsValid())
		{
			if (AtomModulation::FAtomModulationSystem* ModSystem = AtomRuntime->GetAtomModulationSystem())
			{
				return ModSystem;
			}
		}

		return nullptr;
	}
} // namespace

bool UAtomModulationDestination::ClearModulator()
{
	if (Modulator)
	{
		Modulator = nullptr;
		Destination.UpdateModulators(TSet<const UAtomModulatorBase*>{ });
		return true;
	}

	return false;
}

const UAtomModulatorBase* UAtomModulationDestination::GetModulator() const
{
	return Modulator;
}

float UAtomModulationDestination::GetValue() const
{
	using namespace AtomModulation;

	if (Modulator)
	{
		if (AtomModulation::FAtomModulationSystem* ModSystem = DestinationPrivate::GetModulationSystem(*this))
		{
			float OutValue = 1.0f;
			ModSystem->GetModulatorValueThreadSafe(Modulator->GetUniqueID(), OutValue);
			return OutValue;
		}
	}

	return 1.0f;
}

void UAtomModulationDestination::PostInitProperties()
{
	using namespace AtomModulation;

	Super::PostInitProperties();

	if (UAtomModulationDestination::StaticClass()->GetDefaultObject() == this)
	{
		return;
	}

	FAtomRuntimeHandle AtomRuntime = DestinationPrivate::GetAtomRuntime(*this);
	if (AtomRuntime.IsValid())
	{
		constexpr bool bIsBuffered = false;
		Destination.Init(AtomRuntime.GetRuntimeID(), bIsBuffered);
	}
}

bool UAtomModulationDestination::SetModulator(const UAtomModulatorBase* InModulator)
{
	if (InModulator == Modulator)
	{
		return true;
	}

	if (InModulator)
	{
		Modulator = InModulator;
		Destination.UpdateModulators(TSet<const UAtomModulatorBase*>{ InModulator });
		return true;
	}

	return ClearModulator();
}
