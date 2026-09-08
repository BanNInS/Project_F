/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBaseThumbnailRenderer.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomSoundBaseThumbnailRenderer.generated.h"

class FCanvas;
class FRenderTarget;

/**
 * This thumbnail renderer generates a render of a waveform from a cue.
 */
UCLASS()
class UAtomSoundBaseThumbnailRenderer
	: public UThumbnailRenderer
{
	GENERATED_BODY()

public:

	//~ Begin UThumbnailRenderer Object
	virtual bool CanVisualizeAsset(UObject* Object);
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas, bool bAdditionalViewFamily) override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	virtual EThumbnailRenderFrequency GetThumbnailRenderFrequency(UObject* Object) const override;
#else
	virtual bool AllowsRealtimeThumbnails(UObject* Object) const override;
#endif
	//~ End UThumbnailRenderer Object
};
