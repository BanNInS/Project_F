
#include "Atom/Modulation/AtomModulationStatics.h"

#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Async/Async.h"

#include "CriWare.h"
#include "Atom/AtomRuntime.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationProfileSerializer.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationStatics)

#define LOCTEXT_NAMESPACE "AtomModulationStatics"


static FAutoConsoleCommand GAtomModulationSaveMixProfile(
	TEXT("atom.Modulation.SaveMixProfile"),
	TEXT("Saves modulation mix profile to the config save directory.\n"
		"Path - Path to Object\n"
		"ProfileIndex - (Optional) Index of profile (defaults to 0)"),
	FConsoleCommandWithArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogAtomModulation, Error, TEXT("Failed to save mix profile: Path not provided"));
				return;
			}

			const FString& Path = Args[0];
			int32 ProfileIndex = 0;
			if (Args.Num() > 1)
			{
				ProfileIndex = FCString::Atoi(*Args[1]);
			}

			FSoftObjectPath ObjPath = Path;
			if (UObject* MixObj = ObjPath.TryLoad())
			{
				if (UAtomModulationControlBusMix* Mix = Cast<UAtomModulationControlBusMix>(MixObj))
				{
					UAtomModulationStatics::SaveMixToProfile(Mix, Mix, ProfileIndex);
					return;
				}
			}

			UE_LOG(LogAtomModulation, Error, TEXT("Failed to save mix '%s' to profile index '%i'"), *Path, ProfileIndex);
		}
	)
);

static FAutoConsoleCommand GAtomModulationLoadMixProfile(
	TEXT("atom.Modulation.LoadMixProfile"),
	TEXT("Loads modulation mix profile from the config save directory.\n"
		"Path - Path to Object to load\n"
		"Activate - (Optional) Whether or not to activate/update the mix once it is loaded (default: true)."
		"ProfileIndex - (Optional) Index of profile (default: 0)"),
	FConsoleCommandWithArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogAtomModulation, Error, TEXT("Failed to load mix profile: Object path not provided"));
				return;
			}

			const FString& Path = Args[0];
			int32 ProfileIndex = 0;
			if (Args.Num() > 1)
			{
				ProfileIndex = FCString::Atoi(*Args[1]);
			}

			bool bActivateUpdate = true;
			if (Args.Num() > 2)
			{
				bActivateUpdate = FCString::ToBool(*Args[2]);
			}

			FSoftObjectPath ObjPath = Path;
			if (UObject* MixObj = ObjPath.TryLoad())
			{
				if (UAtomModulationControlBusMix* Mix = Cast<UAtomModulationControlBusMix>(MixObj))
				{
					UAtomModulationStatics::LoadMixFromProfile(Mix, Mix, bActivateUpdate, ProfileIndex);

					if (bActivateUpdate)
					{
						UAtomModulationStatics::UpdateMixFromObject(Mix, Mix);
					}
					return;
				}
			}

			UE_LOG(LogAtomModulation, Error, TEXT("Failed to load mix '%s' from profile index '%i'"), *Path, ProfileIndex);
		}
	)
);

UAtomModulationStatics::UAtomModulationStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomModulationStatics::ActivateBus(const UObject* WorldContextObject, UAtomModulationControlBus* Bus)
{
	if (!Bus)
	{
		return;
	}

	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ModSystem->ActivateBus(*Bus);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}

void UAtomModulationStatics::ActivateBusMix(const UObject* WorldContextObject, UAtomModulationControlBusMix* BusMix)
{
	if (BusMix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->ActivateBusMix(*BusMix);
		}
	}
}

void UAtomModulationStatics::ActivateGenerator(const UObject* WorldContextObject, UAtomModulationGenerator* Generator)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		if (Generator)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ModSystem->ActivateGenerator(*Generator);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
}

UWorld* UAtomModulationStatics::GetAudioWorld(const UObject* WorldContextObject)
{
	if (!GEngine || !GEngine->UseSound())
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || !World->bAllowAudioPlayback || World->IsNetMode(NM_DedicatedServer))
	{
		return nullptr;
	}

	return World;
}

AtomModulation::FAtomModulationSystem* UAtomModulationStatics::GetModulation(UWorld* World)
{
	FAtomRuntimeHandle AtomRuntime;
	if (World)
	{
		AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World);
	}
	else
	{
		if (GCriWare)
		{
			AtomRuntime = GCriWare->GetMainAtomRuntime();
		}
	}

	if (AtomRuntime.IsValid())
	{
		if (AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem())
		{
			return Modulation;
		}
	}

	return nullptr;
}

UAtomModulationControlBus* UAtomModulationStatics::CreateBus(UObject* WorldContextObject, FName Name, UAtomModulationParameter* Parameter, bool Activate)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationControlBus::StaticClass(), Name);
	UAtomModulationControlBus* NewBus = NewObject<UAtomModulationControlBus>(WorldContextObject, UniqueName);
	NewBus->Parameter = Parameter;
	NewBus->Address = Name.ToString();

	if (Activate)
	{
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ModSystem->ActivateBus(*NewBus);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}

	return NewBus;
}

FAtomModulationControlBusMixStage UAtomModulationStatics::CreateBusMixStage(const UObject* WorldContextObject, UAtomModulationControlBus* Bus, float Value, float AttackTime, float ReleaseTime)
{
	FAtomModulationControlBusMixStage MixStage;
	MixStage.Bus = Bus;
	MixStage.Value = FAtomModulationMixValue(Value, AttackTime, ReleaseTime);
	return MixStage;
}

UAtomModulationControlBusMix* UAtomModulationStatics::CreateBusMix(UObject* WorldContextObject, FName Name, TArray<FAtomModulationControlBusMixStage> Stages, bool Activate, double Duration, bool bRetriggerOnActivation)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationControlBusMix::StaticClass(), Name);
	UAtomModulationControlBusMix* NewBusMix = NewObject<UAtomModulationControlBusMix>(WorldContextObject, UniqueName);
	for (FAtomModulationControlBusMixStage& Stage : Stages)
	{
		if (Stage.Bus)
		{
			NewBusMix->MixStages.Emplace(Stage);
		}
		else
		{
			UE_LOG(LogAtomModulation, Warning,
				TEXT("UAtomModulationControlBusMix '%s' was created but bus provided is null. Stage not added to mix."),
				*Name.ToString());
		}
	}

	NewBusMix->Duration = Duration;
	NewBusMix->bRetriggerOnActivation = bRetriggerOnActivation;

	if (Activate)
	{
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->ActivateBusMix(*NewBusMix);
		}
	}

	return NewBusMix;
}

UAtomModulationParameter* UAtomModulationStatics::CreateModulationParameter(UObject* WorldContextObject, FName Name, TSubclassOf<UAtomModulationParameter> ParamClass, float DefaultValue)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationParameter::StaticClass(), Name);
	UAtomModulationParameter* NewParam = NewObject<UAtomModulationParameter>(WorldContextObject, ParamClass, UniqueName);

	if (NewParam)
	{
		NewParam->Settings.ValueNormalized = DefaultValue;
	}

	return NewParam;
}

UAtomModulationGeneratorEnvelopeFollower* UAtomModulationStatics::CreateEnvelopeFollowerGenerator(UObject* WorldContextObject, FName Name, FAtomEnvelopeFollowerGeneratorParams Params)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationGeneratorEnvelopeFollower::StaticClass(), Name);
	UAtomModulationGeneratorEnvelopeFollower* NewGenerator = NewObject<UAtomModulationGeneratorEnvelopeFollower>(WorldContextObject, UAtomModulationGeneratorEnvelopeFollower::StaticClass(), UniqueName);

	if (NewGenerator)
	{
		NewGenerator->Params = Params;
	}

	return NewGenerator;
}

UAtomModulationGeneratorLFO* UAtomModulationStatics::CreateLFOGenerator(UObject* WorldContextObject, FName Name, FAtomModulationLFOParams Params)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationGeneratorLFO::StaticClass(), Name);
	UAtomModulationGeneratorLFO* NewGenerator = NewObject<UAtomModulationGeneratorLFO>(WorldContextObject, UAtomModulationGeneratorLFO::StaticClass(), UniqueName);

	if (NewGenerator)
	{
		NewGenerator->Params = Params;
	}

	return NewGenerator;
}

UAtomModulationGeneratorADEnvelope* UAtomModulationStatics::CreateADEnvelopeGenerator(UObject* WorldContextObject, FName Name, const FAtomModulationADEnvelopeParams& Params)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationGeneratorADEnvelope::StaticClass(), Name);
	UAtomModulationGeneratorADEnvelope* NewGenerator = NewObject<UAtomModulationGeneratorADEnvelope>(WorldContextObject, UAtomModulationGeneratorADEnvelope::StaticClass(), UniqueName);

	if (NewGenerator)
	{
		NewGenerator->Params = Params;
	}

	return NewGenerator;
}

UAtomModulationDestination* UAtomModulationStatics::CreateModulationDestination(UObject* WorldContextObject, FName Name, UAtomModulatorBase* Modulator)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (!World)
	{
		return nullptr;
	}

	FName UniqueName = MakeUniqueObjectName(WorldContextObject, UAtomModulationDestination::StaticClass(), Name);
	UAtomModulationDestination* NewDestination = NewObject<UAtomModulationDestination>(WorldContextObject, UAtomModulationDestination::StaticClass(), UniqueName);
	if (NewDestination)
	{
		NewDestination->SetModulator(Modulator);
		return NewDestination;
	}

	return nullptr;
}

void UAtomModulationStatics::DeactivateBus(const UObject* WorldContextObject, UAtomModulationControlBus* Bus)
{
	if (Bus)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ModSystem->DeactivateBus(*Bus);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
}

void UAtomModulationStatics::DeactivateBusMix(const UObject* WorldContextObject, UAtomModulationControlBusMix* BusMix)
{
	if (BusMix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->DeactivateBusMix(*BusMix);
		}
	}
}

void UAtomModulationStatics::DeactivateGenerator(const UObject* WorldContextObject, UAtomModulationGenerator* Generator)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		if (Generator)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			ModSystem->DeactivateGenerator(*Generator);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}
	}
}

bool UAtomModulationStatics::IsControlBusMixActive(const UObject* WorldContextObject, UAtomModulationControlBusMix* Mix)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		if (Mix)
		{
			return ModSystem->IsControlBusMixActive(*Mix);
		}
	}
	return false;
}

void UAtomModulationStatics::SaveMixToProfile(const UObject* WorldContextObject, UAtomModulationControlBusMix* BusMix, int32 ProfileIndex)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		if (BusMix)
		{
			return ModSystem->SaveMixToProfile(*BusMix, ProfileIndex);
		}
	}
}

TArray<FAtomModulationControlBusMixStage> UAtomModulationStatics::LoadMixFromProfile(const UObject* WorldContextObject, UAtomModulationControlBusMix* BusMix, bool bActivate, int32 ProfileIndex)
{
	if (BusMix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			if (bActivate)
			{
				ActivateBusMix(WorldContextObject, BusMix);
			}
			return ModSystem->LoadMixFromProfile(ProfileIndex, *BusMix);
		}
	}

	return TArray<FAtomModulationControlBusMixStage>();
}

void UAtomModulationStatics::UpdateMix(const UObject* WorldContextObject, UAtomModulationControlBusMix* Mix, TArray<FAtomModulationControlBusMixStage> Stages, float InFadeTime, double Duration, bool bRetriggerOnActivation)
{
	if (Mix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			// UObject representation is not updated in this form of the call as doing so from
			// PIE can result in an unstable state where UObject is modified but not properly dirtied.
			ModSystem->UpdateMix(Stages, *Mix, false /* bUpdateObject */, InFadeTime, Duration, bRetriggerOnActivation);
		}
	}
}

UAtomModulationControlBusMix* UAtomModulationStatics::CreateBusMixFromValue(const UObject* WorldContextObject, FName Name, const TArray<UAtomModulationControlBus*>& Buses, float Value, float AttackTime, float ReleaseTime, bool bActivate)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		if (UAtomModulationControlBusMix* NewMix = ModSystem->CreateBusMixFromValue(Name, Buses, Value, AttackTime, ReleaseTime))
		{
			if (bActivate)
			{
				ModSystem->ActivateBusMix(*NewMix);
			}

			return NewMix;
		}
	}

	return nullptr;
}

void UAtomModulationStatics::SetGlobalBusMixValue(const UObject* WorldContextObject, UAtomModulationControlBus* Bus, float Value, float FadeTime)
{
	if (Bus)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->SetGlobalBusMixValue(*Bus, Value, FadeTime);
		}
	}
}

void UAtomModulationStatics::ClearGlobalBusMixValue(const UObject* WorldContextObject, UAtomModulationControlBus* Bus, float FadeTime)
{
	if (Bus)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->ClearGlobalBusMixValue(*Bus, FadeTime);
		}
	}
}

void UAtomModulationStatics::ClearAllGlobalBusMixValues(const UObject* WorldContextObject, float FadeTime)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		ModSystem->ClearAllGlobalBusMixValues(FadeTime);
	}
}

void UAtomModulationStatics::DeactivateAllBusMixes(const UObject* WorldContextObject)
{
	UWorld* World = GetAudioWorld(WorldContextObject);
	if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
	{
		ModSystem->DeactivateAllBusMixes();
	}
}

void UAtomModulationStatics::UpdateMixByFilter(
	const UObject* WorldContextObject,
	UAtomModulationControlBusMix* Mix,
	FString AddressFilter,
	TSubclassOf<UAtomModulationParameter> ParamClassFilter,
	UAtomModulationParameter* ParamFilter,
	float Value,
	float FadeTime)
{
	if (Mix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			// UObject representation is not updated in this form of the call as doing so from
			// PIE can result in an unstable state where UObject is modified but not properly dirtied.
			ModSystem->UpdateMixByFilter(AddressFilter, ParamClassFilter, ParamFilter, Value, FadeTime, *Mix, false /* bUpdateObject */);
		}
	}
}

void UAtomModulationStatics::UpdateMixFromObject(const UObject* WorldContextObject, UAtomModulationControlBusMix* Mix, float InFadeTime)
{
	if (Mix)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->UpdateMix(*Mix, InFadeTime);
		}
	}
}

void UAtomModulationStatics::UpdateModulator(const UObject* WorldContextObject, UAtomModulatorBase* Modulator)
{
	if (Modulator)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			ModSystem->UpdateModulator(*Modulator);
		}
	}
}

float UAtomModulationStatics::GetModulatorValue(const UObject* WorldContextObject, UAtomModulatorBase* Modulator)
{
	if (Modulator)
	{
		UWorld* World = GetAudioWorld(WorldContextObject);
		if (AtomModulation::FAtomModulationSystem* ModSystem = GetModulation(World))
		{
			float OutValue = 1.0f;
			ModSystem->GetModulatorValueThreadSafe(Modulator->GetUniqueID(), OutValue);
			return OutValue;
		}
	}

	return 1.0f;
}

TSet<UAtomModulatorBase*> UAtomModulationStatics::GetModulatorsFromDestination(const FAtomSoundModulationSettings& Destination)
{
	TSet<UAtomModulatorBase*> Modulators;
	for (const TObjectPtr<UAtomModulatorBase>& Modulator : Destination.Modulators)
	{
		Modulators.Add(Modulator.Get());
	}

	return Modulators;
}

#undef LOCTEXT_NAMESPACE
