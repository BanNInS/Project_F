
#pragma once

#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SCheckBox.h"

#include "Views/TableDashboardViewFactory.h"

class UAtomAudioBus;

namespace Atom::Insights
{
	class FAudioBusProvider;

	class FAudioBusesDashboardViewFactory : public FTraceObjectTableDashboardViewFactory
	{
	public:
		FAudioBusesDashboardViewFactory();
		virtual ~FAudioBusesDashboardViewFactory();

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;
		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

		DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBusAssetInit, const bool /*bIsChecked*/, TWeakObjectPtr<UAtomAudioBus> /*AudioBus*/);
		inline static FOnBusAssetInit OnBusAssetInit;

		DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAudioBusAssetChecked, const bool /*bIsChecked*/, const TWeakObjectPtr<UAtomAudioBus> /*AudioBus*/);
		inline static FOnAudioBusAssetChecked OnAudioBusAssetChecked;

	protected:

		enum class EAudioBusTypeComboboxSelection : uint8
		{
			AssetBased,
			CodeGenerated,
			All
		};

		TSharedRef<SWidget> MakeAudioBusTypeFilterWidget();

		virtual TSharedRef<SWidget> GenerateWidgetForColumn(TSharedRef<IDashboardDataViewEntry> InRowData, const FName& InColumnName) override;
		virtual void ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason) override;
		virtual const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& GetColumns() const override;

		virtual void SortTable() override;

		void FilterByAudioBusName();
		void FilterByAudioBusType();

		void RequestListRefresh();

		void HandleOnAudioBusAssetListUpdated(const TWeakObjectPtr<UObject> InAsset);

		TSharedPtr<FAudioBusProvider> AudioBusProvider;
		TMap<const TWeakObjectPtr<UAtomAudioBus>, bool> AudioBusCheckboxCheckedStates;

		using FComboboxSelectionItem = TPair<EAudioBusTypeComboboxSelection, FText>;
		TArray<TSharedPtr<FComboboxSelectionItem>> AudioBusTypes;
		TSharedPtr<FComboboxSelectionItem> SelectedAudioBusType;
	};
} // namespace
