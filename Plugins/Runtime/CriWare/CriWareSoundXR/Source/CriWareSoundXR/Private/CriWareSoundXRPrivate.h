#pragma once

#include "Logging/LogMacros.h"

DECLARE_STATS_GROUP(TEXT("CRIWARESOUNDXR"), STATGROUP_CriWareSoundXR, STATCAT_Advanced);

/** Log category for the CriWareSoundXR module. */
DECLARE_LOG_CATEGORY_EXTERN(LogCriWareSoundXR, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogCriWareSoundXRDebug, Log, All);

#include "CriWareLocalization.h"
