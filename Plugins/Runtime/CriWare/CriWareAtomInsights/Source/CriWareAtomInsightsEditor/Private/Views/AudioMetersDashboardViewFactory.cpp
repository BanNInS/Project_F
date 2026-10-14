// Copyright Epic Games, Inc. All Rights Reserved.
#include "AudioMetersDashboardViewFactory.h"

#include "Atom/AtomAudioBus.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"

#include "AtomInsightsDashboardFactory.h"
#include "AtomInsightsStyle.h"
#include "Providers/AudioBusProvider.h"
#include "Providers/SubmixProvider.h"
#include "Views/AudioBusesDashboardViewFactory.h"
#include "Views/SubmixesDashboardViewFactory.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	FName FAudioMetersDashboardViewFactory::GetName() const
	{
		return "AudioMeters";
	}

	FText FAudioMetersDashboardViewFactory::GetDisplayName() const
	{
		return LOCTEXT("AtomDashboard_AudioMetersTab_DisplayName", "Audio Meters");
	}

	EDefaultDashboardTabStack FAudioMetersDashboardViewFactory::GetDefaultTabStack() const
	{
		return EDefaultDashboardTabStack::AudioMeters;
	}

	FSlateIcon FAudioMetersDashboardViewFactory::GetIcon() const
	{
		return FSlateStyle::Get().CreateIcon("AtomInsights.Icon.Submix");
	}

	TSharedRef<SWidget> FAudioMetersDashboardViewFactory::MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs)
	{
		if (!OnSubmixAssetInit.IsValid())
		{
			OnSubmixAssetInit = FSubmixesDashboardViewFactory::OnSubmixAssetInit.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnSubmixAssetInit);
		}

		if (!OnAudioBusAssetInit.IsValid())
		{
			OnAudioBusAssetInit = FAudioBusesDashboardViewFactory::OnBusAssetInit.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetInit);
		}

		if (!OnSubmixAssetCheckedHandle.IsValid())
		{
			OnSubmixAssetCheckedHandle = FSubmixesDashboardViewFactory::OnSubmixAssetChecked.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnSubmixAssetChecked);
		}

		if (!OnAudioBusAssetCheckedHandle.IsValid())
		{
			OnAudioBusAssetCheckedHandle = FAudioBusesDashboardViewFactory::OnAudioBusAssetChecked.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetChecked);
		}

		if (!OnSubmixAssetRemovedHandle.IsValid())
		{
			OnSubmixAssetRemovedHandle = FSubmixProvider::OnSubmixAssetRemoved.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnSubmixAssetRemoved);
		}

		if (!OnAudioBusAssetRemovedHandle.IsValid())
		{
			OnAudioBusAssetRemovedHandle = FAudioBusProvider::OnAudioBusAssetRemoved.AddSP(this, &FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetRemoved);
		}
		
		if (!MeterViewsScrollBox.IsValid())
		{
			SAssignNew(MeterViewsScrollBox, SScrollBox)
			.Orientation(Orient_Horizontal)
			+ SScrollBox::Slot()
			[
				SAssignNew(AudioMeterViewsContainer, SHorizontalBox)
			];
		}

		return MeterViewsScrollBox.ToSharedRef();
	}

	void FAudioMetersDashboardViewFactory::HandleOnSubmixAssetInit(const bool bInIsChecked, const uint32 InSubmixID, const FString& InSubmixName)
	{
		if (!bInIsChecked)
		{
			return;
		}

		if (!AudioMeterViews.Contains(InSubmixID))
		{
			return;
		}

		const TObjectPtr<UObject> LoadedSubmix = FSoftObjectPath(InSubmixName).TryLoad();
		if (LoadedSubmix == nullptr)
		{
			return;
		}

		// Tear down and re-create meter analyzers on initialization to ensure we receive the correct volume data
		const int32 SlotIndex = AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[InSubmixID]->GetWidget());

		AudioMeterViews.Remove(InSubmixID);

		if (UAtomRackWithParentBase* Rack = Cast<UAtomRackWithParentBase>(LoadedSubmix))
		{
			AudioMeterViews.Add(InSubmixID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomRackWithParentBase>>(), Rack)));
		}
		else if (UAtomBus* Bus = Cast<UAtomBus>(LoadedSubmix))
		{
			AudioMeterViews.Add(InSubmixID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomBus>>(), Bus)));
		}
		else
		{
			return;
		}

		AudioMeterViewsContainer->InsertSlot(SlotIndex)
			.AutoWidth()
			.Padding(10.0f, 0.0f, 10.0f, 0.0f)
			[
				AudioMeterViews[InSubmixID]->GetWidget()
			];

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}

	void FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetInit(const bool bInIsChecked, const TWeakObjectPtr<UAtomAudioBus> InAudioBus)
	{
		if (!bInIsChecked || !InAudioBus.IsValid())
		{
			return;
		}

		const uint32 AudioBusUniqueID = InAudioBus->GetUniqueID();
		if (!AudioMeterViews.Contains(AudioBusUniqueID))
		{
			return;
		}

		// Tear down and re-create meter analyzers on initialization to ensure we receive the correct volume data
		const int32 SlotIndex = AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[AudioBusUniqueID]->GetWidget());

		AudioMeterViews.Remove(AudioBusUniqueID);
		AudioMeterViews.Add(AudioBusUniqueID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomAudioBus>>(), InAudioBus)));

		AudioMeterViewsContainer->InsertSlot(SlotIndex)
			.AutoWidth()
			.Padding(10.0f, 0.0f, 10.0f, 0.0f)
			[
				AudioMeterViews[AudioBusUniqueID]->GetWidget()
			];

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}

	void FAudioMetersDashboardViewFactory::HandleOnSubmixAssetChecked(const bool bInIsChecked, const uint32 InSubmixID, const FString& InSoundSubmixName)
	{
		if (bInIsChecked)
		{
			const TObjectPtr<UObject> LoadedSubmix = FSoftObjectPath(InSoundSubmixName).TryLoad();
			if (LoadedSubmix == nullptr)
			{
				return;
			}


			if (UAtomRackWithParentBase* Rack = Cast<UAtomRackWithParentBase>(LoadedSubmix))
			{
				AudioMeterViews.Add(InSubmixID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomRackWithParentBase>>(), Rack)));
			}
			else if (UAtomBus* Bus = Cast<UAtomBus>(LoadedSubmix))
			{
				AudioMeterViews.Add(InSubmixID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomBus>>(), Bus)));
			}
			else
			{
				return;
			}

			AudioMeterViewsContainer->AddSlot()
				.AutoWidth()
				.Padding(10.0f, 0.0f, 10.0f, 0.0f)
				[
					AudioMeterViews[InSubmixID]->GetWidget()
				];
		}
		else
		{
			if (AudioMeterViews.Contains(InSubmixID))
			{
				AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[InSubmixID]->GetWidget());
				AudioMeterViews.Remove(InSubmixID);
			}
		}

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}

	void FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetChecked(const bool bInIsChecked, const TWeakObjectPtr<UAtomAudioBus> InAudioBus)
	{
		if (!InAudioBus.IsValid())
		{
			return;
		}

		const uint32 AudioBusUniqueID = InAudioBus->GetUniqueID();

		if (bInIsChecked)
		{
			AudioMeterViews.Add(AudioBusUniqueID, MakeShared<FAudioMeterView>(FAudioMeterView::FAudioAssetVariant(TInPlaceType<TWeakObjectPtr<UAtomAudioBus>>(), InAudioBus)));

			AudioMeterViewsContainer->AddSlot()
			.AutoWidth()
			.Padding(10.0f, 0.0f, 10.0f, 0.0f)
			[
				AudioMeterViews[AudioBusUniqueID]->GetWidget()
			];
		}
		else
		{
			if (AudioMeterViews.Contains(AudioBusUniqueID))
			{
				AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[AudioBusUniqueID]->GetWidget());
				AudioMeterViews.Remove(AudioBusUniqueID);
			}
		}

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}

	void FAudioMetersDashboardViewFactory::HandleOnSubmixAssetRemoved(const uint32 InSubmixID)
	{
		if (AudioMeterViews.Contains(InSubmixID))
		{
			AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[InSubmixID]->GetWidget());
			AudioMeterViews.Remove(InSubmixID);
		}

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}

	void FAudioMetersDashboardViewFactory::HandleOnAudioBusAssetRemoved(const TWeakObjectPtr<UObject> InAudioBusAsset)
	{
		if (!InAudioBusAsset.IsValid())
		{
			return;
		}

		if (const uint32 AssetUniqueID = InAudioBusAsset->GetUniqueID();
			AudioMeterViews.Contains(AssetUniqueID))
		{
			AudioMeterViewsContainer->RemoveSlot(AudioMeterViews[AssetUniqueID]->GetWidget());
			AudioMeterViews.Remove(AssetUniqueID);
		}

		if (MeterViewsScrollBox.IsValid())
		{
			MeterViewsScrollBox->Invalidate(EInvalidateWidget::Layout);
		}
	}
} // namespace

#undef LOCTEXT_NAMESPACE
