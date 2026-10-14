/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2015-2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : File I/O Interface
 * File     : CriWareFileIo.h
 *
 ****************************************************************************/

#pragma once

#define CRI_XPT_DISABLE_UNPREFIXED_TYPE
#if !defined(CRIWARE_UE_LE)
#include <cri_xpt.h>
#include <cri_file_system.h>
#else
#include <cri_le_xpt.h>
#include <cri_le_file_system.h>
#endif

namespace CriWareFileIo
{
	/** */
	CriError CRIAPI SelectIo(const CriChar8 *Filename, CriFsDeviceId *DeviceId, CriFsIoInterfacePtr *IoInterface);
};
