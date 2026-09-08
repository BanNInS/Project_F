// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SWidget.h"
#include "Widgets/Text/STextBlock.h"

class UAtomAudioBus;
class UAtomRackWithParentBase;
class UAtomBus;

namespace Atom::Insights
{
	class FAudioMeterAnalyzer;
	class FAudioMeterRackAnalyzer;
	class FAudioMeterBusAnalyzer;

	class FAudioMeterView : public TSharedFromThis<FAudioMeterView>
	{
	public:
		using FAudioAssetVariant = TVariant<TWeakObjectPtr<UAtomAudioBus>, TWeakObjectPtr<UAtomRackWithParentBase>, TWeakObjectPtr<UAtomBus>>;
		using FAudioMeterVariant = TVariant<TSharedPtr<FAudioMeterAnalyzer>, TSharedPtr<FAudioMeterRackAnalyzer>, TSharedPtr<FAudioMeterBusAnalyzer>>;

		FAudioMeterView(FAudioAssetVariant InAudioAssetVariant);
		virtual ~FAudioMeterView();

		TSharedRef<SWidget> GetWidget() const { return AudioMeterViewWidget; };

	private:
		FAudioMeterVariant MakeAudioMeterAnalyzerVariant(const FAudioAssetVariant InAudioAssetVariant);
		TSharedRef<STextBlock> MakeAudioAssetNameTextBlock(const FAudioAssetVariant InAudioAssetVariant);
		TSharedRef<SWidget> MakeWidget();

		TSharedRef<SWidget> GetAudioMeterAnalyzerWidget() const;

		void HandleOnActiveAtomRuntimeChanged();

		FAudioAssetVariant AudioAssetVariant;
		FAudioMeterVariant AudioMeterAnalyzerVariant;

		TSharedRef<STextBlock> AudioAssetNameTextBlock;

		TSharedRef<SWidget> AudioMeterViewWidget;

		FDelegateHandle OnActiveAtomRuntimeChangedHandle;
	};
} // namespace UE::Audio::Insights
