
#include "Atom/Interfaces/IAtomSoundAisacParameterController.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
//#include "IAudioParameterTransmitter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IAtomSoundAisacParameterController)


namespace AtomSoundAisacParameterControllerPrivate
{
	static const FName ProxyFeatureName("IAtomSoundAisacParameterController");
} // namespace

UAtomSoundAisacParameterController::UAtomSoundAisacParameterController(FObjectInitializer const& InObjectInitializer)
	: Super(InObjectInitializer)
{
}

void IAtomSoundAisacParameterController::ResetParameters()
{
	if (FAtomRuntime* AtomRuntime = GetAtomRuntime())
	{
		if (IsPlaying() && !GetDisableParameterUpdatesWhilePlaying())
		{
			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.AtomSoundAisacParameterController.ResetParameters"), STAT_AtomResetParameters, STATGROUP_AtomThreadCommands);
			AtomRuntime->SendCommandToActiveSounds(GetInstanceOwnerID(), [](FAtomActiveSound& ActiveSound)
			{
				//if (Audio::IParameterTransmitter* Transmitter = ActiveSound.GetTransmitter())
				//{
				//	//Transmitter->ResetParameters();
				//}
				ActiveSound.AisacParametersOverride.Empty();

			}, GET_STATID(STAT_AtomResetParameters));
		}
	}
}

void IAtomSoundAisacParameterController::SetTriggerParameter(const FAtomAisacControl& InControl)
{
	if (!InControl.IsValid())
	{
		return;
	}

	if (IsPlaying() && !GetDisableParameterUpdatesWhilePlaying())
	{
		if (FAtomRuntime* AtomRuntime = GetAtomRuntime())
		{
			FAtomAisacParameter ParamToSet = FAtomAisacParameter(InControl, 1.0);//EAudioParameterType::Trigger);
			
			if (UAtomSoundBase* Sound = GetSound())
			{
				TArray<FAtomAisacParameter> Params = { MoveTemp(ParamToSet) };
				Sound->InitParameters(Params, AtomSoundAisacParameterControllerPrivate::ProxyFeatureName);
				if (Params.Num() == 0)
				{
					// UAtomSoundBase::InitParameters(...) can remove parameters. 
					// Exit early if the parameter is removed.
					return;
				}
				ParamToSet = MoveTemp(Params[0]);
			}

			DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SoundParameterController.ExecuteTriggerParameter"), STAT_AtomExecuteTriggerParameter, STATGROUP_AtomThreadCommands);

			AtomRuntime->SendCommandToActiveSounds(GetInstanceOwnerID(), [AtomRuntime, Param = MoveTemp(ParamToSet)](FAtomActiveSound& ActiveSound)
			{
				/*if (Audio::IParameterTransmitter* Transmitter = ActiveSound.GetTransmitter())
				{
					const FName ParamName = Param.ParamName;

					// Must be copied as original version must be preserved in case command is called on multiple ActiveSounds.
					TArray<FAtomAisacParameter> TempParam = { Param };
					if (!Transmitter->SetParameters(MoveTemp(TempParam)))
					{
						UE_LOG(LogAudio, Warning, TEXT("Failed to execute trigger parameter '%s'"), *ParamName.ToString());
					}
				}*/
				TArray<FAtomAisacParameter> TempParam = { Param };
				FAtomAisacParameter::Merge(MoveTemp(TempParam), ActiveSound.AisacParametersOverride);

			}, GET_STATID(STAT_AtomExecuteTriggerParameter));
		}
	}
}

void IAtomSoundAisacParameterController::SetAisacParameter(const FAtomAisacControl& InControl, float InValue)
{
	SetParameters( { FAtomAisacParameter(InControl, InValue) });
}

void IAtomSoundAisacParameterController::SetParameter(FAtomAisacParameter&& InValue)
{
	SetParameters({ MoveTemp(InValue) });
}

void IAtomSoundAisacParameterController::SetParameters(TArray<FAtomAisacParameter>&& InValues)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ISoundParameterControllerInterface::SetParameters);

	const bool bUpdateActiveSound = IsPlaying() && !GetDisableParameterUpdatesWhilePlaying();

	TArray<FAtomAisacParameter> ParamsToSet;
	if (bUpdateActiveSound)
	{
		ParamsToSet = InValues;
	}

	TArray<FAtomAisacParameter>& InstanceParameters = GetInstanceParameters();
	FAtomAisacParameter::Merge(MoveTemp(InValues), InstanceParameters);

	if (bUpdateActiveSound)
	{
		if (FAtomRuntime* AtomRuntime = GetAtomRuntime())
		{
			if (UAtomSoundBase* Sound = GetSound())
			{
				Sound->InitParameters(ParamsToSet, AtomSoundAisacParameterControllerPrivate::ProxyFeatureName);
			}

			// Prior call to InitParameters can prune parameters if they are
			// invalid, so check here to avoid unnecessary pass of empty array.
			if (!ParamsToSet.IsEmpty())
			{
				DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SoundParameterControllerInterface.SetParameters"), STAT_AtomSetParameters, STATGROUP_AtomThreadCommands);
				AtomRuntime->SendCommandToActiveSounds(GetInstanceOwnerID(), [AtomRuntime, Params = MoveTemp(ParamsToSet)](FAtomActiveSound& ActiveSound) mutable
				{
					/*if (Audio::IParameterTransmitter* Transmitter = ActiveSound.GetTransmitter())
					{
						
						Transmitter->SetParameters(MoveTemp(Temp));
					}*/

					TArray<FAtomAisacParameter> Temp = Params;
					FAtomAisacParameter::Merge(MoveTemp(Temp), ActiveSound.AisacParametersOverride);

				}, GET_STATID(STAT_AtomSetParameters));
			}
		}
	}
}

void IAtomSoundAisacParameterController::SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InValues)
{
	TArray<FAtomAisacParameter> Values = InValues;
	SetParameters(MoveTemp(Values));
}
