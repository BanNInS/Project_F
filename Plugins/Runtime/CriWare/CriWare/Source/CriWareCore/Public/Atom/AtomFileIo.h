/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomFileIo.h
 *
 ****************************************************************************/

#pragma once

#include "CriWareFileIo.h"

#define CRI_XPT_DISABLE_UNPREFIXED_TYPE
#if !defined(CRIWARE_UE_LE)
#include <cri_xpt.h>
#include <cri_file_system.h>
#else
#include <cri_le_xpt.h>
#include <cri_le_file_system.h>
#endif

//~ I/O processing interface with IoDispatcher
extern CriFsIoInterface GAtomCriFileIoToIoDispatcherInferface;

//~ I/O processing interface with BulkData streaming
extern CriFsIoInterface GAtomCriFileAsyncInferface;
