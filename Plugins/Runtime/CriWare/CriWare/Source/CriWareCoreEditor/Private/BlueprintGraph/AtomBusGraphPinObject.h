
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Atom/AtomBus.h"
#include "AtomGraphPinObject.h"

#include "AtomBusGraphPinObject.generated.h"

/** class to hold an AtomBus property to permit a Bus selection. (Local use only) */
UCLASS(Transient)
class UAtomBusGraphPinObject
	: public UAtomGraphPinObject
{
	GENERATED_BODY()

private:

	UPROPERTY(Transient, EditAnywhere, Category = Atom, meta = (AllowPrivateAccess))
	TObjectPtr<class UAtomBus> Bus;

public:

	virtual UObject* GetObject() const override { return Bus; }
	virtual void SetObject(UObject* Object) override { Bus = Cast<UAtomBus>(Object); }
	virtual UClass* GetObjectClass() const override { return UAtomBus::StaticClass(); }
};
