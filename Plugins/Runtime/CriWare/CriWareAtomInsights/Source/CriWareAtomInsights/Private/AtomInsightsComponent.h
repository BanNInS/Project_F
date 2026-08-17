
#pragma once

#include "Containers/Ticker.h"
#include "Insights/IUnrealInsightsModule.h"

namespace Atom::Insights
{
	/**
	 * The component that makes Atom Insights available inside Unreal Insights.
	 */
	class FAtomInsightsComponent : public IInsightsComponent, public TSharedFromThis<FAtomInsightsComponent>
	{
	public:
		FAtomInsightsComponent() = default;
		virtual ~FAtomInsightsComponent();

		static TSharedPtr<FAtomInsightsComponent> CreateInstance();

		// IInsightsComponent overrides
		virtual void Initialize(IUnrealInsightsModule& InsightsModule) override;
		virtual void Shutdown() override;
		virtual void RegisterMajorTabs(IUnrealInsightsModule& InsightsModule) override;
		virtual void UnregisterMajorTabs() override;

		bool GetIsLiveSession() const;
		bool GetIsEditorTrace() const { return bIsEditorTrace; }

		DECLARE_MULTICAST_DELEGATE(FOnTabSpawn);
		inline static FOnTabSpawn OnTabSpawn;

	private:
		bool CanSpawnTab(const FSpawnTabArgs& Args) const;
		TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args);

		bool Tick(float DeltaTime);

		bool bIsInitialized = false;
		bool bIsEditorTrace = false;
		bool bCanSpawnTab   = false;

		FTickerDelegate OnTick;
		FTSTicker::FDelegateHandle OnTickHandle;

		inline static TSharedPtr<FAtomInsightsComponent> Instance;
	};
} // namespace
