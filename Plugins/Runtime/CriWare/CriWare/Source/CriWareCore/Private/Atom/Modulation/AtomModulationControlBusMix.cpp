
#include "Atom/Modulation/AtomModulationControlBusMix.h"

#include "Engine/World.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationProfileSerializer.h"
#include "Atom/Modulation/AtomModulationStatics.h"
#include "Atom/Modulation/AtomModulationControlBus.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationControlBusMix)

#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "AtomModulation"

FAtomModulationControlBusMixStage::FAtomModulationControlBusMixStage()
	: Bus(nullptr)
{
}

FAtomModulationControlBusMixStage::FAtomModulationControlBusMixStage(UAtomModulationControlBus* InBus, const float TargetValue)
	: Bus(InBus)
{
	Value.TargetValue = FMath::Clamp(TargetValue, 0.0f, 1.0f);
}

UAtomModulationControlBusMix::UAtomModulationControlBusMix(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomModulationControlBusMix::BeginDestroy()
{
	if (UWorld* World = GetWorld())
	{
		FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World);
		if (AtomRuntime.IsValid())
		{
			AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem();
			check(Modulation);
			if (Modulation)
			{
				Modulation->DeactivateBusMix(*this);
			}
		}
	}

	// Call parent destroy at end to ensure object is in a valid state for the modulation manager to clean up first
	Super::BeginDestroy();
}

void UAtomModulationControlBusMix::ActivateMix()
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
	{
		OutModulation.ActivateBusMix(*this);
	});
}

void UAtomModulationControlBusMix::DeactivateMix()
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
	{
		OutModulation.DeactivateBusMix(*this);
	});
}

void UAtomModulationControlBusMix::DeactivateAllMixes()
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
	{
		OutModulation.DeactivateAllBusMixes();
	});
}

void UAtomModulationControlBusMix::LoadMixFromProfile()
{
	const bool bSucceeded = AtomModulation::FProfileSerializer::Deserialize(ProfileIndex, *this);
#if WITH_EDITOR
	if (bSucceeded)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("AtomModulationControlBusMix_LoadSucceeded", "'Control Bus Mix '{0}' profile {1} loaded successfully."),
			FText::FromName(GetFName()),
			FText::AsNumber(ProfileIndex)
		));
		Info.bFireAndForget = true;
		Info.ExpireDuration = 2.0f;
		Info.bUseThrobber = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
#endif // WITH_EDITOR

	ActivateMix();
}

#if WITH_EDITOR
void UAtomModulationControlBusMix::PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
{
	OnPropertyChanged(InPropertyChangedEvent.Property, InPropertyChangedEvent.ChangeType);
	Super::PostEditChangeProperty(InPropertyChangedEvent);
}

void UAtomModulationControlBusMix::PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	OnPropertyChanged(InPropertyChangedEvent.Property, InPropertyChangedEvent.ChangeType);
	Super::PostEditChangeChainProperty(InPropertyChangedEvent);
}

void UAtomModulationControlBusMix::OnPropertyChanged(FProperty* Property, EPropertyChangeType::Type InChangeType)
{
	if (Property)
	{
		if (InChangeType == EPropertyChangeType::Interactive || InChangeType == EPropertyChangeType::ValueSet)
		{
			if ((Property->GetFName() == GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, TargetValue))
				|| (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FAtomModulationControlBusMixStage, Bus)))
			{
				for (FAtomModulationControlBusMixStage& Stage : MixStages)
				{
					if (Stage.Bus)
					{
						Stage.Value.TargetValue = FMath::Clamp(Stage.Value.TargetValue, 0.0f, 1.0f);
						float UnitValue = Stage.Value.TargetValue;
						if (UAtomModulationParameter* Parameter = Stage.Bus->Parameter)
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
	}

	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
	{
		OutModulation.UpdateMix(*this);
	});
}
#endif // WITH_EDITOR

void UAtomModulationControlBusMix::SaveMixToProfile()
{
	const bool bSucceeded = AtomModulation::FProfileSerializer::Serialize(*this, ProfileIndex);
#if WITH_EDITOR
	if (bSucceeded)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("AtomModulationControlBusMix_SaveSucceeded", "'Control Bus Mix '{0}' profile {1} saved successfully."),
			FText::FromName(GetFName()),
			FText::AsNumber(ProfileIndex)
		));
		Info.bFireAndForget = true;
		Info.ExpireDuration = 2.0f;
		Info.bUseThrobber = true;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
#endif // WITH_EDITOR
}

void UAtomModulationControlBusMix::SoloMix()
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
	{
		OutModulation.SoloBusMix(*this);
	});
}

#undef LOCTEXT_NAMESPACE
