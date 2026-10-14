/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : SAtomEffectEditor.cpp
 *
 ****************************************************************************/

#include "SAtomEffectEditor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "Styling/StyleColors.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomEffectPreset.h"

#define LOCTEXT_NAMESPACE "CriWareCoreEditor"

void SAtomEffectEditor::Construct(const FArguments& InArgs)
{
	RackToEdit = InArgs._RackToEdit;

	this->ChildSlot
	[
		SNew(SBox)
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("AtomEffectEditorPanel")))
		[
			GetGraphPanel()
		]
	];

	// Refresh every 1/2 seconds
	RegisterActiveTimer(0.5f, FWidgetActiveTimerDelegate::CreateSP(this, &SAtomEffectEditor::UpdateGraph));
}

EActiveTimerReturnType SAtomEffectEditor::UpdateGraph(double InCurrentTime, float InDeltaTime)
{
	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> SAtomEffectEditor::GetGraphPanel()
{
	//return SNullWidget::NullWidget;

	TSharedRef<SGridPanel> Panel = SNew(SGridPanel);

	return Panel;
}

#undef LOCTEXT_NAMESPACE
