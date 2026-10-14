
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Atom/AtomSoundBase.h"
#include "AtomGraphPinObject.h"

#include "AtomSoundGraphPinObject.generated.h"

/** class to hold a Sound property to permit a Sound selection. (Local Use only) */
UCLASS(Transient)
class UAtomSoundGraphPinObject
	: public UAtomGraphPinObject
{
	GENERATED_BODY()

private:

	UPROPERTY(Transient, EditAnywhere, Category = Sound, meta = (AllowPrivateAccess))
	TObjectPtr<UAtomSoundBase> Sound;

public:

	virtual UObject* GetObject() const override { return Sound; }
	virtual void SetObject(UObject* Object) override { Sound = Cast<UAtomSoundBase>(Object); }
	virtual UClass* GetObjectClass() const override { return UAtomSoundBase::StaticClass(); }
};
