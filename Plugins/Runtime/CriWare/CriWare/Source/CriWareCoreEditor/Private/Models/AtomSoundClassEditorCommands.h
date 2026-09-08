
#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Internationalization/Internationalization.h"
#include "Styling/AppStyle.h"
#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"
#include "UObject/UnrealNames.h"

class FUICommandInfo;

/*-----------------------------------------------------------------------------
   FAtomSoundClassEditorCommands
-----------------------------------------------------------------------------*/

class FAtomSoundClassEditorCommands
	: public TCommands<FAtomSoundClassEditorCommands>
{
public:

	/** Constructor */
	FAtomSoundClassEditorCommands();
	
	/** Initialize commands */
	virtual void RegisterCommands() override;

public:

	/** Plays the SoundCue or stops the currently playing cue/node */
	TSharedPtr<FUICommandInfo> ToggleSolo;

	/** Plays the SoundCue or stops the currently playing cue/node */
	TSharedPtr<FUICommandInfo> ToggleMute;
};
