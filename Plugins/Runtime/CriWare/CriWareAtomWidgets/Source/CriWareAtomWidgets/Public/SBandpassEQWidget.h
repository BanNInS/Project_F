#pragma once

#include "Templates/SharedPointer.h"

#include "Atom/AtomBusEffectPreset.h"

namespace AtomWidgets
{
	// Forward Definitions
	class SAtomRadialSlider;

	class CRIWAREATOMWIDGETS_API SBandpassEQWidget
		: public SCompoundWidget
	{
	public:

		SLATE_BEGIN_ARGS(SBandpassEQWidget)
			: _Preset(nullptr)
			{}

			SLATE_ARGUMENT(UAtomBusEffectPreset*, Preset)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:

		TSharedPtr<SAtomRadialSlider> FreqencySlider;
		TSharedPtr<SAtomRadialSlider> GainSlider;
		TSharedPtr<SAtomRadialSlider> QSlider;

		TObjectPtr<UAtomBusEffectPreset> Preset;
		int16 PresetIndex = INDEX_NONE;
	};
} // namespace
