
#include "Atom/Gameplay/AtomAisacVolumeComponent.h"

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/AtomVolume.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/Gameplay/AtomGameplayFlags.h"

#define LOCTEXT_NAMESPACE "AtomAisacVolumeComponent"

constexpr TCHAR FAtomProxyMutator_Aisac::MutatorAisacName[];

FAtomProxyMutator_Aisac::FAtomProxyMutator_Aisac()
{
	MutatorName = MutatorAisacName;
}

void FAtomProxyMutator_Aisac::Apply(FAtomProxyActiveSoundParams& Params) const
{
	check(IsInAtomThread());

	// Determine location state.  Inside if any of the following conditions are met:
	// spatialization disabled on the active sound, we're in the same volume as the listener,
	// or if the active sound's interior settings are still 'default' (bUsingWorldSettings)
	EAtomVolumeLocationState LocationState = EAtomVolumeLocationState::OutsideTheVolume;
	if (Params.bListenerInVolume || !Params.bAllowSpatialization || Params.bUsingWorldSettings)
	{
		LocationState = EAtomVolumeLocationState::InsideTheVolume;
	}

	if (AisacControlSettings.Num() > 0)
	{
		// Internal statuses setup
		if (StartTime < 0.0f)
		{
			InterpValues.Reset();

			for (const FAtomVolumeAisacControlSettings& ControlSetting : AisacControlSettings)
			{
				for (const FAtomVolumeAisacControlSetting& ControlInfo : ControlSetting.AisacControls)
				{
					InterpValues.Add(ControlInfo.Control, { ControlInfo.Value, ControlInfo.Value });
				}
			}
		}

		if (StartTime < 0.0f || LocationState != PrevLocationState)
		{
			StartTime = FApp::GetCurrentTime();
		}
		
		for (const FAtomVolumeAisacControlSettings& ControlSetting : AisacControlSettings)
		{
			if (ControlSetting.ListenerLocationState == LocationState)
			{
				float Interp = Interpolate(FApp::GetCurrentTime(), StartTime + FMath::Max(ControlSetting.FadeTime, 0.0f));

				for (const FAtomVolumeAisacControlSetting& ControlInfo : ControlSetting.AisacControls)
				{
					auto& [Start, Last] = InterpValues[ControlInfo.Control];

					if (LocationState != PrevLocationState)
					{
						Start = Last;
					}

					if (Interp < 1.0f)
					{
						Last = FMath::Lerp(Start, ControlInfo.Value, Interp);
					}
					else
					{
						Last = ControlInfo.Value;
					}

					Params.AisacControls.Add(ControlInfo.Control, Last);
				}
			}
		}

		PrevLocationState = LocationState;
	}
}

float FAtomProxyMutator_Aisac::Interpolate(double CurrentTime, double EndTime) const
{
	if (CurrentTime < StartTime)
	{
		return 0.0f;
	}

	if (CurrentTime >= EndTime)
	{
		return 1.0f;
	}

	float InterpValue = (float)((CurrentTime - StartTime) / (EndTime - StartTime));
	return FMath::Clamp(InterpValue, 0.0f, 1.0f);
}

UAtomAisacVolumeComponent::UAtomAisacVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound;
	bAutoActivate = true;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomAisacVolumeComponent::SetAisacControlSettings(const TArray<FAtomVolumeAisacControlSettings>& NewAisacControlSettings)
{
	AisacControlSettings = NewAisacControlSettings;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomAisacVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_Aisac>();
}

void UAtomAisacVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	TSharedPtr<FAtomProxyMutator_Aisac> AisacMutator = StaticCastSharedPtr<FAtomProxyMutator_Aisac>(Mutator);
	AisacMutator->AisacControlSettings = AisacControlSettings;
}

#undef LOCTEXT_NAMESPACE
