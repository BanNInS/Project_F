
#pragma once

#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SCheckBox.h"

#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	class FControlBusDashboardViewFactory : public FTraceObjectTableDashboardViewFactory
	{
	public:
		FControlBusDashboardViewFactory();
		virtual ~FControlBusDashboardViewFactory();

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;
		virtual void ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason) override;
		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

	protected:
		virtual const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& GetColumns() const override;

		virtual void SortTable() override;

		virtual TSharedRef<SWidget> MakeControlBusListWidget();
		virtual TSharedRef<SWidget> MakeControlBusWatchWidget();

	};
} // namespace
