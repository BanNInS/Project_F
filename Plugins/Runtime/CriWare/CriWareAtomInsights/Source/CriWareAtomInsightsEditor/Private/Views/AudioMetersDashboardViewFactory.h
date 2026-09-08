
#pragma once

#include "Templates/SharedPointer.h"
#include "Widgets/Layout/SScrollBox.h"

#include "AudioMeterView.h"
#include "Views/DashboardViewFactory.h"

class UAtomAudioBus;

namespace Atom::Insights
{
	class FAudioMeterAnalyzer;

	class FAudioMetersDashboardViewFactory : public IDashboardViewFactory, public TSharedFromThis<FAudioMetersDashboardViewFactory>
	{
	public:
		virtual ~FAudioMetersDashboardViewFactory() = default;

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

	private:
		void HandleOnSubmixAssetInit(const bool bInIsChecked, const uint32 InSubmixID, const FString& InSubmixName);
		void HandleOnAudioBusAssetInit(const bool bInIsChecked, const TWeakObjectPtr<UAtomAudioBus> InAudioBus);

		void HandleOnSubmixAssetChecked(const bool bInIsChecked, const uint32 InSubmixID, const FString& InSubmixName);
		void HandleOnAudioBusAssetChecked(const bool bInIsChecked, const TWeakObjectPtr<UAtomAudioBus> InAudioBus);

		void HandleOnSubmixAssetRemoved(const uint32 InSubmixID);
		void HandleOnAudioBusAssetRemoved(const TWeakObjectPtr<UObject> InAudioBusAsset);

		TSharedPtr<SScrollBox> MeterViewsScrollBox;
		TSharedPtr<SHorizontalBox> AudioMeterViewsContainer;
		TMap<uint32, TSharedRef<FAudioMeterView>> AudioMeterViews;

		FDelegateHandle OnSubmixAssetInit;
		FDelegateHandle OnAudioBusAssetInit;
		FDelegateHandle OnSubmixAssetCheckedHandle;
		FDelegateHandle OnAudioBusAssetCheckedHandle;
		FDelegateHandle OnSubmixAssetRemovedHandle;
		FDelegateHandle OnAudioBusAssetRemovedHandle;
	};
} // namespace
