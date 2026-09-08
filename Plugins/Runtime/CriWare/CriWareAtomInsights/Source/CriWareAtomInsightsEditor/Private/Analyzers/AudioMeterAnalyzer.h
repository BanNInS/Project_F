// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

//Forward Definitions
namespace AtomWidgets { class FAtomLevelMeter; }
class UAtomAudioBus;
class UAtomRackBase;

namespace Atom::Insights
{
	class FAudioMeterAnalyzer : public TSharedFromThis<FAudioMeterAnalyzer>
	{
	public:

		explicit FAudioMeterAnalyzer(TWeakObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr);
		explicit FAudioMeterAnalyzer(TWeakObjectPtr<UAtomRackBase> InAtomRack);
		virtual ~FAudioMeterAnalyzer() = default;

		TSharedRef<AtomWidgets::FAtomLevelMeter> GetLevelMeter() { return LevelMeter; };

		void RebuildAudioMeter(TWeakObjectPtr<UAtomAudioBus> InExternalAudioBus = nullptr);
		void RebuildAudioMeter(TWeakObjectPtr<UAtomRackBase> InAtomRack);

	private:

		TSharedRef<AtomWidgets::FAtomLevelMeter> LevelMeter;
	};
} // namespace
