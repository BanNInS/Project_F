
#pragma once

#include "Containers/Map.h"
#include "Engine/World.h"
#include "Framework/Docking/TabManager.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SComboBox.h"

#include "Atom/Atom.h"

#include "Views/DashboardViewFactory.h"
#include "IAtomInsightsDashboardFactory.h"

namespace Atom::Insights
{
	class FEditorDashboardFactory : public IDashboardFactory, public TSharedFromThis<FEditorDashboardFactory>
	{
		class FSceneViewport;
		class FEditorViewportClient;

	public:

		FEditorDashboardFactory() = default;
		virtual ~FEditorDashboardFactory() = default;

		TSharedRef<SDockTab> MakeDockTabWidget(const FSpawnTabArgs& Args);

		virtual void RegisterViewFactory(TSharedRef<IDashboardViewFactory> InFactory) override;
		virtual void UnregisterViewFactory(FName InDashboardName) override;
		virtual FAtomRuntimeId GetRuntimeID() const override;

		DECLARE_MULTICAST_DELEGATE(FOnActiveAtomRuntimeChanged);
		inline static FOnActiveAtomRuntimeChanged OnActiveAtomRuntimeChanged;

	private:

		void OnWorldRegisteredToAtomRuntime(const UWorld* InWorld, FAtomRuntimeId InRuntimeID);
		void OnWorldUnregisteredFromAtomRuntime(const UWorld* InWorld, FAtomRuntimeId InRuntimeID);
	
		void OnRuntimeCreated(FAtomRuntimeId InRuntimeID);
		void OnRuntimeDestroyed(FAtomRuntimeId InRuntimeID);
		
		void OnPIEStarted(bool bSimulating);
		void OnPostPIEStarted(bool bSimulating);
		void OnPIEStopped(bool bSimulating);

		void RefreshRuntimeSelector();
		void ResetDelegates();

		TSharedRef<SWidget> MakeMenuBarWidget();
		TSharedRef<SWidget> MakeMainToolbarWidget();

		void StartTraceAnalysis(const TObjectPtr<const UWorld> InWorld, const FAtomRuntimeId InRuntimeID);
		void InitDelegates();
		TSharedRef<FTabManager::FLayout> GetDefaultTabLayout();

		void RegisterTabSpawners();
		void UnregisterTabSpawners();

		TSharedRef<FTabManager::FLayout> LoadLayoutFromConfig();
		void SaveLayoutToConfig();

		FDelegateHandle OnWorldRegisteredToAtomRuntimeHandle;
		FDelegateHandle OnWorldUnregisteredFromAtomRuntimeHandle;

		FDelegateHandle OnRuntimeCreatedHandle;
		FDelegateHandle OnRuntimeDestroyedHandle;

		FDelegateHandle OnPIEStartedHandle;
		FDelegateHandle OnPIEStoppedHandle;

		bool bOnlyTraceAtomChannels = false;

		TSharedPtr<FTabManager> DashboardTabManager;
		TSharedPtr<FWorkspaceItem> DashboardWorkspace;

		TArray<TSharedPtr<FAtomRuntimeId>>  AtomRuntimeIDs;
		TSharedPtr<SComboBox<TSharedPtr<FAtomRuntimeId>>> AtomRuntimeComboBox;

		FAtomRuntimeId ActiveRuntimeID = INDEX_NONE;

		TMap<FName, TSharedPtr<IDashboardViewFactory>> DashboardViewFactories;
	};
} // namespace
