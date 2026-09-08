
#include "Atom/Gameplay/AtomAisacParameterComponent.h"

#include "Kismet/KismetStringLibrary.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
//#include "IAudioParameterTransmitter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomAisacParameterComponent)

namespace AtomAisacParameterComponentConsoleVariables
{
	int32 bSetParamOnlyOnValueChange = 1;
	FAutoConsoleVariableRef CVarSetParamOnlyOnValueChange(
		TEXT("atom.AtomAisacParameterComponent.SetParamOnlyOnValueChange"),
		bSetParamOnlyOnValueChange,
		TEXT("Only sets parameters when the underlying value has changed.\n0: Disable, 1: Enable (default)"),
		ECVF_Default);
}

DEFINE_LOG_CATEGORY(LogCriWareAtomAisacParameterComponent);

void UAtomAisacParameterComponent::GetActorAisacParams_Implementation(TArray<FAtomAisacParameter>& Params) const
{
	Params.Append(GetParameters());
}

void UAtomAisacParameterComponent::ResetParameters()
{
	int32 CurrSize = Parameters.Num();
	Parameters.Reset(CurrSize);
}

void UAtomAisacParameterComponent::SetTriggerParameter(const FAtomAisacControl& InControl)
{
	// todo: use "GetAllAtomComponents" and execute this trigger on them
	UE_LOG(LogCriWareAtom, Warning, TEXT("Trigger parameter not supported for UAtomAisacParameterComponent"));
}

void UAtomAisacParameterComponent::SetAisacParameter(const FAtomAisacControl& InControl, float InValue)
{
	SetParameterInternal(FAtomAisacParameter(InControl, InValue));
}

void UAtomAisacParameterComponent::SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InParameters)
{
	TArray<FAtomAisacParameter> Values = InParameters;
	SetParameters(MoveTemp(Values));
}

void UAtomAisacParameterComponent::SetParameter(FAtomAisacParameter&& InValue)
{
	SetParameterInternal(MoveTemp(InValue));
}

void UAtomAisacParameterComponent::SetParameters(TArray<FAtomAisacParameter>&& InValues)
{
	for (const FAtomAisacParameter& Value : InValues)
	{
		if (FAtomAisacParameter* CurrentParam = FAtomAisacParameter::FindOrAddParam(Parameters, Value.Control))
		{
			CurrentParam->Value = Value.Value;
		}
	}

	// Forward to any AtomComponents currently playing on this actor (if any)
	TArray<TObjectPtr<UAtomComponent>> Components;
	GetAllAtomComponents(Components);

	for (auto& Component : Components)
	{
		if (Component && Component->IsPlaying())
		{
			TArray<FAtomAisacParameter> ValuesTemp = InValues;
			Component->SetParameters(MoveTemp(ValuesTemp));
		}
	}
}

void UAtomAisacParameterComponent::SetParameterInternal(FAtomAisacParameter&& InParam)
{
	if (!InParam.Control.IsValid())
	{
		return;
	}

	const int32 StartingParamCount = Parameters.Num();
	if (FAtomAisacParameter* CurrentParam = FAtomAisacParameter::FindOrAddParam(Parameters, InParam.Control))
	{
		if (AtomAisacParameterComponentConsoleVariables::bSetParamOnlyOnValueChange != 0)
		{
			if (StartingParamCount == Parameters.Num())
			{
				// Early out if we did not add a parameter and this one matches.
				// We always take the type of the param (see merge below), so the utility function below
				// is only going to test based on changes to the right hand side 
				return;
			}
		}

		CurrentParam->Value = InParam.Value;;
	}

	// Optional logging
	LogParameter(InParam);

	// Optional broadcast (for editor-usage only)
#if WITH_EDITORONLY_DATA
	if (OnParameterChanged.IsBound())
	{
		OnParameterChanged.Broadcast(InParam);
	}
#endif // WITH_EDITORONLY_DATA

	// Forward to any AudioComponents currently playing on this actor (if any)
	TArray<TObjectPtr<UAtomComponent>> Components;
	GetAllAtomComponents(Components);

	TArray<FAtomAisacParameter> ParametersCopy;
	for (auto& Component : Components)
	{
		if (Component == nullptr)
		{
			continue;
		}

		if (Component->IsPlaying() && !Component->GetDisableParameterUpdatesWhilePlaying())
		{
			if (FAtomRuntime* AtomRuntime = Component->GetAtomRuntime())
			{
				ParametersCopy = Parameters;
				if (UAtomSoundBase* Sound = Component->GetSound())
				{
					static const FName ProxyFeatureName("AtomParameterComponent");
					Sound->InitParameters(ParametersCopy, ProxyFeatureName);
				}

				// Prior call to InitParameters can prune parameters if they are
				// invalid, so check here to avoid unnecessary pass of empty array.
				if (!ParametersCopy.IsEmpty())
				{
					DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SoundParameterControllerInterface.SetParameters"), STAT_AtomSetParameters, STATGROUP_AtomThreadCommands);

					AtomRuntime->SendCommandToActiveSounds(Component->GetInstanceOwnerID(), [AtomRuntime, Params = MoveTemp(ParametersCopy)](FAtomActiveSound& ActiveSound)
					{
						/*if (Audio::IParameterTransmitter* Transmitter = ActiveSound.GetTransmitter())
						{
							TArray<FAtomAisacParameter> TempParams = Params;
							Transmitter->SetParameters(MoveTemp(TempParams));
						}*/
						TArray<FAtomAisacParameter> TempParams = Params;
						FAtomAisacParameter::Merge(MoveTemp(TempParams), ActiveSound.AisacParametersOverride);

					}, GET_STATID(STAT_AtomSetParameters));
				}
			}
		}
	}
}

void UAtomAisacParameterComponent::GetAllAtomComponents(TArray<TObjectPtr<UAtomComponent>>& OutComponents) const
{
	AActor* RootActor = GetOwner();
	if (USceneComponent* RootComp = RootActor->GetRootComponent())
	{
		// Walk up to the top-most attach actor in the hierarchy (will just be the RootActor if no attachment)
		RootActor = RootComp->GetAttachmentRootActor();
	}

	// Helper to collect components from an actor
	auto CollectFromActor = [&OutComponents](const AActor* InActor)
	{
		if (InActor)
		{
			TInlineComponentArray<TObjectPtr<UAtomComponent>> TempComponents;
			constexpr bool bIncludeChildActors = false;
			InActor->GetComponents(TempComponents, bIncludeChildActors);
			OutComponents.Append(MoveTemp(TempComponents));
		}
	};

	if (RootActor)
	{
		// Grab any AudioComponents from our owner directly
		CollectFromActor(RootActor);

		// Recursively grab AudioComponents from our Attached Actor's
		TArray<AActor*> AttachedActors;
		constexpr bool bResetArray = false;
		constexpr bool bRecursivelyIncludeAttachedActors = true;
		RootActor->GetAttachedActors(AttachedActors, bResetArray, bRecursivelyIncludeAttachedActors);

		for (AActor* Actor : AttachedActors)
		{
			CollectFromActor(Actor);
		}
	}
}

void UAtomAisacParameterComponent::LogParameter(FAtomAisacParameter& InParam)
{
	if (!UE_LOG_ACTIVE(LogCriWareAtomAisacParameterComponent, VeryVerbose))
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	FString ParamValueString = FString::SanitizeFloat(InParam.Value);

	UE_LOG(LogCriWareAtomAisacParameterComponent, VeryVerbose, TEXT("%s: Set AISAC %s to %s"), *Owner->GetName(), *InParam.Control.Name.ToString(), *ParamValueString)
}
