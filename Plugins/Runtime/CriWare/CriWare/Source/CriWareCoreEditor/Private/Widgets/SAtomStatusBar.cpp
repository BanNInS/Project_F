/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomStatusBar.cpp
 *
 ****************************************************************************/

#include "SAtomStatusBar.h"

#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"
#include "EditorStyleSet.h"
#include "EditorFontGlyphs.h"
#include "ISettingsModule.h"
#include "ToolMenuContext.h"
#include "ToolMenus.h"
#include "Settings/EditorSettings.h"
#include "Settings/EditorProjectSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Modules/ModuleManager.h"

#include "CriWare.h"
#include "Atom/Profiling/AtomMonitor.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"

#include "CriWareCoreEditorModule.h"
#include "AtomEditorStyle.h"

#define LOCTEXT_NAMESPACE "AtomEditor"

TSharedRef<FUICommandList> FAtomStatusBarMenuCommands::ActionList(new FUICommandList());

FAtomStatusBarMenuCommands::FAtomStatusBarMenuCommands()
	: TCommands<FAtomStatusBarMenuCommands>
	(
		"CriWareAtomSettings",
		NSLOCTEXT("Contexts", "CriWareAtom", "CriWareAtom"),
		"LevelEditor",
		FAppStyle::GetAppStyleSetName()
	)
{}

void FAtomStatusBarMenuCommands::RegisterCommands()
{
	UI_COMMAND(ChangeSettings, "Change Project Settings", "Opens a dialog to change Atom Project settings.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ChangeEditorSettings, "Change Editor Settings", "Opens a dialog to change Atom Editor settings.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ViewAtomStatistics, "View Runtime Statistics", "Opens the Runtime Statistics panel.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ViewAtomResourceUsage, "View Resource Usage", "Opens the Resource Usage panel.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenAtomMonitor, "Open Atom Monitor", "Opens the Atom Monitor panel. Monitor also enables remote ADX In-Game previewer and profilers.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StartAtomMonitor, "Start In-Game Preview", "Enables remote ADX In-Game previewer and profilers.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StopAtomMonitor, "Stop In-Game Preview", "Disables remote ADX In-Game previewer and profilers.", EUserInterfaceActionType::Button, FInputChord());


	ActionList->MapAction(
		ChangeSettings,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangeSettings_Clicked)
	);

	ActionList->MapAction(
		ChangeEditorSettings,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangeEditorSettings_Clicked)
	);

	ActionList->MapAction(
		ViewAtomStatistics,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ViewAtomStatistics_Clicked)
	);

	ActionList->MapAction(
		ViewAtomResourceUsage,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ViewAtomResourceUsage_Clicked)
	);

	ActionList->MapAction(
		OpenAtomMonitor,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::OpenAtomMonitor_Clicked)
	);

	ActionList->MapAction(
		StartAtomMonitor,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::StartStopAtomMonitor_Clicked)
	);

	ActionList->MapAction(
		StopAtomMonitor,
		FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::StartStopAtomMonitor_Clicked)
	);

	// platforms - todo declare it in platform module

	// Android
	if (FModuleManager::Get().IsModuleLoaded("CriWareAndroidSettings"))
	{
		const FName SectionName = TEXT("Android - ADX Atom");

		TSharedPtr< FUICommandInfo > ChangeAndroidSettings;
		UI_COMMAND(ChangeAndroidSettings, "Change Android Settings", "Opens a dialog to change ADX Atom Android settings.", EUserInterfaceActionType::Button, FInputChord());

		ChangePlatformSettingsList.Add(ChangeAndroidSettings);

		ActionList->MapAction(
			ChangeAndroidSettings,
			FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked, SectionName)
		);
	}

	// iOS
	if (FModuleManager::Get().IsModuleLoaded("CriWareAppleSettings"))
	{
		const FName SectionName = TEXT("iOS - ADX Atom");

		TSharedPtr< FUICommandInfo > ChangeIOSSettings;
		UI_COMMAND(ChangeIOSSettings, "Change iOS Settings", "Opens a dialog to change ADX Atom iOS settings.", EUserInterfaceActionType::Button, FInputChord());

		ChangePlatformSettingsList.Add(ChangeIOSSettings);

		ActionList->MapAction(
			ChangeIOSSettings,
			FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked, SectionName)
		);
	}

	// Linux
	if (FModuleManager::Get().IsModuleLoaded("CriWareLinuxSettings"))
	{
		const FName SectionName = TEXT("Linux - ADX Atom");

		TSharedPtr< FUICommandInfo > ChangeLinuxSettings;
		UI_COMMAND(ChangeLinuxSettings, "Change Linux Settings", "Opens a dialog to change ADX Atom Linux settings.", EUserInterfaceActionType::Button, FInputChord());

		ChangePlatformSettingsList.Add(ChangeLinuxSettings);

		ActionList->MapAction(
			ChangeLinuxSettings,
			FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked, SectionName)
		);
	}

	// macOS
	if (FModuleManager::Get().IsModuleLoaded("CriWareAppleSettings"))
	{
		const FName SectionName = TEXT("macOS - ADX Atom");

		TSharedPtr< FUICommandInfo > ChangeMacSettings;
		UI_COMMAND(ChangeMacSettings, "Change macOS Settings", "Opens a dialog to change ADX Atom macOS settings.", EUserInterfaceActionType::Button, FInputChord());

		ChangePlatformSettingsList.Add(ChangeMacSettings);

		ActionList->MapAction(
			ChangeMacSettings,
			FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked, SectionName)
		);
	}

	// Windows
	if (FModuleManager::Get().IsModuleLoaded("CriWareWindowsSettings"))
	{
		const FName SectionName = TEXT("Windows - ADX Atom");

		TSharedPtr< FUICommandInfo > ChangeWindowsSettings;
		UI_COMMAND(ChangeWindowsSettings, "Change Windows Settings", "Opens a dialog to change ADX Atom Windows settings.", EUserInterfaceActionType::Button, FInputChord());

		ChangePlatformSettingsList.Add(ChangeWindowsSettings);

		ActionList->MapAction(
			ChangeWindowsSettings,
			FExecuteAction::CreateStatic(&FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked, SectionName)
		);
	}
}

void FAtomStatusBarMenuCommands::ChangeSettings_Clicked()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "CriWare", "ADX Atom");
}

void FAtomStatusBarMenuCommands::ChangeEditorSettings_Clicked()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "CriWare", "ADX Atom");
}

void FAtomStatusBarMenuCommands::ChangePlatformSettings_Clicked(const FName SectionName)
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "CriWare", SectionName);
}

void FAtomStatusBarMenuCommands::ViewAtomStatistics_Clicked()
{
	ICriWareCoreEditorModule& CriWareCoreEditorModule = FModuleManager::LoadModuleChecked<ICriWareCoreEditorModule>("CriWareCoreEditor");
	CriWareCoreEditorModule.ShowAtomStatisticsTab();
}

void FAtomStatusBarMenuCommands::ViewAtomResourceUsage_Clicked()
{
	ICriWareCoreEditorModule& CriWareCoreEditorModule = FModuleManager::LoadModuleChecked<ICriWareCoreEditorModule>("CriWareCoreEditor");
	CriWareCoreEditorModule.ShowAtomResourceUsageTab();
}

void FAtomStatusBarMenuCommands::OpenAtomMonitor_Clicked()
{
	ICriWareCoreEditorModule& CriWareCoreEditorModule = FModuleManager::LoadModuleChecked<ICriWareCoreEditorModule>("CriWareCoreEditor");
	CriWareCoreEditorModule.ShowAtomMonitorTab();
}

void FAtomStatusBarMenuCommands::StartStopAtomMonitor_Clicked()
{
	FAtomMonitor& Monitor = FAtomMonitor::Get();
	if (Monitor.IsStarted())
	{
		Monitor.Stop();
	}
	else
	{
		Monitor.Start();
	}
}

TSharedRef<SWidget> SAtomStatusBarWidget::CreateStatusBarMenu()
{
	UToolMenu* Menu = UToolMenus::Get()->RegisterMenu("StatusBar.ToolBar.CriWareAtom", NAME_None, EMultiBoxType::Menu, false);

	{
		FToolMenuSection& Section = Menu->AddSection("AtomMenuSettingsSection", LOCTEXT("AtomMenuSettingsSection", "Settings"));

		Section.AddMenuEntry(
			FAtomStatusBarMenuCommands::Get().ChangeEditorSettings,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.Cache.Settings")
		);

		Section.AddMenuEntry(
			FAtomStatusBarMenuCommands::Get().ChangeSettings,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.Cache.Settings")
		);

		Section.AddSubMenu(
			"ChangePlatformSettings",
			LOCTEXT("ChangePlatformSettings", "Change Platform Settings"),
			TAttribute<FText>(), 
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* InSubMenu)
			{
				FToolMenuSection& SubSection = InSubMenu->AddSection("AtomMenuPlatformSettingsSection", LOCTEXT("AtomMenuPlatformSettingsSection", "Platform Settings"));

				for (auto ChangePlatformSettings : FAtomStatusBarMenuCommands::Get().ChangePlatformSettingsList)
				{
					SubSection.AddMenuEntry(
						ChangePlatformSettings,
						TAttribute<FText>(),
						TAttribute<FText>(),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.Cache.Settings")
					);
				}
			}),
			false, // default value
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.Cache.Settings")
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection("AtomMenuStatisticsSection", LOCTEXT("AtomMenuStatisticsSection", "Statistics"));

		Section.AddMenuEntry(
			FAtomStatusBarMenuCommands::Get().ViewAtomStatistics,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.Cache.Statistics")
		);

		Section.AddMenuEntry(
			FAtomStatusBarMenuCommands::Get().ViewAtomResourceUsage,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "DerivedData.ResourceUsage")
		);
	}

	{
		FToolMenuSection& Section = Menu->AddSection("AtomMenuToolsSection", LOCTEXT("AtomMenuToolsSection", "Tools"));

		Section.AddMenuEntry(
			FAtomStatusBarMenuCommands::Get().OpenAtomMonitor,
			TAttribute<FText>(),
			TAttribute<FText>(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Log.TabIcon")
		);
	
		Section.AddDynamicEntry("StartStopAtomMonitor", 
			FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				if (FAtomMonitor::Get().IsStarted())
				{
					InSection.AddMenuEntry(
						FAtomStatusBarMenuCommands::Get().StopAtomMonitor,
						TAttribute<FText>(),
						TAttribute<FText>(),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Log.TabIcon")
					);
				}
				else
				{
					InSection.AddMenuEntry(
						FAtomStatusBarMenuCommands::Get().StartAtomMonitor,
						TAttribute<FText>(),
						TAttribute<FText>(),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "Log.TabIcon")
					);
				}
			})
		);
	}

	return UToolMenus::Get()->GenerateWidget("StatusBar.ToolBar.CriWareAtom", FToolMenuContext(FAtomStatusBarMenuCommands::ActionList));
}

void SAtomStatusBarWidget::Construct(const FArguments& InArgs)
{
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboButton)
			.ContentPadding(FMargin(6.0f, 0.0f))
			.MenuPlacement(MenuPlacement_AboveAnchor)
			.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("StatusBar.StatusBarComboButton"))
			.ButtonContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 3, 0)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image_Lambda([this] { return GetRemoteCacheStateBackgroundIcon();  })
						.ToolTipText_Lambda([this] { return GetRemoteCacheToolTipText(); })
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image_Lambda([this] { return GetRemoteCacheStateBadgeIcon();  })
						.ToolTipText_Lambda([this] { return GetRemoteCacheToolTipText(); })
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Uploading"))
						//.ColorAndOpacity_Lambda([this] { return (FDerivedDataInformation::IsUploading() && FDerivedDataInformation::GetRemoteCacheState() == ERemoteCacheState::Busy) ? FLinearColor::White.CopyWithNewOpacity(FMath::MakePulsatingValue(ElapsedUploadTime, 2)) : FLinearColor(0, 0, 0, 0); })
						.ToolTipText_Lambda([this] { return GetRemoteCacheToolTipText(); })
					]

					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Top)
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Downloading"))
						//.ColorAndOpacity_Lambda([this] { return (FDerivedDataInformation::IsDownloading() && FDerivedDataInformation::GetRemoteCacheState() == ERemoteCacheState::Busy) ? FLinearColor::White.CopyWithNewOpacity(FMath::MakePulsatingValue(ElapsedDownloadTime, 2)) : FLinearColor(0, 0, 0, 0); })
						.ToolTipText_Lambda([this] { return GetRemoteCacheToolTipText(); })
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 10, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([this] { return GetTitleText(); })
					.ToolTipText_Lambda([this] { return GetTitleToolTipText(); })
				]
			]
			.OnGetMenuContent(FOnGetContent::CreateRaw(this, &SAtomStatusBarWidget::CreateStatusBarMenu))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.Content()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image_Lambda([this] { return GetBinauralStateBackgroundIcon();  })
					.ToolTipText_Lambda([this] { return GetBinauralToolTipText(); })
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image_Lambda([this] { return GetBinauralStateBadgeIcon();  })
					.ToolTipText_Lambda([this] { return GetBinauralToolTipText(); })
				]
			]
			.ToolTipText_Lambda([this] { return GetBinauralToolTipText(); })
			.OnClicked_Lambda([this]
			{
				if (GCriWare)
				{
					if (auto Runtime = GCriWare->GetActiveAtomRuntime())
					{
						Runtime->SetHRTFDisabled(!Runtime->IsHRTFDisabled());
					}
				}
				return FReply::Handled();
			})
		]
	];

	RegisterActiveTimer(0.2f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomStatusBarWidget::UpdateBusyIndicator));
	RegisterActiveTimer(5.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomStatusBarWidget::UpdateWarnings));
}

EActiveTimerReturnType SAtomStatusBarWidget::UpdateBusyIndicator(double InCurrentTime, float InDeltaTime)
{
	/*FDerivedDataInformation::UpdateRemoteCacheState();

	bBusy = GetDerivedDataCache()->AnyAsyncRequestsRemaining();

	if (FDerivedDataInformation::IsUploading())
	{
		ElapsedUploadTime += fmod(InDeltaTime, 3600.0);
	}
	else
	{
		ElapsedUploadTime = 0.0;
	}

	if (FDerivedDataInformation::IsDownloading())
	{
		ElapsedDownloadTime += fmod(InDeltaTime, 3600.0);
	}
	else
	{
		ElapsedDownloadTime = 0.0;
	}

	if (bBusy)
	{
		ElapsedBusyTime += fmod(InDeltaTime, 3600.0);
	}
	else
	{
		ElapsedBusyTime = 0;
	}*/

	return EActiveTimerReturnType::Continue;
}

EActiveTimerReturnType SAtomStatusBarWidget::UpdateWarnings(double InCurrentTime, float InDeltaTime)
{
	/*if (FDerivedDataInformation::GetRemoteCacheState() == ERemoteCacheState::Warning)
	{
		if (NotificationItem.IsValid() == false || NotificationItem->GetCompletionState() == SNotificationItem::CS_None)
		{
			// No existing notification or the existing one has finished
			TPromise<TWeakPtr<SNotificationItem>> NotificationPromise;

			FNotificationInfo Info(FDerivedDataInformation::GetRemoteCacheWarningMessage());
			Info.bUseSuccessFailIcons = true;
			Info.bFireAndForget = false;
			Info.bUseThrobber = false;
			Info.FadeOutDuration = 0.0f;
			Info.ExpireDuration = 0.0f;

			Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("UpdateSettings", "Update Settings"), FText(), FSimpleDelegate::CreateLambda([NotificationFuture = NotificationPromise.GetFuture().Share()]() {
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Editor", "General", "Global");

				TWeakPtr<SNotificationItem> NotificationPtr = NotificationFuture.Get();
				if (TSharedPtr<SNotificationItem> Notification = NotificationPtr.Pin())
				{
					Notification->SetCompletionState(SNotificationItem::CS_None);
					Notification->ExpireAndFadeout();
				}
			}),
				SNotificationItem::ECompletionState::CS_Fail));

			NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);

			if (NotificationItem.IsValid())
			{
				NotificationPromise.SetValue(NotificationItem);
				NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
			}
		}
	}
	else
	{
		// No longer any warnings
		if (NotificationItem.IsValid())
		{
			NotificationItem->SetCompletionState(SNotificationItem::CS_None);
			NotificationItem->ExpireAndFadeout();
		}
	}*/

	return EActiveTimerReturnType::Continue;
}

FText SAtomStatusBarWidget::GetTitleToolTipText() const
{
	const FString AtomVersion = FAtomRuntime::GetAtomLibraryVersionString();

	FTextBuilder DescBuilder;
	DescBuilder.AppendLine(LOCTEXT("AtomMenuToolTip", "ADX Sound Engine:"));
	DescBuilder.AppendLineFormat(LOCTEXT("AtomMenuToolTip", "  {0}"), FText::FromString(AtomVersion));
	return DescBuilder.ToText();
}

FText SAtomStatusBarWidget::GetTitleText() const
{
	return LOCTEXT("AtomToolBarName", "Atom");
}

FText SAtomStatusBarWidget::GetRemoteCacheToolTipText() const
{
	FTextBuilder DescBuilder;

	/*if (FDerivedDataInformation::GetRemoteCacheState() == ERemoteCacheState::Warning)
	{
		DescBuilder.AppendLineFormat(LOCTEXT("RemoteCacheErrorText", "WARNING\t: {0}\n"), FDerivedDataInformation::GetRemoteCacheWarningMessage());
	}

	DescBuilder.AppendLine(LOCTEXT("RemoteCacheToolTipText", "Remote Cache\n"));
	DescBuilder.AppendLineFormat(LOCTEXT("RemoteCacheConnectedText", "Connected\t: {0}"), FText::FromString((FDerivedDataInformation::GetHasRemoteCache() ? TEXT("Yes") : TEXT("No"))));
	DescBuilder.AppendLineFormat(LOCTEXT("RemoteCacheStatusText", "Status\t: {0}"), FDerivedDataInformation::GetRemoteCacheStateAsText());

	const double DownloadedBytesMB = FUnitConversion::Convert(FDerivedDataInformation::GetCacheActivitySizeBytes(true, false), EUnit::Bytes, EUnit::Megabytes);
	const double UploadedBytesMB = FUnitConversion::Convert(FDerivedDataInformation::GetCacheActivitySizeBytes(false, false), EUnit::Bytes, EUnit::Megabytes);

	DescBuilder.AppendLineFormat(LOCTEXT("RemoteCacheDownloaded", "Downloaded\t: {0} MB"), DownloadedBytesMB);
	DescBuilder.AppendLineFormat(LOCTEXT("RemoteCacheUploaded", "Uploaded\t: {0} MB"), UploadedBytesMB);
	*/
	return DescBuilder.ToText();
}

const FSlateBrush* SAtomStatusBarWidget::GetRemoteCacheStateBackgroundIcon() const
{
	/*switch (FDerivedDataInformation::GetRemoteCacheState())
	{
	case ERemoteCacheState::Idle:
	{*/
		return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.IdleBG");
	/*	break;
	}

	case ERemoteCacheState::Busy:
	{
		return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.BusyBG");
		break;
	}

	case ERemoteCacheState::Unavailable:
	{
		return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.UnavailableBG");
		break;
	}

	case ERemoteCacheState::Warning:
	default:
	{
		return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.WarningBG");
		break;
	}
	}*/
}

const FSlateBrush* SAtomStatusBarWidget::GetRemoteCacheStateBadgeIcon() const
{
	/*switch (FDerivedDataInformation::GetRemoteCacheState())
	{
		case ERemoteCacheState::Idle:
		{*/
			return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Idle");
	/*		break;
		}

		case ERemoteCacheState::Busy:
		{
			return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Busy");
			break;
		}

		case ERemoteCacheState::Unavailable:
		{
			return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Unavailable");
			break;
		}

		case ERemoteCacheState::Warning:
		default:
		{
			return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Warning");
			break;
		}
	}*/
}

FText SAtomStatusBarWidget::GetBinauralToolTipText() const
{
	if (GCriWare)
	{
		if (auto Runtime = GCriWare->GetActiveAtomRuntime())
		{
			if (Runtime && Runtime->IsHRTFDisabled())
			{
				return LOCTEXT("AtomBinauralDisabledToolTip", "Binaural spatializer is disabled.");
			}
		}
	}

	return LOCTEXT("AtomBinauralEnableToolTip", "Binaural spatializer is enabled for all Soundfields.");
}

const FSlateBrush* SAtomStatusBarWidget::GetBinauralStateBackgroundIcon() const
{
	//return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.IdleBG");
	return FAtomEditorStyle::Get().GetBrush("CriWare.HeadphoneIcon");
}

const FSlateBrush* SAtomStatusBarWidget::GetBinauralStateBadgeIcon() const
{
	if (GCriWare)
	{
		if (auto Runtime = GCriWare->GetActiveAtomRuntime())
		{
			if (Runtime && Runtime->IsHRTFDisabled())
			{
				return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Busy");
			}
		}
	}
	
	return FAppStyle::Get().GetBrush("DerivedData.RemoteCache.Idle");
}

#undef LOCTEXT_NAMESPACE
