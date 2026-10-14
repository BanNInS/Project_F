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

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"

// Forward Definitions
class SAtomMonitorLogWidget;
enum class EAtomMonitorLogMode : uint16;

class SAtomMonitorDialog
	: public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAtomMonitorDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAtomMonitorDialog();

private:

	void OnMonitorStateChanged(const FString& HostIP, uint16 HostPort, bool bIsStarted);
	void OnMonitorLog(const FString& LogMsg, EAtomMonitorLogMode LogMode);
	void OnMonitorConnected(const FString& ClientIP, bool bIsConnectedStarted);

	TSharedPtr<SAtomMonitorLogWidget> MonitorLog;

	FDelegateHandle OnAtomMonitorStartedHandle;
	FDelegateHandle	OnAtomMonitorStoppedHandle;
	FDelegateHandle OnAtomMonitorLogHandle;
	FDelegateHandle OnAtomMonitorConnectedHandle;
	FDelegateHandle OnAtomMonitorDisconnectedHandle;

	EActiveTimerReturnType UpdateGridPanels(double InCurrentTime, float InDeltaTime);

	TSharedRef<SWidget> GetGridPanel();



	SVerticalBox::FSlot* GridSlot = nullptr;
};
