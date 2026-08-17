/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomConfig.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "UObject/Class.h"
#include "UObject/Interface.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Templates/SubclassOf.h"

//#include "AtomPanelWidgetInterface.h"
#include "Atom/AtomEffectPreset.h"

#include "AtomEffectPresetWidgetInterface.generated.h"

UINTERFACE()
class CRIWARECORE_API UAtomPanelWidgetInterface
	: public UInterface
{
	GENERATED_BODY()
};

class CRIWARECORE_API IAtomPanelWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	FText GetEditorName();

	UFUNCTION(BlueprintImplementableEvent)
	FName GetIconBrushName();
};

UINTERFACE(Blueprintable)
class CRIWARECORE_API UAtomEffectPresetWidgetInterface
	: public UAtomPanelWidgetInterface
{
	GENERATED_BODY()
};

class CRIWARECORE_API IAtomEffectPresetWidgetInterface
	: public IAtomPanelWidgetInterface
{
	GENERATED_BODY()

public:
	// Returns the class of Preset the widget supports
	UFUNCTION(BlueprintImplementableEvent)
	TSubclassOf<UAtomEffectPreset> GetClass();

	// Called when the preset widget is constructed
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On AtomEffectPreset Widget Constructed"))
	void OnConstructed(UAtomEffectPreset* Preset);

	// Called when the preset object is changed
	UFUNCTION(BlueprintImplementableEvent)
	void OnPropertyChanged(UAtomEffectPreset* Preset, FName PropertyName);
};
