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

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"

class SAtomResourceUsageDialog
	: public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAtomResourceUsageDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	EActiveTimerReturnType UpdateGridPanels(double InCurrentTime, float InDeltaTime);

	TSharedRef<SWidget> GetGridPanel();

	SVerticalBox::FSlot* GridSlot = nullptr;
};
