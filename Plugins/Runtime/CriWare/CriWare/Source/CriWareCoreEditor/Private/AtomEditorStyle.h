/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEditorStyle.h
 *
 ****************************************************************************/

#pragma once

#include "Styling/SlateStyle.h"

/**
 * Implements the visual style of CriWare ADX for editor UI.
 */
class FAtomEditorStyle
	: public FSlateStyleSet
{
public:

	/** Default constructor. */
	FAtomEditorStyle();

	/** Destructor. */
	~FAtomEditorStyle();

	/** Accessors. */
	static const ISlateStyle& Get();

private:

	static FName AtomStyleName;
};
