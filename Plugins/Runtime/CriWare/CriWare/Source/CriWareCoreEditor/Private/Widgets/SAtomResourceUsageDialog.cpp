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

#include "SAtomResourceUsageDialog.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomWaveBankResource.h"
#include "Atom/AtomCueSheetResource.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomSoundBank.h"

#define LOCTEXT_NAMESPACE "CriWareCoreEditor"

void SAtomResourceUsageDialog::Construct(const FArguments& InArgs)
{
	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColour = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	// todo : use weak/shared ptr to access managers since module can be restarted by editor
	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
	FAtomResourceManager* ResourceManager = GCriWare->GetAtomResourceManager();

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
				.Text( FText::FromString(TEXT("MainAtom")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 0)
		.Expose(GridSlot)
		[
			GetGridPanel()
		]

	];

	// Refresh every 1/2 seconds
	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomResourceUsageDialog::UpdateGridPanels));
}

EActiveTimerReturnType SAtomResourceUsageDialog::UpdateGridPanels(double InCurrentTime, float InDeltaTime)
{
	(*GridSlot)
	[
		GetGridPanel()
	];

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SAtomResourceUsageDialog::GetGridPanel()
{
	if (!GCriWare)
	{
		return SNullWidget::NullWidget;
	}

	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
	FAtomResourceManager* ResourceManager = GCriWare->GetAtomResourceManager();

	TSharedRef<SGridPanel> Panel = SNew(SGridPanel);

	int32 Row = 0;

	// setup
	
	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColor = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	// headers
	
	Panel->AddSlot(0, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		.Text(LOCTEXT("Resource", "Resource"))
		.ColorAndOpacity(TitleColor)
		.Text(LOCTEXT("ResourceID", "ID"))
	];

	Panel->AddSlot(1, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("ResourceName", "Name"))
	];

	Panel->AddSlot(2, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("ResourceType", "Type"))
	];

	Panel->AddSlot(3, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("ResourceNumElements", "Num Waves/Cues"))
	];

	Panel->AddSlot(4, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Resource Handles", "Num Handles"))
	];

	Panel->AddSlot(5, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Resource Scope", "Scope"))
	];

	Panel->AddSlot(6, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Resource Status", "Status"))
	];

	Panel->AddSlot(7, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Resource Memory", "Memory"))
	];

	Panel->AddSlot(8, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Resource Behaviour", "Behaviour"))
	];

	// rows

	ResourceManager->IterateOverAllResourcesForStatistics([&](FAtomResourceId ResourceID, FAtomResource* Resource, int32 NumberOfHandles, EAtomResourceScope Scope)
	{
		FAtomCueSheetResource* CueSheetResource = Resource->GetCueSheetResource();
		FAtomWaveBankResource* WaveBankResource = Resource->GetWaveBankResource();

		auto SoundResource = Cast<UAtomSoundBank>(Resource->GetSoundResource());
		auto CueSheet = Cast<UAtomCueSheet>(SoundResource);
		auto WaveBank = Cast<UAtomWaveBank>(SoundResource);

		Row++;

		Panel->AddSlot(0, Row) // ID
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("#%u"), ResourceID)))
		];

		Panel->AddSlot(1, Row) // Name
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(SoundResource ? SoundResource->GetName() : "Unnamed"))
		];

		Panel->AddSlot(2, Row) // Type
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Center)
			.Text(FText::FromString(CueSheetResource ? TEXT("CueSheet") : WaveBankResource ? TEXT("WaveBank") : TEXT("Undefined")))
		];

		Panel->AddSlot(3, Row) // Num Elements
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("%u"), CueSheet ? CueSheet->GetNumCues() : WaveBank ? WaveBank->GetNumWaves() : 0)))
		];

		Panel->AddSlot(4, Row) // Num handles (this loop use an handle that is not in counter)
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(FString::Printf(TEXT("%u"), NumberOfHandles)))
		];

		Panel->AddSlot(5, Row) // Scope
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Scope == EAtomResourceScope::Unique ? TEXT("Unique") : Scope == EAtomResourceScope::Shared ? TEXT("Shared") : TEXT("Undefined")))
		];

		Panel->AddSlot(6, Row) // Status
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(Resource->IsLoaded() ? TEXT("Loaded") : TEXT("Unloaded")))
		];

		Panel->AddSlot(7, Row) // Raw Data Memory Location
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(CueSheetResource ? (CueSheetResource->IsUsingFileSystemBinder() ? TEXT("From Archive") : TEXT("Inlined")) :
									WaveBankResource ? (WaveBankResource->IsUsingFileSystemBinder() ? TEXT("From Archive") : TEXT("Inlined")) :
									TEXT("Undefined")))
		];

		Panel->AddSlot(8, Row) // Current Loading Behaviour
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Margin(FMargin(ColumnMargin, RowMargin))
			.Justification(ETextJustify::Left)
			.Text(FText::FromString(SoundResource ? ::ToString(SoundResource->LoadingBehavior) : TEXT("Error")))
		];
	});

	return Panel;
}

#undef LOCTEXT_NAMESPACE
