
#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

#include "AudioMeterAnalyzer.h"

// Forward Definitions
class SAtomLevelMeter;
class UAtomBus;

namespace Atom::Insights
{
	class FAudioMeterBusAnalyzer final : public TSharedFromThis<FAudioMeterBusAnalyzer>
	{
	public:
		
		FAudioMeterBusAnalyzer(TWeakObjectPtr<UAtomBus> InAtomBus);
		~FAudioMeterBusAnalyzer();

		// Rebuilds the audio meter and registers the newly selected bus or of a rack
		void SetBus(TWeakObjectPtr<UAtomBus> InAtomBus);

		TSharedRef<SAtomLevelMeter> GetWidget();

	private:
		
		void UnregisterAudioBusFromBus();

		TWeakObjectPtr<UAtomBus> AtomBus;
		FAudioMeterAnalyzer AudioMeterAnalyzer;
	};
} // namespace
