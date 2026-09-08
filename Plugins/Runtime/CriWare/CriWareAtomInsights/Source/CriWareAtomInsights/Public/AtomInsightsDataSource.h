
#pragma once

#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"

namespace Atom::Insights
{
	class CRIWAREATOMINSIGHTS_API IDashboardDataViewEntry : public TSharedFromThis<IDashboardDataViewEntry>
	{
	public:
		virtual ~IDashboardDataViewEntry() = default;

		virtual bool IsValid() const = 0;
	};

	class CRIWAREATOMINSIGHTS_API IDashboardDataTreeViewEntry : public TSharedFromThis<IDashboardDataTreeViewEntry>
	{
	public:
		virtual ~IDashboardDataTreeViewEntry() = default;

		virtual bool IsValid() const = 0;
		virtual bool ShouldInitExpandChildren() const = 0;
		virtual void ResetShouldInitExpandChildren() = 0;

		TArray<TSharedPtr<IDashboardDataTreeViewEntry>> Children;
		bool bIsExpanded = false;
	};
} // namespace
