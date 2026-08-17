/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationPatchFactory.cpp
 *
 ****************************************************************************/

#include "AtomModulationPatchFactory.h"

#include "CriWareFactoryUtility.h"
#include "Atom/Modulation/AtomModulationPatch.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom Modulation Patch Factory
 *****************************************************************************/

UAtomModulationPatchFactory::UAtomModulationPatchFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomModulationPatch::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomModulationPatchFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	//Atom::Analytics::RecordEvent_Usage(TEXT("AtomModulation.ParameterPatchCreated"));
	UAtomModulationPatch* NewPatch = NewObject<UAtomModulationPatch>(InParent, InName, Flags);

	if (NewPatch)
	{
		NewPatch->PatchSettings.bBypass = false;

		FAtomControlModulationInput DefaultInput;

		DefaultInput.Transform.Curve = EWaveTableCurve::Custom;
		DefaultInput.Transform.CurveCustom.AddKey(0.0f, 0.0f);
		DefaultInput.Transform.CurveCustom.AddKey(1.0f, 1.0f);

		NewPatch->PatchSettings.Inputs.Add(DefaultInput);
	}

	return NewPatch;
}

bool UAtomModulationPatchFactory::CanCreateNew() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE
