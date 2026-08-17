
#include "AtomSoundCueFactory.h"

#include "ClassViewerModule.h"
#include "Kismet2/SClassPickerDialog.h"

#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomCueSheet.h"

#include "CriWareCoreEditorPrivate.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomSoundCueFactory::UAtomSoundCueFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{

    bCreateNew = false;
    SupportedClass = UAtomSoundCue::StaticClass();
}

// for test
UObject* UAtomSoundCueFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    if (CueClass != nullptr)
    {
        return NewObject<UAtomSoundCue>(InParent, CueClass, InName, Flags | RF_Transactional);
    }
    else
    {
        check(InClass->IsChildOf(UAtomSoundCue::StaticClass()));
        return NewObject<UAtomSoundCue>(InParent, InClass, InName, Flags | RF_Transactional);
    }
}

bool UAtomSoundCueFactory::ConfigureProperties()
{
    // nullptr the ItemClass so we can check for selection
    CueClass = nullptr;

    // Load the classviewer module to display a class picker
    FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

    // Fill in options
    FClassViewerInitializationOptions Options;
    Options.Mode = EClassViewerMode::ClassPicker;

    TSharedPtr<FAtomSoundCueFilterViewer> Filter = MakeShareable<FAtomSoundCueFilterViewer>(new FAtomSoundCueFilterViewer);
    Options.ClassFilters.Add(Filter.ToSharedRef());

    Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated;
    Filter->AllowedChildrenOfClasses.Add(UAtomSoundBase::StaticClass());

    const FText TitleText = LOCTEXT("CreateItemOptions", "Pick Item Class");
    UClass* ChosenClass = nullptr;
    const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UAtomSoundBase::StaticClass());

    if (bPressedOk)
    {
        CueClass = ChosenClass;
    }

    return bPressedOk;
}

#undef LOCTEXT_NAMESPACE
