
#pragma once

#include "Containers/Map.h"
#include "Framework/Docking/TabManager.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Docking/SDockTab.h"

#include "CriWareDefines.h"

#include "Views/DashboardViewFactory.h"
#include "IAtomInsightsDashboardFactory.h"

namespace Atom::Insights
{
	class FDashboardFactory : public IDashboardFactory, public TSharedFromThis<FDashboardFactory>
	{
	public:

		FDashboardFactory() = default;
		virtual ~FDashboardFactory() = default;

		TSharedRef<SDockTab> MakeDockTabWidget(const FSpawnTabArgs& Args);

		virtual void RegisterViewFactory(TSharedRef<IDashboardViewFactory> InFactory) override;
		virtual void UnregisterViewFactory(FName InDashboardName) override;
		virtual FAtomRuntimeId GetRuntimeID() const override;

	private:

		TSharedRef<SWidget> MakeMenuBarWidget();

		TSharedPtr<FTabManager::FLayout> GetDefaultTabLayout();

		void RegisterTabSpawners();
		void UnregisterTabSpawners();

#if !WITH_EDITOR
		TSharedRef<SWidget> MakeEnableTracesOverlay();
		TSharedRef<SWidget> MakeEnableTracesButton();
		FReply ToggleAutoEnableAtomTraces();
#endif

		TSharedPtr<FTabManager> DashboardTabManager;
		TSharedPtr<FWorkspaceItem> DashboardWorkspace;
		TSharedPtr<FTabManager::FLayout> TabLayout;

		static constexpr FAtomRuntimeId ActiveRuntimeID = 1; // The default Atom runtime ID in standalone

		TMap<FName, TSharedPtr<IDashboardViewFactory>> DashboardViewFactories;
	};
} // namespace
