
#include "AtomSoundscapePaletteFactory.h"

#include "AtomSoundscapePalette.h"

UAtomSoundscapePaletteFactory::UAtomSoundscapePaletteFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundscapePalette::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundscapePaletteFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundscapePalette* SoundscapePalette = NewObject<UAtomSoundscapePalette>(InParent, Name, Flags);

	return SoundscapePalette;
}
