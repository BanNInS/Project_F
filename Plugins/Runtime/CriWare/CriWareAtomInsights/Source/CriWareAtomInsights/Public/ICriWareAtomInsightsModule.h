
#pragma once

#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"

#include "IAtomInsightsModuleInterface.h"

class IAtomInsightsTraceModule;

namespace Atom::Insights
{
	class IDashboardViewFactory;
	class FTraceProviderBase;
} // namespace

using FAtomRuntimeId = uint32;

class CRIWAREATOMINSIGHTS_API ICriWareAtomInsightsModule : public IAtomInsightsModuleInterface
{
public:
	virtual void RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory) = 0;
	virtual void UnregisterDashboardViewFactory(FName InName) = 0;

	virtual FAtomRuntimeId GetRuntimeID() const = 0;

	virtual IAtomInsightsTraceModule& GetTraceModule() override;

	virtual TSharedRef<SDockTab> CreateDashboardTabWidget(const FSpawnTabArgs& Args) = 0;

	static ICriWareAtomInsightsModule& GetChecked();

#ifdef WITH_EDITOR
	static ICriWareAtomInsightsModule& GetEditorChecked();
#endif

	static FName GetName()
	{
		const FLazyName ModuleName = "CriWareAtomInsights";
		return ModuleName.Resolve();
	}
};
