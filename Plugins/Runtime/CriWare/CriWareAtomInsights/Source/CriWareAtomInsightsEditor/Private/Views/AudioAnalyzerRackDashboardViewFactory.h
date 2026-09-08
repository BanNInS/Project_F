
#pragma once

#include "Templates/SharedPointer.h"
#include "Views/DashboardViewFactory.h"

class UAtomRackWithParentBase;

namespace Atom::Insights
{
	class FSubmixAudioAnalyzerRack;

	class FAudioAnalyzerRackDashboardViewFactory : public IDashboardViewFactory, public TSharedFromThis<FAudioAnalyzerRackDashboardViewFactory>
	{
	public:
		virtual FName GetName() const override;
		virtual FText GetDisplayName() const override;
		virtual EDefaultDashboardTabStack GetDefaultTabStack() const override;
		virtual FSlateIcon GetIcon() const override;
		virtual TSharedRef<SWidget> MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs) override;

	private:
		void HandleOnActiveAtomRuntimeChanged();
		void HandleOnSubmixSelectionChanged(const TWeakObjectPtr<UObject> InSubmix);

		TSharedPtr<FSubmixAudioAnalyzerRack> SubmixAudioAnalyzerRack;
		TObjectPtr<UAtomRackBase> MainSubmix;
	};
} // namespace
