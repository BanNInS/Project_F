
#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

namespace AtomWidgets { class FAudioAnalyzerRack; }

class FSpawnTabArgs;
class SDockTab;
class SWidget;
class UAtomRackBase;
class UAtomBus;

namespace Atom::Insights
{
	class FSubmixAudioAnalyzerRack : public TSharedFromThis<FSubmixAudioAnalyzerRack>
	{
	public:
		FSubmixAudioAnalyzerRack(TWeakObjectPtr<UAtomRackBase> InAtomRack);
		FSubmixAudioAnalyzerRack(TWeakObjectPtr<UAtomBus> InAtomBus);
		virtual ~FSubmixAudioAnalyzerRack();

		TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> InOwnerTab, const FSpawnTabArgs& InSpawnTabArgs);

		void RebuildAudioAnalyzerRack(TWeakObjectPtr<UAtomRackBase> InAtomRack);
		void RebuildAudioAnalyzerRack(TWeakObjectPtr<UAtomBus> InSoundSubmix);

	private:
		void CleanupAudioAnalyzerRack();

		TSharedRef<AtomWidgets::FAudioAnalyzerRack> AudioAnalyzerRack;
		TWeakObjectPtr<UAtomRackBase> AtomRack;
		TWeakObjectPtr<UAtomBus> AtomBus;
	};
} // namespace
