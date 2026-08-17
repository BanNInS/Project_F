/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffect3BandEQ.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"
#include "Atom/Effects/AtomBusEffectMultiBandEQ.h"

#include "AtomBusEffect3BandEQ.generated.h"

/*
 * UAtomBusEffect3BandEQPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffect3BandEQPreset
	: public UAtomBusEffectMultiBandEQPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffect3BandEQPreset(const FObjectInitializer& ObjectInitializer);

protected:

	virtual int GetEqualizerNumBands() override { return 3; }
};

