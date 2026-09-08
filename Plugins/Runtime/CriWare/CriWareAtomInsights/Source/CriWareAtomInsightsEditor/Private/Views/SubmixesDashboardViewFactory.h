
#pragma once

#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SCheckBox.h"

#include "Views/TableDashboardViewFactory.h"

class UAtomRackBase;
class UAtomBus;

namespace Atom::Insights
{
	class FSubmixProvider;

	class FSubmixesDashboardViewFactory : public FTraceObjectTableDashboardViewFactory
	{
	public:
		FSubmixesDashboardViewFactory();
		virtual ~FSubmixesDashboardViewFactory();

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;
		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

		DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSubmixAssetInit, const bool /*bIsChecked*/, const uint32 /*SubmixID*/, const FString& /*SubmixName*/);
		inline static FOnSubmixAssetInit OnSubmixAssetInit;

		DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSubmixAssetChecked, const bool /*bIsChecked*/, const uint32 /*SubmixID*/, const FString& /*RSubmixName*/);
		inline static FOnSubmixAssetChecked OnSubmixAssetChecked;

		DECLARE_MULTICAST_DELEGATE_OneParam(FOnSubmixSelectionChanged, const TWeakObjectPtr<UObject> /*AtomRack or AtomBus*/);
		inline static FOnSubmixSelectionChanged OnSubmixSelectionChanged;

	protected:
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(TSharedRef<IDashboardDataViewEntry> InRowData, const FName& InColumnName) override;
		virtual void ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason) override;
		virtual const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& GetColumns() const override;

		virtual void SortTable() override;

		virtual void OnSelectionChanged(TSharedPtr<IDashboardDataViewEntry> SelectedItem, ESelectInfo::Type SelectInfo) override;

		void RequestListRefresh();

		void HandleOnSubmixAssetListUpdated(const uint32 InSubmixID);

		TSharedPtr<FSubmixProvider> SubmixProvider;
		TMap<uint32, bool> SubmixCheckboxCheckedStates;
	};
} // namespace UE::Audio::Insights
