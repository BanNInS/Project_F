/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : CriWareCoreEditor
* File     : AtomSoundBankEditorCommands.cpp
*
****************************************************************************/

#include "AtomSoundBankEditorCommands.h"

#define LOCTEXT_NAMESPACE "FAtomSoundBankEditorCommands"

FAtomSoundBankEditorCommands::FAtomSoundBankEditorCommands() : TCommands<FAtomSoundBankEditorCommands>(
		"AtomSoundBankEditor", 
		NSLOCTEXT("Contexts", "AtomSoundBankEditor", "AtomSoundBank Editor"), 
		NAME_None, 
		"AtomEditorStyle")
{}

void FAtomSoundBankEditorCommands::RegisterCommands()
{
	UI_COMMAND(CopyReferences, "Copy Reference", "Copies reference paths for the selected sound assets to the clipboard.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::C));
	UI_COMMAND(ViewReferences, "Reference Viewer...", "Launches the reference viewer showing the selected sound assets' references", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::R));
	UI_COMMAND(ViewSizeMap, "Size Map...", "Displays an interactive map showing the approximate size of parent sound bank asset and everything it references", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::M));
}

#undef LOCTEXT_NAMESPACE
