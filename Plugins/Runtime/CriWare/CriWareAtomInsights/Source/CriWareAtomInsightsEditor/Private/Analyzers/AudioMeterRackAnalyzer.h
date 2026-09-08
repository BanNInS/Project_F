
#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

#include "AudioMeterAnalyzer.h"

// Forward Definitions
class SAtomLevelMeter;
class UAtomRackWithParentBase;

namespace Atom::Insights
{
	class FAudioMeterRackAnalyzer final : public TSharedFromThis<FAudioMeterRackAnalyzer>
	{
	public:

		FAudioMeterRackAnalyzer(TWeakObjectPtr<UAtomRackWithParentBase> InAtomRack);
		~FAudioMeterRackAnalyzer();

		void SetRack(TWeakObjectPtr<UAtomRackWithParentBase> InAtomRack);

		TSharedRef<SAtomLevelMeter> GetWidget();

	private:

		TWeakObjectPtr<UAtomRackWithParentBase> AtomRack;
		FAudioMeterAnalyzer AudioMeterAnalyzer;
	};
} // namespace
