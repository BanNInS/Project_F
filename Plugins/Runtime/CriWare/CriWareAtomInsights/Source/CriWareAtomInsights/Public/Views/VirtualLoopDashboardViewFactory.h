
#pragma once

#include "Views/TableDashboardViewFactory.h"

namespace Atom::Insights
{
	class CRIWAREATOMINSIGHTS_API FVirtualLoopDashboardViewFactory : public FTraceObjectTableDashboardViewFactory
	{
	public:
		FVirtualLoopDashboardViewFactory();
		virtual ~FVirtualLoopDashboardViewFactory() = default;

		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;

#if WITH_EDITOR
		DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDebugDrawEntries, float /*InElapsed*/, const TArray<TSharedPtr<IDashboardDataViewEntry>>& /*InSelectedItems*/, FAtomRuntimeId /*InAtomRuntimeID*/);
		inline static FOnDebugDrawEntries OnDebugDrawEntries;
#endif // WITH_EDITOR

	protected:
		virtual void ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason) override;
		virtual const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& GetColumns() const override;

		virtual void SortTable() override;

#if WITH_EDITOR
		virtual bool IsDebugDrawEnabled() const override;
		virtual void DebugDraw(float InElapsed, const TArray<TSharedPtr<IDashboardDataViewEntry>>& InSelectedItems, FAtomRuntimeId RuntimeID) const override;
#endif // WITH_EDITOR
	};
} // namespace
