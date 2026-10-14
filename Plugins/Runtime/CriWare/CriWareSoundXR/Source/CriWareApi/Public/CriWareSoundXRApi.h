
#pragma once

#define CRI_XPT_DISABLE_UNPREFIXED_TYPE
#include <cri_le_xpt.h>
#include <crisoundxr_le_plugin.h>

// Function Definitions
typedef const void* (CRIAPI* criSoundxR_GetInterface_TYPE)(void);

// Function Wrapper
class CRIWARECORE_API FCriWareSoundXRApi
{
public:
	static criSoundxR_GetInterface_TYPE criSoundxR_GetInterface;
};
