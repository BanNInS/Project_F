/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomConfigFactoryDialog.h
 *
 ****************************************************************************/

#include "SAtomStatisticsDialog.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"

#include "Math/UnitConversion.h"
#include "Math/BasicMathExpressionEvaluator.h"
#include "Misc/ExpressionParser.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/FastDecimalFormat.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomListener.h"
#include "Atom/AtomConcurrency.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWaveBank.h"

#define LOCTEXT_NAMESPACE "CriWareCoreEditor"

const FName SAtomStatisticsDialog::ColumnID_Expander(TEXT("Expand"));
const FName SAtomStatisticsDialog::ColumnID_Active(TEXT("Active"));
const FName SAtomStatisticsDialog::ColumnID_Label(TEXT("Label"));
const FName SAtomStatisticsDialog::ColumnID_Value(TEXT("Value"));
const FName SAtomStatisticsDialog::ColumnID_Type(TEXT("Type"));
const FName SAtomStatisticsDialog::ColumnID_ID(TEXT("ID"));


void SAtomStatisticsDialog::Construct(const FArguments& InArgs)
{
	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColour = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();

	//RuntimeManager->GetActiveRuntime().name

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(0, 20, 0, 0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)		
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
				.ColorAndOpacity(TitleColour)
				.Font(TitleFont)
				.Justification(ETextJustify::Left)
				.Text( FText::FromString(TEXT("Atom Runtimes")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		.Expose(GridSlot)
		[
			GetGridPanel()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 20, 0, 0)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
			.ColorAndOpacity(TitleColour)
			.Font(TitleFont)
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(TEXT("Atom Runtime Outliner")))
		]
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Top)
		.HAlign(HAlign_Fill)
		.Padding(0, 5, 0, 0)
		.Expose(TreeSlot)
		[
			ConstructOutlinerTreeView()
		]
	];

	// Refresh every 1/2 seconds
	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomStatisticsDialog::UpdateGridPanels));
	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomStatisticsDialog::UpdateOutlinerTreeView));
}

EActiveTimerReturnType SAtomStatisticsDialog::UpdateGridPanels(double InCurrentTime, float InDeltaTime)
{
	(*GridSlot)
	[
		GetGridPanel()
	];

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SAtomStatisticsDialog::GetGridPanel()
{
	if (!GCriWare)
	{
		return SNullWidget::NullWidget;
	}

	/*TSharedRef<FDerivedDataCacheStatsNode> RootUsage = GetDerivedDataCache()->GatherUsageStats();

	TArray<TSharedRef<const FDerivedDataCacheStatsNode>> LeafUsageStats;
	RootUsage->ForEachDescendant([&LeafUsageStats](TSharedRef<const FDerivedDataCacheStatsNode> Node) {
		if (Node->Children.Num() == 0)
		{
			LeafUsageStats.Add(Node);
		}
	});*/

	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();

	TSharedRef<SGridPanel> Panel = SNew(SGridPanel);

	int32 Row = 0;

	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColor = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	//Panel->AddSlot(0, Row)
	//[
	//   Active
	//];

	//Panel->AddSlot(0, Row)
	//[
	//   Audible (not muted)
	//];

	Panel->AddSlot(2, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("RuntimeID", "ID"))
	];

	Panel->AddSlot(3, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("RuntimeScope", "Scope"))
	];

	Panel->AddSlot(4, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("RuntimeMaxSources", "Max Sources"))
	];

	Panel->AddSlot(5, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("ActiveSound", "Active Sounds"))
	];

	Panel->AddSlot(6, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("VirtualSounds", "Virtual Sounds"))
	];

	Panel->AddSlot(7, Row)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Worlds", "Worlds"))
		];

	RuntimeManager->IterateOverAllRuntimesForStatistics([&](FAtomRuntimeId RuntimeID, FAtomRuntime* Runtime, int32 NumberOfHandles, EAtomRuntimeScope Scope)
	{
		Row++;

		if (Runtime->IsActive())
		{
			Panel->AddSlot(0, Row)
			.HAlign(HAlign_Left)
			.Padding(ColumnMargin, RowMargin, 0.0f, RowMargin)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Sequencer.Tracks.Audio"))
				.ToolTipText(LOCTEXT("ActiveRuntime","Runtime is Active."))
			];
		}

		/*if (!Runtime->IsAtomMuted())
		{
			Panel->AddSlot(1, Row)
			.HAlign(HAlign_Left)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("Sequencer.Tracks.Audio"))
				.ToolTipText(LOCTEXT("RuntimeAudible","Runtime is not silent."))
			];
		}*/

		Panel->AddSlot(2, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("#%u"), RuntimeID)))
		];

		Panel->AddSlot(3, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Scope == EAtomRuntimeScope::Unique ? TEXT("Unique") : Scope == EAtomRuntimeScope::Shared ? TEXT("Shared") : TEXT("Undefined")))
		];

		Panel->AddSlot(4, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Center)
			.Text(FText::FromString(FString::Printf(TEXT("%u(+%u)"), Runtime->GetMaxChannels(), Runtime->GetMaxSources() - Runtime->GetMaxChannels())))
		];

		Panel->AddSlot(5, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("%u/%u"), Runtime->GetActiveSounds().Num(), Runtime->GetMaxSources())))
		];

		Panel->AddSlot(6, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("%u"), Runtime->GetVirtualLoops().Num())))
		];

		TArray<UWorld*> Worlds = RuntimeManager->GetWorldsUsingAtomRuntime(RuntimeID);
		FString WorldsStr;
		for (auto World : Worlds)
		{
			if (World->IsValidLowLevel() && World->GetFName().IsValid())
				WorldsStr += FString::Printf(TEXT("%s(%u), "), *World->GetFName().ToString(), World->GetUniqueID());
		}

		Panel->AddSlot(7, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(WorldsStr))
		];
	});

	return Panel;

	/*double SumTotalGetMB = 0.0;
	double SumTotalPutMB = 0.0;

	for (TSharedRef<const FDerivedDataCacheStatsNode> Node : LeafUsageStats)
	{
		const FDerivedDataBackendInterface* Backend = Node->GetBackendInterface();

		if (Backend->GetDisplayName().Equals("Memory") || Backend->IsWrapper())
			continue;

		TSharedRef<FDerivedDataCacheStatsNode> Usage = Backend->GatherUsageStats();
		FDerivedDataCacheUsageStats Stats;

		for (const auto& KVP : Usage->Stats)
		{
			Stats.Combine(KVP.Value);
		}

		
		const int64 TotalGetBytes = Stats.GetStats.GetAccumulatedValueAnyThread(FCookStats::CallStats::EHitOrMiss::Hit, FCookStats::CallStats::EStatType::Bytes);
		const int64 TotalPutBytes = Stats.PutStats.GetAccumulatedValueAnyThread(FCookStats::CallStats::EHitOrMiss::Hit, FCookStats::CallStats::EStatType::Bytes);
		const int64 TotalPrefetchBytes = Stats.PrefetchStats.GetAccumulatedValueAnyThread(FCookStats::CallStats::EHitOrMiss::Hit, FCookStats::CallStats::EStatType::Bytes);

		const int64 TotalGets_Hit = Stats.GetStats.GetAccumulatedValueAnyThread(FCookStats::CallStats::EHitOrMiss::Hit, FCookStats::CallStats::EStatType::Counter);
		const int64 TotalGets_Miss = Stats.GetStats.GetAccumulatedValueAnyThread(FCookStats::CallStats::EHitOrMiss::Miss, FCookStats::CallStats::EStatType::Counter);

		const int64 TotalRequests = TotalGets_Hit + TotalGets_Miss;

		Panel->AddSlot(0, Row)
			.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Backend->GetDisplayName()))
		];

		Panel->AddSlot(1, Row)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text( FText::FromString((Backend->GetSpeedClass() == FDerivedDataBackendInterface::ESpeedClass::Local) ? "Local" : "Remote" ))
		];

		const double HitPercentage = TotalRequests > 0 ? (100.0 * (TotalGets_Hit / (double)TotalRequests)) : 0.0;

		Panel->AddSlot(2, Row)
			.HAlign(HAlign_Left)
			[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(SingleDecimalFormat(HitPercentage) + TEXT(" %")))
			];

		const double TotalGetMB = FUnitConversion::Convert(TotalGetBytes, EUnit::Bytes, EUnit::Megabytes);

		SumTotalGetMB += TotalGetMB;

		Panel->AddSlot(3, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(SingleDecimalFormat(TotalGetMB) + TEXT(" MB")))
		];

		const double TotalPutMB = FUnitConversion::Convert(TotalPutBytes, EUnit::Bytes, EUnit::Megabytes);

		SumTotalPutMB += TotalPutMB;

		Panel->AddSlot(4, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(SingleDecimalFormat(TotalPutMB) + TEXT(" MB")))
		];

		Panel->AddSlot(5, Row)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Right)
			.Text(FText::FromString(SingleDecimalFormat(TotalPutMB) + TEXT(" MB")))
		];

		Panel->AddSlot(6, Row)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Backend->GetName()))
		];

		Row++;
	}

	Panel->AddSlot(0, Row)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.ColorAndOpacity(TitleColor)
			.Font(TitleFont)
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(TEXT("Total")))	
		];

	Panel->AddSlot(3, Row)
		.HAlign(HAlign_Right)
		[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Right)
		.Text(FText::FromString(SingleDecimalFormat(SumTotalGetMB) + TEXT(" MB")))
		];

	Panel->AddSlot(4, Row)
		.HAlign(HAlign_Right)
		[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Right)
		.Text(FText::FromString(SingleDecimalFormat(SumTotalPutMB) + TEXT(" MB")))
		];

	return Panel;*/
}

EActiveTimerReturnType SAtomStatisticsDialog::UpdateOutlinerTreeView(double InCurrentTime, float InDeltaTime)
{
	/*(*TreeSlot)
	[
		GetTreeView()
	];*/
	UpdateOutlinerTreeViewItems();

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}


void SAtomStatisticsDialog::UpdateOutlinerTreeViewItems()
{
	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();

	// build list - todo: incremental list
	Items.Empty();
	RuntimeManager->IterateOverAllRuntimesForStatistics([&](FAtomRuntimeId RuntimeID, FAtomRuntime* Runtime, int32 NumberOfHandles, EAtomRuntimeScope Scope)
	{
		auto Config = Runtime->GetAtomConfiguration();
		FString ConfigName = FString::Format(TEXT("Config: {0}"), { Config ? Config->GetFName().ToString() : TEXT("none") });
		auto RuntimeItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::Runtime, RuntimeID, TEXT("Runtime"), Runtime->IsActive(), ConfigName);
		Items.Add(RuntimeItem);
		TreeView->SetItemExpansion(RuntimeItem, true);

		// listeners -> should on audio thread
		for (auto& Listener : Runtime->GetListeners())
		{
			auto ListenerItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::Listener, Listener.ListenerIndex, TEXT("Listener"), false);
			RuntimeItem->Children.Add(ListenerItem);
		}

		// voice pools
		Runtime->IterateOverVoicePools([&](EAtomVoicePoolCodecType CodecType, EAtomVoicePoolStreamingType StreamingType, int32 ID, int32 NumUsedVoices, int32 MaxVoices, int32 MaxChannels, int32 MaxSampleRate)
		{
			FString PoolName = FString::Format(TEXT("{0} ({1})"), { ::ToString(CodecType), ::ToString(StreamingType) });
			if (ID == Runtime->GetAtomDefaultVoicePoolIdentifier()) { PoolName = TEXT("[") + PoolName + TEXT("]"); }
			FString PoolDetails = FString::Format(TEXT(" ({0}ch {1}Hz)"), { FString::FromInt(MaxChannels), FString::FromInt(MaxSampleRate) });
			FString Label = FString::Format(TEXT("{0} {1}"), { PoolName, MaxChannels >= 0 ? PoolDetails : TEXT("")});
			FString Value = FString::Format(TEXT("{0}/{1} voices"), { FString::FromInt(NumUsedVoices), FString::FromInt(MaxVoices) });
			auto VoicePoolItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::VoicePool, ID, Label, false, Value);
			RuntimeItem->Children.Add(VoicePoolItem);
		});

		// racks
		auto AddRackWithChildren = [&](const UAtomRackWithParentBase* Rack, auto ParentItem)
		{
			auto AddRackWithChildren_Impl = [&](const UAtomRackWithParentBase* Rack, auto ParentItem, auto& AddRackWithChildren_Ref) -> void
			{
				if (Rack)
				{
					FString RendererType = ::ToString(EAtomSoundRendererType::Asr);
					if (auto EndPoint = Cast<UAtomEndpointRack>(Rack->ParentRack))
					{
						RendererType = ::ToString(EndPoint->SoundRendererType);
					}

					// end point cannot be used yet for Spatial rack ...
					if (auto Soundfield = Cast<UAtomSoundfieldRack>(Rack))
					{
						RendererType = ::ToString(Soundfield->GetRackType());
					}

					FString OutputPort = TEXT("");
					TArray<FName> OutputPortNames = Runtime->GetMappedOutputPortNames(Rack);
					if (!OutputPortNames.IsEmpty())
					{
						OutputPort += TEXT(" [");
						for (FName& Name : OutputPortNames)
						{
							OutputPort += Name.ToString() + TEXT(", ");
						}
						OutputPort.LeftChopInline(2);
						OutputPort += TEXT("]");
					}

					FString Label = Rack->GetFName().ToString();
					FString Value = FString::Format(TEXT("{0}{1}"), { RendererType, OutputPort });
					auto RackItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::Rack, Runtime->GetAsrRackId(Rack), Label, false, Value);
					ParentItem->Children.Add(RackItem);
					TreeView->SetItemExpansion(RackItem, true);

					for (auto Child : Rack->ChildRacks)
					{
						AddRackWithChildren_Ref(Cast<UAtomRackWithParentBase>(Child), RackItem, AddRackWithChildren_Ref);
					}
				}
			};

			AddRackWithChildren_Impl(Rack, ParentItem, AddRackWithChildren_Impl);
		};

		// Additional racks
		Runtime->IterateOverRacks([&](const UAtomRackBase* Rack)
		{
			if (Rack && Rack->IsA(UAtomRackWithParentBase::StaticClass()))
			{
				// check if rack is root
				auto ParentableRack = Cast<UAtomRackWithParentBase>(Rack);
				if (!ParentableRack->ParentRack || !ParentableRack->ParentRack->IsA(UAtomRackWithParentBase::StaticClass()))
				{
					AddRackWithChildren(ParentableRack, RuntimeItem);
				}
			}
		});

		auto GetOwnerLine = [](const FAtomActiveSound* ActiveSound, uint64 ComponentID) -> FString
		{
			if (ComponentID > 0)
			{
				if (UAtomComponent* AtomComponent = UAtomComponent::GetAtomComponentFromID(ComponentID))
				{
					if (UObject* OwnerObject = AtomComponent->GetOwner())
					{
						return OwnerObject->GetName() + TEXT(".") + ActiveSound->GetAtomComponentName();
					}
				}

				return ActiveSound->GetAtomComponentName();
			}
			
			return ActiveSound->GetOwnerName();
		};

		// active sounds / Playback instances
		for (auto& ActiveSound : Runtime->GetActiveSounds())
		{
			FString Target;

			uint64 ComponentID = ActiveSound->GetAtomComponentID();
			FString Source = ActiveSound->GetSound() ? ActiveSound->GetSound()->GetFName().ToString() : TEXT("");
			FString Owner = GetOwnerLine(ActiveSound, ComponentID);
			FString WorldID = FString::FromInt(ActiveSound->GetWorldID());
			FString Label = FString::Format(TEXT("{0} ({1}) ({2})"), { Source, Owner, WorldID });
			FString Value = 
				!ActiveSound->bVirtualizedDueToMaxConcurrency
				? !Runtime->GetVirtualLoops().Contains(ActiveSound)
					? FString::Printf(TEXT("vol. %.3f (%.3fdB) [%.2fs]"), ActiveSound->GetVolume(), Atom::ConvertToDecibels(ActiveSound->GetVolume()), ActiveSound->PlaybackTime)
					: TEXT("Virtualized by Atom")
				: TEXT("Evicted by Concurrency");
			auto ActiveSoundItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::ActiveSound, ActiveSound->GetInstanceID(), Label, ActiveSound->IsPlayingAudio(), Value, ComponentID);
			TreeView->SetItemExpansion(ActiveSoundItem, true);

			if (ActiveSound) {}

			auto& PlaybackInstances = ActiveSound->GetPlaybackInstances();
			for (auto InstancePair : PlaybackInstances)
			{
				const UPTRINT Hash = InstancePair.Key;
				FAtomPlaybackInstance* Instance = InstancePair.Value;

				if (Instance)
				{
					UAtomRackBase* BusRack = nullptr;
					if (Instance->bEnableSoundBusSends && !Instance->BusSendSettings.IsEmpty())
					{
						if (auto& Bus = Instance->BusSendSettings[0].Bus)
						{
							BusRack = Bus->GetRack();
						}
					}

					if (BusRack)
					{
						Target = BusRack->GetFName().ToString();
					}
					else
					{
						if (!(Instance->SpatializationMethod == EAtomSpatializationAlgorithm::Binaural &&
							Instance->bSpatializationIsExternalSend))
						{
							Target = (Instance->bEnableSoundRack && Instance->SoundRack) ? Instance->SoundRack->GetFName().ToString() : TEXT("[") + Runtime->GetMasterRack()->GetFName().ToString() + TEXT("]");
						}
						else
						{
							Target = Runtime->GetSpatializationRack() ? TEXT("[") + Runtime->GetSpatializationRack()->GetFName().ToString() + TEXT("]") : TEXT("[") + Runtime->GetMasterRack()->GetFName().ToString() + TEXT("]");
						}
					}

					Label = FString::Printf(TEXT("%s (%llu)   -> %s)"), *Instance->SoundInfo.SoundName.ToString(), Hash, *Target);

					Value = FString::Printf(TEXT("vol. %.3f (%.3fdB) [%.2fs]"), Instance->GetVolume(), Atom::ConvertToDecibels(Instance->GetVolume()), Instance->PlaybackTime);

					auto PlaybackInstanceItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::PlaybackInstance, Instance->GetPlayOrder(), Label, Instance->IsPlaying(), Value);
					ActiveSoundItem->Children.Add(PlaybackInstanceItem);
				}
			}

			RuntimeItem->Children.Add(ActiveSoundItem);
		}

		// virtual sounds
		for (auto& VirtualSound : Runtime->GetVirtualLoops())
		{
			const FAtomActiveSound* ActiveSound = VirtualSound.Key;
			const FAtomVirtualLoop& VirtualLoop = VirtualSound.Value;

			uint64 ComponentID = ActiveSound->GetAtomComponentID();
			FString Source = ActiveSound->GetSound() ? ActiveSound->GetSound()->GetFName().ToString() : TEXT("");
			FString Owner = GetOwnerLine(ActiveSound, ComponentID);
			FString Target = FString::Format(TEXT("Rack #{0}"), { ActiveSound->GetSoundRack() ? FString::FromInt(Runtime->GetAsrRackId(ActiveSound->GetSoundRack())) : TEXT("0") });
			FString Value = FString::Format(TEXT("{0}"), { VirtualLoop.IsUsingVirtualVoice() ? TEXT("Virtual Voice") : TEXT("Virtual Loop") });
			FString Label = FString::Format(TEXT("{0} ({1} -> {2})"), { Source, Owner, Target });
			auto ActiveSoundItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::VirtualLoop, ActiveSound->GetInstanceID(), Label, ActiveSound->IsPlayingAudio(), Value, ComponentID);
			RuntimeItem->Children.Add(ActiveSoundItem);
		}

		// Concurrency groups
		Runtime->GetConcurrencyManager().IterateOverConcurrencyGroups([RuntimeItem](FAtomConcurrencyGroup* Group)
		{
			if (Group)
			{
				FString Label = !Group->GetSettings().GetDebugName().IsNone() ? Group->GetSettings().GetDebugName().ToString() : FString::FromInt(Group->GetObjectID());
				FString Count = FString::FromInt(Group->GetActiveSounds().Num());
				FString MaxCount = FString::FromInt(Group->GetSettings().MaxCount);
				FString Mode = ::ToString(Group->GetSettings().ResolutionRule);
				FString Value = FString::Format(TEXT("{0}/{1} sounds [{2}]"), { Count, MaxCount, Mode });
				auto GroupItem = MakeShared<FAtomStatisticsTreeItem>(EAtomStatisticsItemType::ConcurrencyGroup, Group->GetGroupID(), Label, !Group->IsEmpty(), Value);
				RuntimeItem->Children.Add(GroupItem);
			}
		});
	});

	TreeView->RequestTreeRefresh();
}

TSharedRef<SWidget> SAtomStatisticsDialog::ConstructOutlinerTreeView()
{
	if (!GCriWare)
	{
		return SNullWidget::NullWidget;
	}

	/*TSharedRef<FDerivedDataCacheStatsNode> RootUsage = GetDerivedDataCache()->GatherUsageStats();

	TArray<TSharedRef<const FDerivedDataCacheStatsNode>> LeafUsageStats;
	RootUsage->ForEachDescendant([&LeafUsageStats](TSharedRef<const FDerivedDataCacheStatsNode> Node) {
		if (Node->Children.Num() == 0)
		{
			LeafUsageStats.Add(Node);
		}
	});*/

	ExpandMode = EAtomStatisticsExpandMode::CollapseAll;

	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	//const FSlateColor TitleColor = FStyleColors::AccentWhite;
	//const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	auto OutlinerWidget = SAssignNew(TreeView, STreeView<TSharedPtr<FAtomStatisticsTreeItem>>)
		//.ItemHeight(20.0f)
		.TreeItemsSource(&Items)
		.SelectionMode(ESelectionMode::None)
		.OnGetChildren(this, &SAtomStatisticsDialog::OnGetChildrenForTree)
		.OnGenerateRow(this, &SAtomStatisticsDialog::OnGenerateWidgetForTree)
		.OnMouseButtonDoubleClick(this, &SAtomStatisticsDialog::OnOutlinerTreeDoubleClick)
		.HeaderRow
		(
			SNew(SHeaderRow)
			// Expand
			+ SHeaderRow::Column(ColumnID_Expander)
			.HAlignCell(HAlign_Left)
			.FixedWidth(15.0f)
			[
				SAssignNew(ToggleExpandButton, SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.ClickMethod(EButtonClickMethod::MouseDown)
					.OnClicked(this, &SAtomStatisticsDialog::OnExpandAllButtonClicked)
					.ContentPadding(0.0f)
					.ForegroundColor(FSlateColor::UseForeground())
					.IsFocusable(false)
					.ToolTipText(LOCTEXT("ExpanderCueListHeaderTip", "Expand / Collapse"))
					[
						SNew(SImage)
							.Image(this, &SAtomStatisticsDialog::GetExpanderImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
					]
			]
			// Active
			+ SHeaderRow::Column(ColumnID_Active)
			.FillWidth(0.15)
			[
				SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush("Sequencer.Tracks.Audio"))
							.ToolTipText(LOCTEXT("Active", "Item is Active."))
					]
			]
			// Label
			+ SHeaderRow::Column(ColumnID_Label)
			[
				SNew(STextBlock)
					.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
					//.ColorAndOpacity(TitleColor)
					//.Font(TitleFont)
					.Justification(ETextJustify::Left)
					.Text(LOCTEXT("AtomStatisticsItemLabel", "Item Label"))
					.ToolTipText(LOCTEXT("AtomStatisticsItemLabelTip", "Label of the Item."))
			]
			// Value
			+ SHeaderRow::Column(ColumnID_Value)
			[
				SNew(STextBlock)
					.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
					//.ColorAndOpacity(TitleColor)
					//.Font(TitleFont)
					.Justification(ETextJustify::Left)
					.Text(LOCTEXT("AtomStatisticsItemValue", "Value"))
					.ToolTipText(LOCTEXT("AtomStatisticsItemValueTip", "Value of the Item."))
			]
			// Type
			+ SHeaderRow::Column(ColumnID_Type)
			[
				SNew(STextBlock)
					.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
					//.ColorAndOpacity(TitleColor)
					//.Font(TitleFont)
					.Justification(ETextJustify::Left)
					.Text(LOCTEXT("AtomStatisticsItemType", "Type"))
					.ToolTipText(LOCTEXT("AtomStatisticsItemTypeTip", "Type of the Item."))
			]
			// Runtime ID
			+ SHeaderRow::Column(ColumnID_ID)
			//.SortMode(this, &SAtomCueSheetFactoryDialog::GetIDSortMode)
			//.OnSort(this, &SAtomCueSheetFactoryDialog::OnSortByChanged)
			.FillWidth(0.15)
			[
				SNew(STextBlock)
					.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
					//.ColorAndOpacity(TitleColor)
					//.Font(TitleFont)
					.Justification(ETextJustify::Left)
					.Text(LOCTEXT("AtomStatisticsItemID", "ID"))
					.ToolTipText(LOCTEXT("AtomStatisticsItemIDTip", "ID of the Item."))
			]
		);

	return OutlinerWidget;
}

const FSlateBrush* SAtomStatisticsDialog::GetExpanderImage() const
{
	FName ResourceName;
	if (ExpandMode == EAtomStatisticsExpandMode::ExpandAll)
	{
		if (ToggleExpandButton->IsHovered())
		{
			static FName ExpandedHoveredName = "TreeArrow_Expanded_Hovered";
			ResourceName = ExpandedHoveredName;
		}
		else
		{
			static FName ExpandedName = "TreeArrow_Expanded";
			ResourceName = ExpandedName;
		}
	}
	else
	{
		if (ToggleExpandButton->IsHovered())
		{
			static FName CollapsedHoveredName = "TreeArrow_Collapsed_Hovered";
			ResourceName = CollapsedHoveredName;
		}
		else
		{
			static FName CollapsedName = "TreeArrow_Collapsed";
			ResourceName = CollapsedName;
		}
	}

	return FCoreStyle::Get().GetBrush(ResourceName);
}

TSharedRef<ITableRow> SAtomStatisticsDialog::OnGenerateWidgetForTree(TSharedPtr<FAtomStatisticsTreeItem> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	//CueListView->SetItemSelection(InItem, InItem->Selected, ESelectInfo::OnMouseClick);

	return	SNew(SAtomStatisticsListTableRow, OwnerTable)
			.Item(InItem)
			.StatisticDialog(SharedThis(this));
}

void SAtomStatisticsDialog::OnGetChildrenForTree(TSharedPtr<FAtomStatisticsTreeItem> InItem, TArray<TSharedPtr<FAtomStatisticsTreeItem>>& OutChildren)
{
	OutChildren.Append(InItem->Children);
}

TSharedRef<SWidget> SAtomStatisticsDialog::GenerateWidgetForItemAndColumn(TSharedPtr<FAtomStatisticsTreeItem> Item, const FName& ColumnID, const TSharedRef<ITableRow>& TableRow)
{
	//check(Item.IsValid());

	const FMargin RowPadding(3, 0, 0, 0);
	const float RowMargin = 0.0f;
	const float ColumnMargin = 10.0f;

	// Any
	if (ColumnID == SAtomStatisticsDialog::ColumnID_Expander)
	{
	}

	if (ColumnID == SAtomStatisticsDialog::ColumnID_Label)
	{
		// The first column gets the tree expansion arrow for this row
		return	SNew(SBox)
				.MinDesiredHeight(FAtomStatisticsDefaultTreeItemMetrics::RowHeight())
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(6, 0, 0, 0)
					[
						SNew(SExpanderArrow, TableRow).IndentAmount(12)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Margin(FMargin(ColumnMargin, RowMargin))
						.Justification(ETextJustify::Left)
						.Text(FText::FromString(Item->Label))
					]
				];
	}

	if (ColumnID == SAtomStatisticsDialog::ColumnID_Value)
	{
		return	SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Item->Value));
	}

	if (ColumnID == SAtomStatisticsDialog::ColumnID_Type)
	{
		return	SNew(STextBlock)
				.Margin(FMargin(ColumnMargin, RowMargin))
				.Justification(ETextJustify::Left)
				.Text(FText::FromString(Item->GetTypeString()));
	}

	if (ColumnID == SAtomStatisticsDialog::ColumnID_ID)
	{
		return SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("#%u"), Item->ID)));
	}

	// Runtime
	if (Item->Type == EAtomStatisticsItemType::Runtime)
	{
		if (ColumnID == SAtomStatisticsDialog::ColumnID_Active)
		{
			if (Item->Active)
			{
				return	SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFit)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Sequencer.Tracks.Audio"))
							.ToolTipText(LOCTEXT("ActiveRuntime", "Runtime is Active."))
						];
			}
		}
	}

	return SNullWidget::NullWidget;
}

FReply SAtomStatisticsDialog::OnExpandAllButtonClicked()
{
	if (ExpandMode == EAtomStatisticsExpandMode::ExpandAll)
	{
		ExpandMode = EAtomStatisticsExpandMode::CollapseAll;
		TreeView->ClearExpandedItems();
	}
	else
	{
		ExpandMode = EAtomStatisticsExpandMode::ExpandAll;
		for (auto Item : Items)
		{
			TreeView->SetItemExpansion(Item, true);
		}
	}

	return FReply::Handled();
}

void SAtomStatisticsDialog::OnOutlinerTreeDoubleClick(TSharedPtr<FAtomStatisticsTreeItem> TreeItem)
{
	if (TreeItem.IsValid())
	{
		if (TreeItem->Type == EAtomStatisticsItemType::ActiveSound ||
			TreeItem->Type == EAtomStatisticsItemType::VirtualLoop)
		{
			if (UAtomComponent* Component = UAtomComponent::GetAtomComponentFromID(TreeItem->ComponentID))
			{
				const bool bActiveViewportOnly = false;
				GEditor->MoveViewportCamerasToComponent(Component, bActiveViewportOnly);
			}
		}
	}
}

void SAtomStatisticsListTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	StatisticDialogWeak = InArgs._StatisticDialog;
	Item = InArgs._Item;

	SMultiColumnTableRow< TSharedPtr<FAtomStatisticsTreeItem> >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SAtomStatisticsListTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	// Create the widget for this item
	auto StatisticDialogShared = StatisticDialogWeak.Pin();
	if (StatisticDialogShared.IsValid())
	{
		return StatisticDialogShared->GenerateWidgetForItemAndColumn(Item, ColumnName, SharedThis(this));
	}

	// Packages dialog no longer valid; return a valid, null widget.
	return SNullWidget::NullWidget;
}

#undef LOCTEXT_NAMESPACE
