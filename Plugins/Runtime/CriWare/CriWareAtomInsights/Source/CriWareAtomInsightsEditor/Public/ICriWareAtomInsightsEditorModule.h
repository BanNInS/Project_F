
#pragma once

#include "UObject/NameTypes.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Docking/SDockTab.h"

#include "CriWareDefines.h"

#include "IAtomInsightsModuleInterface.h"

class IAtomInsightsTraceModule;
class SDockTab;

namespace Atom::Insights
{
	class IDashboardViewFactory;
	class FTraceProviderBase;
} // namespace

class ICriWareAtomInsightsEditorModule : public IAtomInsightsModuleInterface
{
public:
	CRIWAREATOMINSIGHTSEDITOR_API virtual void RegisterDashboardViewFactory(TSharedRef<Atom::Insights::IDashboardViewFactory> InDashboardFactory) = 0;
	CRIWAREATOMINSIGHTSEDITOR_API virtual void UnregisterDashboardViewFactory(FName InName) = 0;

	CRIWAREATOMINSIGHTSEDITOR_API virtual FAtomRuntimeId GetRuntimeID() const = 0;

	CRIWAREATOMINSIGHTSEDITOR_API virtual IAtomInsightsTraceModule& GetTraceModule() override;

	CRIWAREATOMINSIGHTSEDITOR_API virtual TSharedRef<SDockTab> CreateDashboardTabWidget(const FSpawnTabArgs& Args) = 0;

	CRIWAREATOMINSIGHTSEDITOR_API static bool IsModuleLoaded();

	CRIWAREATOMINSIGHTSEDITOR_API static ICriWareAtomInsightsEditorModule& GetChecked();
};
