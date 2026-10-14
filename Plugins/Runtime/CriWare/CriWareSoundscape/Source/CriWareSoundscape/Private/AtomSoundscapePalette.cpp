
#include "AtomSoundscapePalette.h"

#include "AtomSoundscapeColor.h"

UAtomSoundscapePalette::UAtomSoundscapePalette()
{
}

void UAtomSoundscapePalette::PostLoad()
{
	Super::PostLoad();
}

void UAtomSoundscapePalette::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

void UAtomActiveSoundscapePalette::InitializeSettings(UObject* WorldContextObject, UAtomSoundscapePalette* SoundscapePalette)
{
	if (SoundscapePalette && WorldContextObject)
	{
		// Set up Soundscape Colors
		for (FAtomSoundscapePaletteColor& SoundscapePaletteColor : SoundscapePalette->Colors)
		{
			if (UAtomSoundscapeColor* SoundscapeColor = SoundscapePaletteColor.SoundscapeColor)
			{
				// Verify that the input SoundscapeElement is not null
				if (SoundscapeColor)
				{
					// Create a new ActiveSoundscapeColor
					UAtomActiveSoundscapeColor* ActiveSoundscapeColor = NewObject<UAtomActiveSoundscapeColor>(WorldContextObject);

					//
					ActiveSoundscapeColor->SetParameterValues(SoundscapeColor);

#if WITH_EDITOR
					ActiveSoundscapeColor->BindToParameterChangeDelegate(SoundscapeColor);
#endif //WITH_EDITOR
					ActiveSoundscapeColor->VolumeMod = SoundscapePaletteColor.ColorVolume;
					ActiveSoundscapeColor->PitchMod = SoundscapePaletteColor.ColorPitch;
					ActiveSoundscapeColor->FadeInMin = SoundscapePaletteColor.ColorFadeIn;

					// Playback stop values
					ActiveSoundscapeColor->FadeOutMin = SoundscapePaletteColor.ColorFadeOut;

					// Add to list
					ActiveSoundscapeColors.Add(ActiveSoundscapeColor);
				}

			}
		}
	}
}

void UAtomActiveSoundscapePalette::Play()
{
	for (UAtomActiveSoundscapeColor* ActiveSoundscapeColor : ActiveSoundscapeColors)
	{
		if (ActiveSoundscapeColor)
		{
			ActiveSoundscapeColor->PlayNative();
		}
	}
}

void UAtomActiveSoundscapePalette::Stop()
{
	for (UAtomActiveSoundscapeColor* ActiveSoundscapeColor : ActiveSoundscapeColors)
	{
		if (ActiveSoundscapeColor)
		{
			ActiveSoundscapeColor->StopNative();
		}
	}

}

