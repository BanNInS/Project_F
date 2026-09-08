/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareUtils.cpp
 *
 ****************************************************************************/

#include "CriWareUtils.h"

#include "CriWare.h"

#define ATOM_TRACE_SDK_HANDLE_DESTROY 0

 /*
  * Smart TCriHandle
  *****************************************************************************/

template<> void FCriAtomVoicePoolPtr::Destroy(CriAtomExVoicePoolHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomVoicePoolPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomExVoicePool_Free(Handle); // may blocking if something is playing
	}
}

template<> void FCriAtomPlayerPtr::Destroy(CriAtomPlayerHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomPlayerPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomPlayer_Destroy(Handle); // async
	}
}

template<> void FCriAtomExPlayerPtr::Destroy(CriAtomExPlayerHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExPlayerPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomExPlayer_Destroy(Handle); // async
	}
}

template<> void FCriAtomExSourcePtr::Destroy(CriAtomEx3dSourceHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExSourcePtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomEx3dSource_Destroy(Handle);
	}
}

template<> void FCriAtomExSourceListPtr::Destroy(CriAtomEx3dSourceListHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExSourceListPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomEx3dSourceList_Destroy(Handle);
	}
}

template<> void FCriAtomExListenerPtr::Destroy(CriAtomEx3dListenerHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExListenerPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomEx3dListener_Destroy(Handle);
	}
}

template<> void FCriAtomExSoundObjectPtr::Destroy(CriAtomExSoundObjectHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExSoundObjectPtr <%p>"), Handle);
#endif
		FCriWareApi::criAtomExSoundObject_Destroy(Handle);
	}
}

template <> void FCriAtomAwbPtr::Destroy(CriAtomAwbHn Handle)
{
	if (FCriWareApi::criAtomEx_IsInitialized() == CRI_TRUE)
	{
		if (Handle)
		{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
			UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomAwbPtr <%p>"), Handle);
#endif
			FCriWareApi::criAtomAwb_Release(Handle);
		}
	}
}

template <> void FCriAtomExAcbPtr::Destroy(CriAtomExAcbHn Handle)
{
	if (FCriWareApi::criAtomEx_IsInitialized() == CRI_TRUE)
	{
		if (Handle)
		{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
			UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExAcbPtr <%p>"), Handle);
#endif
			FCriWareApi::criAtomExAcb_Release(Handle);
		}
	}
}

template <> void FCriFsBinderPtr::Destroy(CriFsBinderHn Handle)
{
	if (Handle)
	{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
		UE_LOG(LogTemp, Warning, TEXT("Destroy FCriFsBinderPtr <%p>"), Handle);
#endif
		FCriWareApi::criFsBinder_Destroy(Handle);
	}
}

/*
 * Smart TCriId  
 *****************************************************************************/

template <> void FCriAtomDbasId::Destroy(CriAtomExDbasId Id)
{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
	UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomDbasId [%d]"), Id);
#endif
	FCriWareApi::criAtomDbas_Destroy(Id);
}

template <> void FCriAtomExAsrRackId::Destroy(CriAtomExAsrRackId Id)
{
#if ATOM_TRACE_SDK_HANDLE_DESTROY
	UE_LOG(LogTemp, Warning, TEXT("Destroy FCriAtomExAsrRackId [%d]"), Id);
#endif
	FCriWareApi::criAtomExAsrRack_Destroy(Id);
}
