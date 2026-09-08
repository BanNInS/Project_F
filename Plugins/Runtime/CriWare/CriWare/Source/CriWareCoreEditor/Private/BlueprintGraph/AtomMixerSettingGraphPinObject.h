
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "Atom/AtomConfig.h"
#include "AtomGraphPinObject.h"

#include "AtomMixerSettingGraphPinObject.generated.h"

/** class to hold an UAtomDspBusSetting property to permit a setting selection. (Local use only) */
UCLASS(Transient)
class UAtomMixerSettingGraphPinObject
	: public UAtomGraphPinObject
{
	GENERATED_BODY()

private:

	UPROPERTY(Transient, EditAnywhere, Category = Atom, meta = (AllowPrivateAccess))
	TObjectPtr<class UAtomDspBusSetting> DspBusSetting;

public:

	virtual UObject* GetObject() const { return DspBusSetting; }
	virtual void SetObject(UObject* Object) { DspBusSetting = Cast<UAtomDspBusSetting>(Object); }
	virtual UClass* GetObjectClass() const override { return UAtomDspBusSetting::StaticClass(); }
};
