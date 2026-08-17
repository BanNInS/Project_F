
#include "Views/SubmixesDashboardViewFactory.h"

#include "Editor.h"
#include "Internationalization/Text.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/SharedPointer.h"

#include "CriWareDefines.h"
#include "Atom/AtomRuntimeManager.h"

#include "CriWareAtomInsightsEditorModule.h"
#include "AtomInsightsStyle.h"
#include "Providers/SubmixProvider.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"

#define LOCTEXT_NAMESPACE "AtomInsights"

namespace Atom::Insights
{
	namespace SubmixesPrivate
	{
		const FSubmixAssetDashboardEntry& CastEntry(const IDashboardDataViewEntry& InData)
		{
			return static_cast<const FSubmixAssetDashboardEntry&>(InData);
		};
	}

	FSubmixesDashboardViewFactory::FSubmixesDashboardViewFactory()
	{
		FSubmixProvider::OnSubmixAssetAdded.AddRaw(this, &FSubmixesDashboardViewFactory::HandleOnSubmixAssetListUpdated);
		FSubmixProvider::OnSubmixAssetRemoved.AddRaw(this, &FSubmixesDashboardViewFactory::HandleOnSubmixAssetListUpdated);
		FSubmixProvider::OnSubmixAssetListUpdated.AddRaw(this, &FSubmixesDashboardViewFactory::RequestListRefresh);

		IAtomInsightsTraceModule& AtomInsightsTraceModule = ICriWareAtomInsightsEditorModule::GetChecked().GetTraceModule();

		SubmixProvider = MakeShared<FSubmixProvider>();

		AtomInsightsTraceModule.AddTraceProvider(SubmixProvider);

		Providers = TArray<TSharedPtr<FTraceProviderBase>>
		{
			SubmixProvider
		};

		SortByColumn = "Name";
		SortMode     = EColumnSortMode::Ascending;
	}

	FSubmixesDashboardViewFactory::~FSubmixesDashboardViewFactory()
	{
		FSubmixProvider::OnSubmixAssetAdded.RemoveAll(this);
		FSubmixProvider::OnSubmixAssetRemoved.RemoveAll(this);
		FSubmixProvider::OnSubmixAssetListUpdated.RemoveAll(this);
	}

	FName FSubmixesDashboardViewFactory::GetName() const
	{
		return "Racks";
	}

	FText FSubmixesDashboardViewFactory::GetDisplayName() const
	{
		return LOCTEXT("AtomDashboard_Racks_DisplayName", "Racks");
	}

	TSharedRef<SWidget> FSubmixesDashboardViewFactory::GenerateWidgetForColumn(TSharedRef<IDashboardDataViewEntry> InRowData, const FName& InColumnName)
	{
		if (InColumnName == "Active")
		{
			static const FLinearColor DarkGreen(0.027f, 0.541f, 0.22f);
			static const float Radius = 4.0f;
			static const FVector2f Size(7.0f, 7.0f);
			static const FSlateRoundedBoxBrush GreenRoundedBrush(DarkGreen, Radius, Size);

			return SNew(SBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				.Padding(6.0f)
				.Visibility_Lambda([InRowData]()
				{
						const FSubmixAssetDashboardEntry& SubmixAssetDashboardEntry = SubmixesPrivate::CastEntry(InRowData.Get());
						return SubmixAssetDashboardEntry.bHasActivity ? EVisibility::Visible : EVisibility::Hidden;
				})
				[
					SNew(SImage)
					.Image(&GreenRoundedBrush)
				];
		}
		else if (InColumnName == "Name")
		{
			const FColumnData& ColumnData = GetColumns()[InColumnName];
			const FText ValueText = ColumnData.GetDisplayValue(InRowData.Get());

			if (ValueText.IsEmpty())
			{
				return SNullWidget::NullWidget;
			}

			const FSubmixAssetDashboardEntry& SubmixAssetDashboardEntry = SubmixesPrivate::CastEntry(*InRowData);
			const uint32 SubmixID = SubmixAssetDashboardEntry.SubmixID;

			const bool bInitValue = SubmixCheckboxCheckedStates.FindOrAdd(SubmixID);
			OnSubmixAssetInit.Broadcast(bInitValue, SubmixID, SubmixAssetDashboardEntry.Name);

			return SNew(SHorizontalBox)
				.Clipping(EWidgetClipping::ClipToBounds)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this, SubmixID]()
					{
						const bool* FoundSubmixCheckedState = SubmixCheckboxCheckedStates.Find(SubmixID);
						const bool bIsSubmixChecked = FoundSubmixCheckedState && *FoundSubmixCheckedState == true;

						return bIsSubmixChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this, InRowData](ECheckBoxState NewState)
					{
						const bool bIsChecked = NewState == ECheckBoxState::Checked;

						const FSubmixAssetDashboardEntry& SubmixAssetDashboardEntry = SubmixesPrivate::CastEntry(*InRowData);

						if (bool* FoundSubmixCheckedState = SubmixCheckboxCheckedStates.Find(SubmixAssetDashboardEntry.SubmixID))
						{
							*FoundSubmixCheckedState = bIsChecked;
						}

						OnSubmixAssetChecked.Broadcast(bIsChecked, SubmixAssetDashboardEntry.SubmixID, SubmixAssetDashboardEntry.Name);
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(5.0f)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(ValueText)
					.MinDesiredWidth(300)
					.OnDoubleClicked_Lambda([this, InRowData](const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
					{
						if (GEditor)
						{
							const TSharedPtr<IObjectDashboardEntry> ObjectData = StaticCastSharedPtr<IObjectDashboardEntry>(InRowData.ToSharedPtr());
							if (ObjectData.IsValid())
							{
								const TObjectPtr<UObject> Object = ObjectData->GetObject();
								if (Object && Object->IsAsset())
								{
									GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
									return FReply::Handled();
								}
							}
						}

						return FReply::Unhandled();
					})
				];
		}

		return SNullWidget::NullWidget;
	}

	void FSubmixesDashboardViewFactory::ProcessEntries(FTraceTableDashboardViewFactory::EProcessReason Reason)
	{
		const FString FilterString = GetSearchFilterText().ToString();
		
		FTraceTableDashboardViewFactory::FilterEntries<FSubmixProvider>([&FilterString](const IDashboardDataViewEntry& Entry)
		{
			const FSubmixAssetDashboardEntry& SubmixEntry = static_cast<const FSubmixAssetDashboardEntry&>(Entry);
			
			return !SubmixEntry.GetDisplayName().ToString().Contains(FilterString);
		});
	}

	FSlateIcon FSubmixesDashboardViewFactory::GetIcon() const
	{
		return FSlateStyle::Get().CreateIcon("AtomInsights.Icon.Submix");
	}

	EDefaultDashboardTabStack FSubmixesDashboardViewFactory::GetDefaultTabStack() const
	{
		return EDefaultDashboardTabStack::Analysis;
	}

	TSharedRef<SWidget> FSubmixesDashboardViewFactory::MakeWidget(TSharedRef<SDockTab> OwnerTab, const FSpawnTabArgs& SpawnTabArgs)
	{
		if (!DashboardWidget.IsValid())
		{
			DashboardWidget = FTraceTableDashboardViewFactory::MakeWidget(OwnerTab, SpawnTabArgs);

			if (FilteredEntriesListView.IsValid())
			{
				FilteredEntriesListView->SetSelectionMode(ESelectionMode::Single);
			}
		}
		else
		{
			if (SubmixProvider.IsValid())
			{
				SubmixProvider->RequestEntriesUpdate();
			}
		}

		for (const TSharedPtr<IDashboardDataViewEntry>& Entry : DataViewEntries)
		{
			const FSubmixAssetDashboardEntry& SubmixAssetDashboardEntry = SubmixesPrivate::CastEntry(*Entry);

			if (const bool* bFoundSubmixCheckboxCheckedState = SubmixCheckboxCheckedStates.Find(SubmixAssetDashboardEntry.SubmixID))
			{
				OnSubmixAssetInit.Broadcast(*bFoundSubmixCheckboxCheckedState, SubmixAssetDashboardEntry.SubmixID, SubmixAssetDashboardEntry.Name);
			}
		}

		return DashboardWidget->AsShared();
	}

	const TMap<FName, FTraceTableDashboardViewFactory::FColumnData>& FSubmixesDashboardViewFactory::GetColumns() const
	{
		auto CreateColumnData = []()
		{
			return TMap<FName, FTraceTableDashboardViewFactory::FColumnData>
			{
				{
					"Active",
					{
						LOCTEXT("Submixes_ActiveDisplayName", "Active"),
						[](const IDashboardDataViewEntry& InData) { return FText::GetEmpty(); },
						false,		/* bDefaultHidden */
						0.08f,		/* FillWidth */
						EHorizontalAlignment::HAlign_Center
					}
				},
				{
					"Name",
					{
						LOCTEXT("Submixes_NameColumnDisplayName", "Name"),
						[](const IDashboardDataViewEntry& InData) { return SubmixesPrivate::CastEntry(InData).GetDisplayName(); },
						false,		/* bDefaultHidden */
						0.88f,		/* FillWidth */
						EHorizontalAlignment::HAlign_Left
					}
				}
			};
		};
		
		static const TMap<FName, FTraceTableDashboardViewFactory::FColumnData> ColumnData = CreateColumnData();
		
		return ColumnData;
	}

	void FSubmixesDashboardViewFactory::SortTable()
	{
		if (SortByColumn == "Active")
		{
			if (SortMode == EColumnSortMode::Ascending)
			{
				DataViewEntries.Sort([](const TSharedPtr<IDashboardDataViewEntry>& A, const TSharedPtr<IDashboardDataViewEntry>& B)
					{
						const FSubmixAssetDashboardEntry& AData = SubmixesPrivate::CastEntry(*A.Get());
						const FSubmixAssetDashboardEntry& BData = SubmixesPrivate::CastEntry(*B.Get());

						return !AData.bHasActivity && BData.bHasActivity;
					});
			}
			else if (SortMode == EColumnSortMode::Descending)
			{
				DataViewEntries.Sort([](const TSharedPtr<IDashboardDataViewEntry>& A, const TSharedPtr<IDashboardDataViewEntry>& B)
					{
						const FSubmixAssetDashboardEntry& AData = SubmixesPrivate::CastEntry(*A.Get());
						const FSubmixAssetDashboardEntry& BData = SubmixesPrivate::CastEntry(*B.Get());

						return !BData.bHasActivity && AData.bHasActivity;
					});
			}
		}
		else if (SortByColumn == "Name")
		{
			if (SortMode == EColumnSortMode::Ascending)
			{
				DataViewEntries.Sort([](const TSharedPtr<IDashboardDataViewEntry>& A, const TSharedPtr<IDashboardDataViewEntry>& B)
				{
					const FSubmixAssetDashboardEntry& AData = SubmixesPrivate::CastEntry(*A.Get());
					const FSubmixAssetDashboardEntry& BData = SubmixesPrivate::CastEntry(*B.Get());

					return AData.GetDisplayName().CompareToCaseIgnored(BData.GetDisplayName()) < 0;
				});
			}
			else if (SortMode == EColumnSortMode::Descending)
			{
				DataViewEntries.Sort([](const TSharedPtr<IDashboardDataViewEntry>& A, const TSharedPtr<IDashboardDataViewEntry>& B)
				{
					const FSubmixAssetDashboardEntry& AData = SubmixesPrivate::CastEntry(*A.Get());
					const FSubmixAssetDashboardEntry& BData = SubmixesPrivate::CastEntry(*B.Get());

					return BData.GetDisplayName().CompareToCaseIgnored(AData.GetDisplayName()) < 0;
				});
			}
		}
	}

	void FSubmixesDashboardViewFactory::OnSelectionChanged(TSharedPtr<IDashboardDataViewEntry> SelectedItem, ESelectInfo::Type SelectInfo)
	{
		if (SelectedItem.IsValid())
		{
			const FSubmixAssetDashboardEntry& SubmixAssetDashboardEntry = SubmixesPrivate::CastEntry(*SelectedItem.Get());

			if (const TObjectPtr<UObject> LoadedSubmix = FSoftObjectPath(SubmixAssetDashboardEntry.Name).TryLoad())
			{
				OnSubmixSelectionChanged.Broadcast(LoadedSubmix);
			}
		}
	}

	void FSubmixesDashboardViewFactory::RequestListRefresh()
	{
		if (FilteredEntriesListView.IsValid())
		{
			FilteredEntriesListView->RequestListRefresh();
		}
	}

	void FSubmixesDashboardViewFactory::HandleOnSubmixAssetListUpdated(const uint32 InSubmixID)
	{
		RequestListRefresh();
	}
} // namespace

#undef LOCTEXT_NAMESPACE
