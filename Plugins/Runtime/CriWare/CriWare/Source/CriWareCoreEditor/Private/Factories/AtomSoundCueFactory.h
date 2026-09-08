#pragma once

#include "Factories/Factory.h"
#include "ClassViewerFilter.h"

#include "Atom/AtomSoundCue.h"

#include "AtomSoundCueFactory.generated.h"

// Forward Declarations
class UAtomSoundCue;

UCLASS()
class UAtomSoundCueFactory
    : public UFactory
{
    GENERATED_BODY()

public:

    UAtomSoundCueFactory(const FObjectInitializer& ObjectInitializer);

    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool ConfigureProperties() override;

    UPROPERTY(EditAnywhere, Category = Item)
    TSubclassOf<UAtomSoundCue> CueClass;
};

class FAtomSoundCueFilterViewer : public IClassViewerFilter
{
public:
    TSet<const UClass*> AllowedChildrenOfClasses;
    EClassFlags DisallowedClassFlags;

    virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<class FClassViewerFilterFuncs> InFilterFuncs) override
    {
        return !InClass->HasAnyClassFlags(DisallowedClassFlags)
            && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
    }

    virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
    {
        return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
            && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
    }
};
