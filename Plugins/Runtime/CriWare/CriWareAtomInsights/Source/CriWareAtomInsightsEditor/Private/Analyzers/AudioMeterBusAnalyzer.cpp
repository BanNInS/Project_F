
#include "AudioMeterBusAnalyzer.h"

#include "Editor.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomAudioBusSubsystem.h"

#include "CriWareAtomInsightsEditorModule.h"
#include "AtomLevelMeter.h"

namespace Atom::Insights
{
	FAudioMeterBusAnalyzer::FAudioMeterBusAnalyzer(TWeakObjectPtr<UAtomBus> InAtomBus)
	{
		SetBus(InAtomBus);
	}

	FAudioMeterBusAnalyzer::~FAudioMeterBusAnalyzer()
	{
		UnregisterAudioBusFromBus();
	}

	void FAudioMeterBusAnalyzer::SetBus(TWeakObjectPtr<UAtomBus> InAtomBus)
	{
		if (AtomBus.IsValid())
		{
			UnregisterAudioBusFromBus();
			AtomBus.Reset();
		}

		AudioMeterAnalyzer.RebuildAudioMeter();

		AtomBus = InAtomBus;

		if (!AtomBus.IsValid())
		{
			return;
		}
		
		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
		const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

		const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);
		if (!AtomRuntime)
		{
			return;
		}

		FMixerSubmixWeakPtr MixerSubmixWeakPtr = AtomRuntime->GetSubmixInstance(AtomBus.Get());
		if (!MixerSubmixWeakPtr.IsValid())
		{
			return;
		}

		// Register audio bus in submix
		const TObjectPtr<UAtomAudioBus> AudioBus = AudioMeterAnalyzer.GetLevelMeter()->GetAudioBus();
		if (!AudioBus)
		{
			return;
		}

		const FAudioBusKey AudioBusKey(AudioBus->GetUniqueID());
		const int32 AudioBusNumChannels = AudioBus->GetNumChannels();

		FAtomThread::RunCommandOnAtomThread([AtomRuntime, MixerSubmixWeakPtr, AudioBusKey, AudioBusNumChannels]()
		{
			TObjectPtr<UAtomAudioBusSubsystem> AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
			check(AudioBusSubsystem);

			if (FMixerSubmixPtr MixerSubmix = MixerSubmixWeakPtr.Pin();
				MixerSubmix.IsValid())
			{
				MixerSubmix->RegisterAudioBus(AudioBusKey, AudioBusSubsystem->AddPatchInputForAudioBus(AudioBusKey, AtomRuntime->GetRuntimeNumOutputFrames(), AudioBusNumChannels));
			}
		});
	}

	void FAudioMeterBusAnalyzer::UnregisterAudioBusFromBus()
	{
		FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
		if (!AtomRuntimeManager)
		{
			return;
		}

		const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
		const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

		const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID);
		if (!AtomRuntime)
		{
			return;
		}

		if (!AtomBus.IsValid())
		{
			return;
		}

		FMixerSubmixWeakPtr MixerSubmixWeakPtr = AtomRuntime->GetSubmixInstance(AtomBus.Get());
		if (!MixerSubmixWeakPtr.IsValid())
		{
			return;
		}

		// Unregister audio bus from submix
		const TObjectPtr<UAtomAudioBus> AudioBus = AudioMeterAnalyzer.GetLevelMeter()->GetAudioBus();
		if (!AudioBus)
		{
			return;
		}

		const FAudioBusKey AudioBusKey(AudioBus->GetUniqueID());

		FAtomThread::RunCommandOnAtomThread([MixerSubmixWeakPtr, AudioBusKey]()
		{
			if (FMixerSubmixPtr MixerSubmix = MixerSubmixWeakPtr.Pin();
				MixerSubmix.IsValid())
			{
				MixerSubmix->UnregisterAudioBus(AudioBusKey);
			}
		});
	}

	TSharedRef<SAtomLevelMeter> FAudioMeterBusAnalyzer::GetWidget()
	{
		return AudioMeterAnalyzer.GetLevelMeter()->GetWidget();
	}
} // namespace
