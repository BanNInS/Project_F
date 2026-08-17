
#include "AtomSoundscape.h"
#include "AtomSoundscapePalette.h"
#include "AtomSoundscapeColor.h"

bool UAtomSoundscapeBPFunctionLibrary::SpawnSoundscapeColor(UObject* WorldContextObject, class UAtomSoundscapeColor* SoundscapeColorIn, UAtomActiveSoundscapeColor*& ActiveSoundscapeColor)
{
	// Verify that the input SoundscapeElement is not null
	if (SoundscapeColorIn && WorldContextObject)
	{
		// Create a new ActiveSoundscapeColor
		ActiveSoundscapeColor = NewObject<UAtomActiveSoundscapeColor>(WorldContextObject);

		// Initialize parameters
		ActiveSoundscapeColor->SetParameterValues(SoundscapeColorIn);

#if WITH_EDITOR
		// Bind to delegate for live update from the SoundscapeColor Editor
		ActiveSoundscapeColor->BindToParameterChangeDelegate(SoundscapeColorIn);
#endif

		return true;
	}

	return false;
}

bool UAtomSoundscapeBPFunctionLibrary::SpawnSoundscapePalette(UObject* WorldContextObject, class UAtomSoundscapePalette* SoundscapePaletteIn, UAtomActiveSoundscapePalette*& ActiveSoundscapePaletteOut)
{
	// Verify that the input SoundscapeElement is not null
	if (SoundscapePaletteIn && WorldContextObject)
	{
		// Create a new ActiveSoundscapeColor
		ActiveSoundscapePaletteOut = NewObject<UAtomActiveSoundscapePalette>(WorldContextObject);

		// Initialize parameters
		ActiveSoundscapePaletteOut->InitializeSettings(WorldContextObject, SoundscapePaletteIn);

		return true;
	}

	return false;
}
