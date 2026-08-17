
#include "Atom/Modulation/AtomModulationControlBus.h"

#include "Engine/World.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationLogs.h"
//#include "Atom/Modulation/AtomModulationStatics.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationControlBusProxy.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationControlBus)

UAtomModulationControlBus::UAtomModulationControlBus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bBypass(false)
#if WITH_EDITORONLY_DATA
	, bOverrideAddress(false)
#endif // WITH_EDITORONLY_DATA
	, Parameter(nullptr)
{
}

TUniquePtr<Atom::IModulatorSettings> UAtomModulationControlBus::CreateProxySettings() const
{
	return TUniquePtr<Atom::IModulatorSettings>(new AtomModulation::FControlBusSettings(*this));
}

#if WITH_EDITOR
void UAtomModulationControlBus::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	if (!bOverrideAddress)
	{
		Address = GetName();
	}

	Super::PostDuplicate(DuplicateMode);
}

void UAtomModulationControlBus::PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
{
	if (FProperty* Property = InPropertyChangedEvent.Property)
	{
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomModulationControlBus, bOverrideAddress) && !bOverrideAddress)
		{
			Address = GetName();
		}

		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UAtomModulationControlBus, Parameter))
		{
			// was disabled ??
			for (TObjectIterator<UAtomModulationControlBusMix> Iter; Iter; ++Iter)
			{
				if (UAtomModulationControlBusMix* Mix = *Iter)
				{
					for (FAtomModulationControlBusMixStage& Stage : Mix->MixStages)
					{
						if (Stage.Bus == this)
						{
							float UnitValue = Stage.Value.TargetValue;
							if (Parameter)
							{
								UnitValue = Parameter->ConvertNormalizedToUnit(Stage.Value.TargetValue);
							}

							if (!FMath::IsNearlyEqual(Stage.Value.TargetUnitValue, UnitValue, KINDA_SMALL_NUMBER))
							{
								Stage.Value.TargetUnitValue = UnitValue;
							}
						}
					}
				}
			}
			// ~~ 
		}

		Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModSystem)
		{
			OutModSystem.UpdateModulator(*this);
		});
	}

	Super::PostEditChangeProperty(InPropertyChangedEvent);
}

void UAtomModulationControlBus::PostInitProperties()
{
	if (!bOverrideAddress)
	{
		Address = GetName();
	}

	Super::PostInitProperties();
}

void UAtomModulationControlBus::PostRename(UObject* OldOuter, const FName OldName)
{
	if (!bOverrideAddress)
	{
		Address = GetName();
	}
}
#endif // WITH_EDITOR

void UAtomModulationControlBus::BeginDestroy()
{
	using namespace AtomModulation;

	if (UWorld* World = GetWorld())
	{
		FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World);
		if (AtomRuntime.IsValid())
		{
			FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem();
			check(Modulation);
			if (Modulation)
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				Modulation->DeactivateBus(*this);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}
		}
	}

	// Call parent destroy at end to ensure object is in a valid state for the modulation manager to clean up first
	Super::BeginDestroy();
}

const Atom::FModulationMixFunction UAtomModulationControlBus::GetMixFunction() const
{
	if (Parameter)
	{
		return Parameter->GetMixFunction();
	}

	return Atom::FModulationParameter::GetDefaultMixFunction();
}

//TUniquePtr<Atom::IProxyData> UAtomModulationControlBus::CreateNewProxyData(const Atom::FProxyDataInitParams& InitParams)
//{
//	using namespace AtomModulation;
//	return MakeShared<FAtomModulatorAssetProxy>(*this);
//}

const Atom::FModulationParameter& UAtomModulationControlBus::GetOutputParameter() const
{
#if !UE_BUILD_SHIPPING
	return AtomModulation::GetOrRegisterParameter(Parameter, GetName(), GetClass()->GetName());
#else
	return AtomModulation::GetOrRegisterParameter(Parameter, FString(), FString());
#endif
}

