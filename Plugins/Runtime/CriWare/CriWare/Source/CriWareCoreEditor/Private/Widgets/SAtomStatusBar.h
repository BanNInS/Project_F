/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomStatusBar.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Animation/CurveSequence.h"
#include "Framework/Commands/Commands.h"

class FUICommandList;

class FAtomStatusBarMenuCommands : public TCommands<FAtomStatusBarMenuCommands>
{
public:

	FAtomStatusBarMenuCommands();

	virtual void RegisterCommands() override;

private:

	static void ChangeSettings_Clicked();
	static void ChangeEditorSettings_Clicked();
	static void ChangePlatformSettings_Clicked(const FName SectionName);
	static void ViewAtomStatistics_Clicked();
	static void ViewAtomResourceUsage_Clicked();
	static void OpenAtomMonitor_Clicked();
	static void StartStopAtomMonitor_Clicked();

public:

	TSharedPtr< FUICommandInfo > ChangeSettings;
	TSharedPtr< FUICommandInfo > ChangeEditorSettings;
	TSharedPtr< FUICommandInfo > ViewAtomStatistics;
	TSharedPtr< FUICommandInfo > ViewAtomResourceUsage;
	TSharedPtr< FUICommandInfo > OpenAtomMonitor;
	TSharedPtr< FUICommandInfo > StartAtomMonitor;
	TSharedPtr< FUICommandInfo > StopAtomMonitor;
	TArray<TSharedPtr< FUICommandInfo >> ChangePlatformSettingsList;

	static TSharedRef<FUICommandList> ActionList;
};

class SAtomStatusBarWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAtomStatusBarWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	FText						GetTitleToolTipText() const;
	FText						GetRemoteCacheToolTipText() const;
	FText						GetTitleText() const;
	FText						GetBinauralToolTipText() const;
	const FSlateBrush*			GetRemoteCacheStateBackgroundIcon() const;
	const FSlateBrush*			GetRemoteCacheStateBadgeIcon() const;
	const FSlateBrush*			GetBinauralStateBackgroundIcon() const;
	const FSlateBrush*			GetBinauralStateBadgeIcon() const;
	TSharedRef<SWidget>			CreateStatusBarMenu();
	EActiveTimerReturnType		UpdateBusyIndicator(double InCurrentTime, float InDeltaTime);
	EActiveTimerReturnType		UpdateWarnings(double InCurrentTime, float InDeltaTime);

	double ElapsedDownloadTime = 0;
	double ElapsedUploadTime = 0;
	double ElapsedBusyTime = 0;

	bool bBusy = false;

	TSharedPtr<SNotificationItem> NotificationItem;
};
