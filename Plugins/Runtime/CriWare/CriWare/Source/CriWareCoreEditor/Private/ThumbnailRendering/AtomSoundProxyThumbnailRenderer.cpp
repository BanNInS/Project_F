/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundProxyThumbnailRenderer.cpp
 *
 ****************************************************************************/

#include "AtomSoundProxyThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"

#include "Atom/AtomSoundProxy.h"

bool UAtomSoundProxyThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	if (UAtomSoundProxy* Proxy = Cast<UAtomSoundProxy>(Object))
	{
		return Super::CanVisualizeAsset(Proxy->GetLeafSound());
	}

	return false;
}

void UAtomSoundProxyThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	if (UAtomSoundProxy* Proxy = Cast<UAtomSoundProxy>(Object))
	{
		Super::Draw(Proxy->GetLeafSound(), X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
	}
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
EThumbnailRenderFrequency UAtomSoundProxyThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	if (UAtomSoundProxy* Proxy = Cast<UAtomSoundProxy>(Object))
	{
		return Super::GetThumbnailRenderFrequency(Proxy->GetLeafSound());
	}

	return EThumbnailRenderFrequency::OnPropertyChange;
}
#else
bool UAtomSoundProxyThumbnailRenderer::AllowsRealtimeThumbnails(UObject* Object) const
{
	if (UAtomSoundProxy* Proxy = Cast<UAtomSoundProxy>(Object))
	{
		return Super::AllowsRealtimeThumbnails(Proxy->GetLeafSound());
	}

	return false;
}
#endif