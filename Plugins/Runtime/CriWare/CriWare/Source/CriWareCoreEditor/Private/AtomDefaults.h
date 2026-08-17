/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomDefaults.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Atom/AtomRack.h"

namespace Atom
{
	//~ AtomConfig defaults and utilities

	const FColor DefaultConfigColor = FColor::Red;
	const FColor DefaultDspBusSettingColor = FColor(127, 127, 127);

	//~ AtomSound defaults and utilities

	const FColor DefaultSoundBaseColor = FColor(97, 85, 212);
	const FColor DefaultWaveBankColor = FColor(97, 85, 212);
	const FColor DefaultSoundWaveColor = FColor(97, 85, 212);
	const FColor DefaultCueSheetColor = FColor(255, 127, 39);
	const FColor DefaultSoundCueColor = FColor(255, 127, 39);
	const FColor DefaultAttenuationColor = FColor(77, 120, 239);
	const FColor DefaultConcurrencyColor = FColor(77, 100, 139);
	const FColor DefaultSoundClassColor = FColor(255, 175, 0);
	const FColor DefaultSoundSimpleColor = FColor(212, 97, 85);
	const FColor DefaultSoundProxyColor = FColor(141, 139, 85);
	const FColor DefaultSoundSourceBusColor = FColor(212, 97, 85);

	//~ Atom Modulation and Aisac defaults and utilities

	const FColor DefaultModulationGeneratorColor = FColor(204, 51, 153); // USER, UObject, generate modulation for controls
	const FColor DefaultModulationControlBusColor = FColor(255, 51, 153); // USER + ACF, UStruct (copied), aisac control == modulation bus (defined by Craft)
	const FColor DefaultModulationControlBusMixColor = FColor(255, 153, 153); // USER, UObject link controls with modulable values
	const FColor DefaultModulationPatchColor = FColor(255, 204, 255); // USER, define a parameter in engine, can wrap an aisac control value
	const FColor DefaultModulationParameterColor = FColor(255, 102, 153); // USER, define a parameter in engine, can wrap an aisac control value
	const FColor DefaultAisacPatchColor = FColor(255, 51, 102); // ACF, UObject, Represent an aisac definition (defined by Craft)
	//const FColor DefaultAisacControlColor = FColor(255, 102, 102); // ATOM, UEnum, aisac not modifiable (defined by Craft)

	/* UE AudioModulation as reference
	static const FColor GetModulationGeneratorColor() { return FColor(204, 51, 153); }
	static const FColor GetControlBusColor() { return FColor(255, 51, 153); }
	static const FColor GetControlBusMixColor() { return FColor(255, 153, 153); }
	static const FColor GetPatchColor() { return FColor(255, 204, 255); }
	static const FColor GetParameterColor() { return FColor(255, 102, 153); }
	*/

	//~ AtomRack defaults and utilities

	const FColor DefaultRackColor = FColor(143, 190, 0);
	const FColor SoundfieldDefaultRackColor = FColor(0, 168, 198);
	const FColor EndpointDefaultRackColor = FColor(249, 242, 231);
	const FColor SoundfieldEndpointDefaultRackColor = FColor(64, 192, 203);
	const FColor DefaultBusColor = FColor(97, 97, 85);
	const FColor DefaultAudioBusColor = FColor(97, 97, 85);
	const FColor DefaultEffectPresetColor = FColor(99, 63, 56);

	// These names are used to identify pins by their owning node.
	const FName RackName = TEXT("AtomRack");
	const FName SoundfieldRackName = TEXT("SoundfieldRack");
	const FName EndpointRackName = TEXT("AtomEndpointRack");
	const FName SoundfieldEndpointRackName = TEXT("SoundfieldEndpointRack");

	// These utility functions are used
	FName GetNameForRackType(const UAtomRackBase* InRack);
	FColor GetColorForRackType(const UAtomRackBase* InRack);
	FColor GetColorForRackType(const FName& InRackName);
	const bool IsConnectionPerformingSoundfieldConversion(const UAtomRackBase* InputRack, const UAtomRackBase* OutputRack);
}
