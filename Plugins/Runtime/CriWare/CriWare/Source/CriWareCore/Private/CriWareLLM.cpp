

#include "CriWareLLM.h"

#include "HAL/LowLevelMemStats.h"

#include "CriWareCorePrivate.h"

#if ENABLE_LOW_LEVEL_MEM_TRACKER && CRIWARECORE_USE_LLM

uint64 GCiWareLLMAllocationID = 0x0;

struct FLLMTagInfoCriWare
{
	const TCHAR* Name;
	FName StatName;				// shows in the LLMFULL stat group
	FName SummaryStatName;		// shows in the LLM summary stat group
};

DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/Misc"), STAT_CriWareMiscLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/Atom"), STAT_AtomLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomMisc"), STAT_AtomMiscLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomRuntime"), STAT_AtomRuntimeLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomMixer"), STAT_AtomMixerLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomMixerPlugins"), STAT_AtomMixerPluginsLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomAudioSynthesis"), STAT_AtomAudioSynthesisLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/AtomAudioAnalysis"), STAT_AtomAudioAnalysisLLM, STATGROUP_LLMFULL);

DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/Atom"), STAT_AtomSummaryLLM, STATGROUP_LLM);

DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/Mana"), STAT_ManaLLM, STATGROUP_LLMFULL);
DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/ManaMisc"), STAT_ManaMiscLLM, STATGROUP_LLMFULL);

DECLARE_LLM_MEMORY_STAT(TEXT("CriWare/Mana"), STAT_ManaSummaryLLM, STATGROUP_LLM);


// *** order must match ELLMTagCriWare enum ***
static const FLLMTagInfoCriWare ELLMTagNamesCriWare[] =
{
	// csv name								// stat name								// summary stat name					// enum value
	{ TEXT("CriWare/Misc"),					GET_STATFNAME(STAT_CriWareMiscLLM),			GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::CriWareMisc
	{ TEXT("CriWare/Atom"),					GET_STATFNAME(STAT_AtomLLM),				GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::Atom
	{ TEXT("CriWare/AtomMisc"),				GET_STATFNAME(STAT_AtomMiscLLM),			GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomMisc
	{ TEXT("CriWare/AtomRuntime"),			GET_STATFNAME(STAT_AtomRuntimeLLM),			GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomRuntime
	{ TEXT("CriWare/AtomMixer"),			GET_STATFNAME(STAT_AtomMixerLLM),			GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomMixer
	{ TEXT("CriWare/AtomMixerPlugins"),		GET_STATFNAME(STAT_AtomMixerPluginsLLM),	GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomMixerPlugins
	{ TEXT("CriWare/AtomAudioSynthesis"),	GET_STATFNAME(STAT_AtomAudioSynthesisLLM),	GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomAudioSynthesis
	{ TEXT("CriWare/AtomAudioAnalysis"),	GET_STATFNAME(STAT_AtomAudioAnalysisLLM),	GET_STATFNAME(STAT_AtomSummaryLLM) },	// ELLMTagCriWare::AtomAudioAnalysis
	{ TEXT("CriWare/Mana"),					GET_STATFNAME(STAT_ManaLLM),				GET_STATFNAME(STAT_ManaSummaryLLM) },	// ELLMTagCriWare::Mana
	{ TEXT("CriWare/ManaMisc"),				GET_STATFNAME(STAT_ManaMiscLLM),			GET_STATFNAME(STAT_ManaSummaryLLM) },	// ELLMTagCriWare::ManaMisc
};

/*
 * Register CriWareCore tags with LLM
 */
namespace CriWareLLM
{
	void Initialize()
	{
		int32 TagCount = sizeof(ELLMTagNamesCriWare) / sizeof(FLLMTagInfoCriWare);

		for (int32 Index = 0; Index < TagCount; ++Index)
		{
			int32 Tag = ((int32)ELLMTag::ProjectTagEnd - 20) + Index;
			const FLLMTagInfoCriWare& TagInfo = ELLMTagNamesCriWare[Index];

			FLowLevelMemTracker::Get().RegisterProjectTag(Tag, TagInfo.Name, TagInfo.StatName, TagInfo.SummaryStatName);
		}
	}
}

#endif // CRIWARECORE_USE_LLM
//#endif // ENABLE_LOW_LEVEL_MEM_TRACKER
