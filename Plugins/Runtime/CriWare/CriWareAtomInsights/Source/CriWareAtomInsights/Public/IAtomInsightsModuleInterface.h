
#pragma once

#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"

#include "CriWareDefines.h"

class IAtomInsightsTraceModule;

namespace Atom::Insights
{
	class IDashboardViewFactory;
	class FTraceProviderBase;
} // namespace

class CRIWAREATOMINSIGHTS_API IAtomInsightsModuleInterface : public IModuleInterface
{
public:
	virtual void RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory) = 0;
	virtual void UnregisterDashboardViewFactory(FName InName) = 0;

	virtual FAtomRuntimeId GetRuntimeID() const = 0;

	virtual IAtomInsightsTraceModule& GetTraceModule() = 0;
};
