
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CriWareSoundscapeModule.h"
#include "AtomSoundscapeColor.h"
#include "AtomSoundscapePalette.h"

#include "AtomSoundscape.generated.h"

UCLASS()
class CRIWARESOUNDSCAPE_API UAtomSoundscapeBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/** Spawns a new Soundscape Elemental Agent. Returns true if spawn was successful. */
	UFUNCTION(BlueprintCallable, Category = "Soundscape", meta = (WorldContext = "WorldContextObject"))
	static bool SpawnSoundscapeColor(UObject* WorldContextObject, class UAtomSoundscapeColor* SoundscapeColorIn, UAtomActiveSoundscapeColor*& ActiveSoundscapeColor);

	/** Spawns a new Soundscape Palette Agent. Returns true if spawn was successful. */
	UFUNCTION(BlueprintCallable, Category = "Soundscape", meta = (WorldContext = "WorldContextObject"))
	static bool SpawnSoundscapePalette(UObject* WorldContextObject, class UAtomSoundscapePalette* SoundscapePaletteIn, UAtomActiveSoundscapePalette*& SoundscapePaletteAgentsOut);
};
