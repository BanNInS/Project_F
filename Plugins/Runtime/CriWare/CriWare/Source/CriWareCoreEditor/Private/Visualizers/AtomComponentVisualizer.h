/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomComponentVisualizer.h
 *
 ****************************************************************************/

#pragma once

#include "Visualizers/AtomAttenuatedComponentVisualizer.h"

#include "Atom/AtomComponent.h"

class CRIWARECOREEDITOR_API FAtomComponentVisualizer
	: public TAtomAttenuatedComponentVisualizer<UAtomComponent>
{
private:

	virtual bool IsVisualizerEnabled(const FEngineShowFlags& ShowFlags) const override
	{
		return ShowFlags.AudioRadius;
	}
};
