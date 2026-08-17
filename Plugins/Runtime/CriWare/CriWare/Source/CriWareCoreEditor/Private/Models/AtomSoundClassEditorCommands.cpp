
#include "AtomSoundClassEditorCommands.h"

#include "Framework/Commands/InputChord.h"
#include "Framework/Commands/UICommandInfo.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "AtomSoundClassEditorCommands"

FAtomSoundClassEditorCommands::FAtomSoundClassEditorCommands()
	: TCommands<FAtomSoundClassEditorCommands>(
		"AtomSoundClassEditor", 
		NSLOCTEXT("Contexts", "AtomSoundClassEditor", "AtomSoundClass Editor"),
		NAME_None,
		"AtomEditorStyle")
{
}

void FAtomSoundClassEditorCommands::RegisterCommands()
{
	UI_COMMAND(ToggleSolo, "Solo", "Toggles Soloing this sound class", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::S));
	UI_COMMAND(ToggleMute, "Mute", "Toggles Muting this sound class", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::M));
}

#undef LOCTEXT_NAMESPACE
