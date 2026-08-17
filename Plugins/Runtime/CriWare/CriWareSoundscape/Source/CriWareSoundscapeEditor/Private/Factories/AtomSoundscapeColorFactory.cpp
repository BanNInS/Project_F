
#include "AtomSoundscapeColorFactory.h"

#include "AtomSoundscapeColor.h"

UAtomSoundscapeColorFactory::UAtomSoundscapeColorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundscapeColor::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundscapeColorFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundscapeColor* SoundscapeColor = NewObject<UAtomSoundscapeColor>(InParent, Name, Flags);

	return SoundscapeColor;
}