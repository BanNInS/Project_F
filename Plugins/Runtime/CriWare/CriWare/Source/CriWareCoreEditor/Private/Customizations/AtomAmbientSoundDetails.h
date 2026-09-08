/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAmbientSoundDetails.h
 *
 ****************************************************************************/

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

// Forward Declarations
class IDetailLayoutBuilder;
class SWidget;

class FAtomAmbientSoundDetails
	: public IDetailCustomization
{
public:
	
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:

	//~ Begin IDetailCustomization
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	//~ End IDetailCustomization

	bool IsEditSoundEnabled() const;

	FReply OnEditSoundClicked();
	FReply OnPlaySoundClicked();
	FReply OnStopSoundClicked();

	TWeakObjectPtr<class AAtomAmbientSound> AmbientSound;
};
