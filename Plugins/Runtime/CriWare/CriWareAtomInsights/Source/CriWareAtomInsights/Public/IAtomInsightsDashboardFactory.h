
#pragma once

#include "UObject/NameTypes.h"
#include "Views/DashboardViewFactory.h"

#include "CriWareDefines.h"

namespace Atom::Insights
{
	class CRIWAREATOMINSIGHTS_API IDashboardFactory
	{
	public:
		virtual ~IDashboardFactory() = default;

		virtual void RegisterViewFactory(TSharedRef<IDashboardViewFactory> InFactory) = 0;
		virtual void UnregisterViewFactory(FName InDashboardName) = 0;
		virtual FAtomRuntimeId GetRuntimeID() const = 0;
	};
} // namespace
