
#include "AudioMeterAnalyzer.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomRack.h"
#include "AtomLevelMeter.h"

#include "CriWareAtomInsightsEditorModule.h"

namespace Atom::Insights
{
	namespace FLevelMeterPrivate
	{
		TSharedRef<AtomWidgets::FAtomLevelMeter> CreateLevelMeter(TWeakObjectPtr<UAtomAudioBus> InExternalAudioBus)
		{
			using namespace ::Atom;

			const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

			if (const FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
			{
				if (const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID))
				{
					return MakeShared<AtomWidgets::FAtomLevelMeter>(
						InExternalAudioBus.IsValid() ? InExternalAudioBus->GetNumChannels() : AtomRuntime->GetRuntimeNumOutputChannels(),
						AtomRuntimeID,
						InExternalAudioBus.Get());
				}
			}
			
			return MakeShared<AtomWidgets::FAtomLevelMeter>(1, AtomRuntimeID);
		}

		TSharedRef<AtomWidgets::FAtomLevelMeter> CreateLevelMeter(TWeakObjectPtr<UAtomRackBase> InAtomRack)
		{
			using namespace ::Atom;

			const FCriWareAtomInsightsEditorModule AtomInsightsEditorModule = FCriWareAtomInsightsEditorModule::GetChecked();
			const FAtomRuntimeId AtomRuntimeID = AtomInsightsEditorModule.GetRuntimeID();

			if (const FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
			{
				if (const FAtomRuntime* AtomRuntime = AtomRuntimeManager->GetAtomRuntimeRaw(AtomRuntimeID))
				{
					return MakeShared<AtomWidgets::FAtomLevelMeter>(
						InAtomRack.IsValid() ? AtomRuntime->GetRackNumOutputChannels(InAtomRack.Get()) : AtomRuntime->GetRuntimeNumOutputChannels(),
						AtomRuntimeID,
						InAtomRack.Get());
				}
			}

			return MakeShared<AtomWidgets::FAtomLevelMeter>(1, AtomRuntimeID);
		}
	}

	FAudioMeterAnalyzer::FAudioMeterAnalyzer(TWeakObjectPtr<UAtomAudioBus> InExternalAudioBus)
		: LevelMeter(FLevelMeterPrivate::CreateLevelMeter(InExternalAudioBus))
	{
		
	}

	FAudioMeterAnalyzer::FAudioMeterAnalyzer(TWeakObjectPtr<UAtomRackBase> InAtomRack)
		: LevelMeter(FLevelMeterPrivate::CreateLevelMeter(InAtomRack))
	{

	}

	void FAudioMeterAnalyzer::RebuildAudioMeter(TWeakObjectPtr<UAtomRackBase> InAtomRack)
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

		LevelMeter->Init(
			InAtomRack.IsValid() ? AtomRuntime->GetRackNumOutputChannels(InAtomRack.Get()) : AtomRuntime->GetRuntimeNumOutputChannels(), 
			AtomRuntimeID, 
			InAtomRack.Get());
	}

	void FAudioMeterAnalyzer::RebuildAudioMeter(TWeakObjectPtr<UAtomAudioBus> InExternalAudioBus)
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

		LevelMeter->Init(
			InExternalAudioBus.IsValid() ? InExternalAudioBus->GetNumChannels() : AtomRuntime->GetRuntimeNumOutputChannels(), 
			AtomRuntimeID, 
			InExternalAudioBus.Get());
	}
} // namespace
