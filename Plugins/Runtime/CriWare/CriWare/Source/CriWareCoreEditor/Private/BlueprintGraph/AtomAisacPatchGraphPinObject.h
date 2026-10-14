
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Atom/AtomAisacPatch.h"
#include "AtomGraphPinObject.h"

#include "AtomAisacPatchGraphPinObject.generated.h"

/** class to hold an AtomAisacPatch property to permit a AisacPatch selection. (Local use only) */
UCLASS(Transient)
class UAtomAisacPatchGraphPinObject
	: public UAtomGraphPinObject
{
	GENERATED_BODY()

private:

	UPROPERTY(Transient, EditAnywhere, Category = Atom, meta = (AllowPrivateAccess))
	TObjectPtr<class UAtomAisacPatch> AisacPatch;

public:

	virtual UObject* GetObject() const override { return AisacPatch; }
	virtual void SetObject(UObject* Object) override { AisacPatch = Cast<UAtomAisacPatch>(Object); }
	virtual UClass* GetObjectClass() const override { return UAtomAisacPatch::StaticClass(); }
};
