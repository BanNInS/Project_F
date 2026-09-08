/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : CriWareCoreEditor
* File     : AtomSoundBankEditorCommands.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * Holds the UI commands for the AtomSoundBankEditorToolkit widget.
 */
class FAtomSoundBankEditorCommands
	: public TCommands<FAtomSoundBankEditorCommands>
{
public:

	/** Default constructor. */
	FAtomSoundBankEditorCommands();

public:

	//~ TCommands<> interface
	virtual void RegisterCommands() override;

public:

	// Copies the selected Asset Paths to the Clipboard
	TSharedPtr<FUICommandInfo> CopyReferences;

	// Shows the reference viewer for the selected assets
	TSharedPtr<FUICommandInfo> ViewReferences;

	// Shows a size map for the selected assets
	TSharedPtr<FUICommandInfo> ViewSizeMap;
};
