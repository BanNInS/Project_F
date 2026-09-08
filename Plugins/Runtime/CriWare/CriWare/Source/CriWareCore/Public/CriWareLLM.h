
#pragma once

#include "HAL/LowLevelMemTracker.h"

#include "CriWareDefines.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER && CRIWARECORE_USE_LLM

#define LLM_SCOPE_CRIWARE(Tag)			LLM_SCOPE((ELLMTag)Tag)
//#define LLM_PLATFORM_SCOPE_CRIWARE(Tag)	LLM_PLATFORM_SCOPE((ELLMTag)Tag)
extern uint64 GCriWareLLMAllocationID;
#define LLM_TRACK_CRIWARE_HIGH_LEVEL_ALLOC(AllocObj, Size) { AllocObj.SetLLMTrackerID(0xDEAD | (++GCriWareLLMAllocationID << 16)); FLowLevelMemTracker::Get().OnLowLevelAlloc(ELLMTracker::Default, (void*)AllocObj.GetLLMTrackerID(), Size, (ELLMTag)ELLMTagCriWare::CriWareMisc); }
#define LLM_TRACK_CRIWARE_HIGH_LEVEL_FREE(AllocObj) { FLowLevelMemTracker::Get().OnLowLevelFree(ELLMTracker::Default, (void*)AllocObj.GetLLMTrackerID()); }

#define LLM_TRACK_CRIWARE_HIGH_LEVEL_ALLOCATION
enum class ELLMTagCriWare : LLM_TAG_TYPE
{
	CriWareMisc = (LLM_TAG_TYPE)ELLMTag::ProjectTagEnd - 20, // Because unreal seams not really flexible with LLM tags, we use the last 20 tags available.
	Atom,
	AtomMisc,
	AtomRuntime,
	AtomMixer,
	AtomMixerPlugins,
	AtomAudioSynthesis,
	AtomAudioAnalysis,
	Mana,
	ManaMisc,

	Count,
};

static_assert((int32)ELLMTagCriWare::Count <= (int32)ELLMTag::ProjectTagEnd, "too many ELLMTagCriWare tags");

namespace CriWareLLM
{
	void Initialize();
}

#else

#define LLM_SCOPE_CRIWARE(...)
//#define LLM_PLATFORM_SCOPE_CRIWARE(...)
#define LLM_TRACK_CRIWARE_HIGH_LEVEL_ALLOC(...)
#define LLM_TRACK_CRIWARE_HIGH_LEVEL_FREE(...)

#endif // CRIWARE_USE_LLM
