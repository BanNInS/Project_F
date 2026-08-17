
#include "AudioMeterRackAnalyzer.h"

#include "Editor.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRack.h"

#include "AtomLevelMeter.h"

namespace Atom::Insights
{
	FAudioMeterRackAnalyzer::FAudioMeterRackAnalyzer(TWeakObjectPtr<UAtomRackWithParentBase> InAtomRack)
	{
		SetRack(InAtomRack);
	}

	FAudioMeterRackAnalyzer::~FAudioMeterRackAnalyzer()
	{
		AtomRack.Reset();
	}

	void FAudioMeterRackAnalyzer::SetRack(TWeakObjectPtr<UAtomRackWithParentBase> InAtomRack)
	{
		AtomRack = InAtomRack;

		if (!AtomRack.IsValid())
		{
			return;
		}

		AudioMeterAnalyzer.RebuildAudioMeter(AtomRack);
	}

	TSharedRef<SAtomLevelMeter> FAudioMeterRackAnalyzer::GetWidget()
	{
		return AudioMeterAnalyzer.GetLevelMeter()->GetWidget();
	}
} // namespace
