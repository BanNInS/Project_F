/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusDetails.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

#include "Widgets/Input/SComboBox.h"

// Forward declarations
class UAtomBus;
enum class EAtomSpeakerChannelMap : uint8;
enum class EAtomSoundRendererType : uint8;
enum class EAtomSoundfieldRendererType : uint8;

class CRIWARECOREEDITOR_API FAtomBusDetails
	: public IDetailCustomization
{
public:

	// Makes a new instance of this detail layout class
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface

private:

	void GenerateBusEffectPresetWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder);

	struct FAtomBusMixerOutputInfo
	{
		EAtomSpeakerChannelMap SpeakerChannelMap;
		EAtomSoundRendererType SoundRendererType;
		EAtomSoundfieldRendererType SoundfieldRendererType;
	};

	FAtomBusMixerOutputInfo GetBusOutputInfoFromConfig(const UAtomBus* InAtomBus) const;
};
