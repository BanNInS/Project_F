/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomMonitorDialog.h
 *
 ****************************************************************************/

#include "SAtomMonitorDialog.h"

#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Profiling/AtomMonitor.h"

#include "Widgets/SAtomMonitorLogWidget.h"

#define LOCTEXT_NAMESPACE "AtomMonitor"

void SAtomMonitorDialog::Construct(const FArguments& InArgs)
{
	const float RowMargin = 0.0f;
	const float TitleMargin = 10.0f;
	const float ColumnMargin = 10.0f;
	const FSlateColor TitleColour = FStyleColors::AccentWhite;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);

	// todo : use weak/shared ptr to access managers since module can be restarted by editor
	//FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();
	//if (RuntimeManager)
	//{
	//	FAtomMonitor& Monitor = RuntimeManager->GetAtomMonitor();
	//}

	OnAtomMonitorStartedHandle = FAtomMonitorDelegates::OnAtomMonitorStarted.AddRaw(this, &SAtomMonitorDialog::OnMonitorStateChanged, true);
	OnAtomMonitorStoppedHandle = FAtomMonitorDelegates::OnAtomMonitorStopped.AddRaw(this, &SAtomMonitorDialog::OnMonitorStateChanged, false);
	OnAtomMonitorLogHandle = FAtomMonitorDelegates::OnAtomMonitorLog.AddRaw(this, &SAtomMonitorDialog::OnMonitorLog);
	OnAtomMonitorConnectedHandle = FAtomMonitorDelegates::OnAtomMonitorConnected.AddRaw(this, &SAtomMonitorDialog::OnMonitorConnected, true);
	OnAtomMonitorDisconnectedHandle = FAtomMonitorDelegates::OnAtomMonitorDisconnected.AddRaw(this, &SAtomMonitorDialog::OnMonitorConnected, false);
	

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(0, 5, 0, 0)
		.FillHeight(1.0f)
		[
			SAssignNew(MonitorLog, SAtomMonitorLogWidget)
			.ExpectedFilters(EAtomMonitorLogMode::All)
		]
	];

	if (MonitorLog.IsValid())
	{
		MonitorLog->OnStartStopClicked.BindLambda([]()
		{
			if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
			{
				FAtomMonitor& Monitor = RuntimeManager->GetAtomMonitor();
				if (Monitor.IsStarted())
				{
					Monitor.Stop();
				}
				else
				{
					Monitor.Start();
				}
			}
		});

		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			FAtomMonitor& Monitor = RuntimeManager->GetAtomMonitor();

			MonitorLog->OnMonitorStateChanged(Monitor.GetConnectionInfo().HostAddr.ToString(), Monitor.GetConnectionInfo().HostTCPPort, Monitor.IsStarted() ? EAtomMonitorLogState::Started : EAtomMonitorLogState::Stopped);
		}
	}

	// Refresh every 1/2 seconds
	//RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomMonitorDialog::UpdateGridPanels));
}

SAtomMonitorDialog::~SAtomMonitorDialog()
{
	FAtomMonitorDelegates::OnAtomMonitorStarted.Remove(OnAtomMonitorStartedHandle);
	FAtomMonitorDelegates::OnAtomMonitorStopped.Remove(OnAtomMonitorStoppedHandle);
	FAtomMonitorDelegates::OnAtomMonitorLog.Remove(OnAtomMonitorLogHandle);
	FAtomMonitorDelegates::OnAtomMonitorConnected.Remove(OnAtomMonitorConnectedHandle);
	FAtomMonitorDelegates::OnAtomMonitorDisconnected.Remove(OnAtomMonitorDisconnectedHandle);
}

void SAtomMonitorDialog::OnMonitorStateChanged(const FString& HostIp, uint16 HostPort, bool bIsStarted)
{
	if (MonitorLog.IsValid())
	{
		MonitorLog->OnMonitorStateChanged(HostIp, HostPort, bIsStarted ? EAtomMonitorLogState::Started : EAtomMonitorLogState::Stopped);
	}
}

void SAtomMonitorDialog::OnMonitorLog(const FString& LogMsg, EAtomMonitorLogMode LogMode)
{
	if (MonitorLog.IsValid())
	{
		FString OutMsg;

		FString LogTypeS;
		FString TimestampS;
		FString PidS;
		if (LogMsg.Split(TEXT(","), &LogTypeS, &OutMsg))
		{
			if (OutMsg.Split(TEXT(","), &TimestampS, &OutMsg))
			{
				if (OutMsg.Split(TEXT(","), &PidS, &OutMsg))
				{

				}
			}
		}

		LogTypeS.RemoveFromStart(TEXT("#"));
		PidS.TrimStartInline();
		OutMsg.TrimStartInline();
		OutMsg = FString::Format(TEXT("[{0}] {1}: {2}"), { PidS, LogTypeS, OutMsg });

		FTimespan TimeStamp = FTimespan::FromMicroseconds(FCString::Atoi(*TimestampS));
		
		FSlateColor LogColor = FSlateColor::UseForeground();
		switch (LogMode)
		{
		case EAtomMonitorLogMode::Playback:			LogColor = FColor::White; break;
		case EAtomMonitorLogMode::Error:			LogColor = FColor::Red; break;
		case EAtomMonitorLogMode::LowLevelPlayback:	LogColor = FColor::Cyan; break;
		case EAtomMonitorLogMode::SystemInfo:		LogColor = FColor::Yellow; break;
		case EAtomMonitorLogMode::HandleInfo:		LogColor = FColor::Purple; break;
		case EAtomMonitorLogMode::CueLimit:			LogColor = FColor::Orange; break;
		case EAtomMonitorLogMode::Probability:		LogColor = FColor::Magenta; break;
		case EAtomMonitorLogMode::Category:			LogColor = FColor::Emerald; break;
		case EAtomMonitorLogMode::ExecutingInfo:	LogColor = FColor::Blue; break;
		case EAtomMonitorLogMode::Position3DInfo:	LogColor = FColor::Green; break;
		case EAtomMonitorLogMode::User:				LogColor = FColor::Turquoise; break;
		case EAtomMonitorLogMode::VoiceVolume:		LogColor = FColor::Silver; break;
		case EAtomMonitorLogMode::Off:
		case EAtomMonitorLogMode::All:
		default:
			LogColor = FColor(127, 127, 127);
		break;
		}

		bool bRequestFocus = false;

		MonitorLog->AddLine(LogMode, MakeShareable(new FString(OutMsg)), TimeStamp, LogColor, bRequestFocus);
	}
}

void SAtomMonitorDialog::OnMonitorConnected(const FString& ClientIP, bool bIsConnected)
{
	if (MonitorLog.IsValid())
	{
		MonitorLog->OnMonitorRemoteStateChanged(ClientIP, bIsConnected);
	}
}

EActiveTimerReturnType SAtomMonitorDialog::UpdateGridPanels(double InCurrentTime, float InDeltaTime)
{
	(*GridSlot)
	[
		GetGridPanel()
	];

	SlatePrepass(GetPrepassLayoutScaleMultiplier());

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SAtomMonitorDialog::GetGridPanel()
{
	if (!GCriWare)
	{
		return SNullWidget::NullWidget;
	}

	FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager();

	TSharedRef<SGridPanel> Panel = SNew(SGridPanel);

	/*int32 Row = 0;

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
		.Text(LOCTEXT("Type", "Type"))
		.ColorAndOpacity(TitleColor)
	];

	Panel->AddSlot(1, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Timestamp", "Timestamp"))
	];

	Panel->AddSlot(2, Row)
	[
		SNew(STextBlock)
		.Margin(FMargin(ColumnMargin, RowMargin, 0.0f, TitleMargin))
		.ColorAndOpacity(TitleColor)
		.Font(TitleFont)
		.Justification(ETextJustify::Left)
		.Text(LOCTEXT("Event", "Event"))
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
			.Text(FText::FromString(SoundResource ? ::ToString(SoundResource->GetLoadingBehavior()) : TEXT("Error")))
		];
	});
	*/
	return Panel;
}

#undef LOCTEXT_NAMESPACE
