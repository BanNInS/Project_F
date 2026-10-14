
#pragma once

#include "Containers/Map.h"
#include "Templates/SharedPointer.h"

#include "Providers/ModulationMatrixTraceProvider.h"
#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	class IDashboardDataViewEntry;

	class FModulationMatrixDashboardViewFactory : public FTraceObjectTableDashboardViewFactory
	{
	public:
		FModulationMatrixDashboardViewFactory();
		virtual ~FModulationMatrixDashboardViewFactory() = default;

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;

		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

		virtual void ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason) override;

	private:
		virtual const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& GetColumns() const override;

		void CreateDefaultColumnData();
		void RegisterDelegates();

		TSharedRef<SWidget> MakeModulatingSourceTypeFilterWidget();

		void OnControlBusesAdded(const FModulationMatrixTraceProvider::BusIdToBusInfoMap& AddedControlBuses);
		void OnControlBusesRemoved(const TArray<FName>& RemovedControlBusesNames);
		void OnAtomRuntimeDestroyed(FAtomRuntimeId InRuntimeID);

		void FilterByModulatingSourceName();
		void FilterByModulatingSourceType();

		virtual void SortTable() override;
		virtual FSlateColor GetRowColor(const TSharedPtr<IDashboardDataViewEntry>& InRowDataPtr) override;

		enum class EModulatingSourceComboboxSelection : uint8
		{
			All,
			BusMixes,
			Generators
		};

		TSharedPtr<FModulationMatrixTraceProvider> ModulationMatrixTraceProvider;
		TMap<FName, FTraceTableDashboardViewFactory::FColumnData> ModulationMatrixColumnData;
		TArray<FName> ActiveBusNames;

		FDelegateHandle OnControlBusAddedHandle;
		FDelegateHandle OnControlBusRemovedHandle;
		FDelegateHandle OnDeviceDestroyedHandle;

		using FComboboxSelectionItem = TPair<EModulatingSourceComboboxSelection, FText>;
		TArray<TSharedPtr<FComboboxSelectionItem>> ModulatingSourceTypes;
		TSharedPtr<FComboboxSelectionItem> SelectedModulatingSourceType;
	};
} // namespace
