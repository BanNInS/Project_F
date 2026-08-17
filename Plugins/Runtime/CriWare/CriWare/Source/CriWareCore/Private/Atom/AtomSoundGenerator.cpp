
#include "Atom/AtomSoundGenerator.h"

#include "CriWareLLM.h"

IAtomSoundGenerator::IAtomSoundGenerator()
{
}

IAtomSoundGenerator::~IAtomSoundGenerator()
{
}

int32 IAtomSoundGenerator::GetNextBuffer(float** OutAudio, int32 NumFrames, int32 NumChannels, bool bRequireNumberSamples)
{
	LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomAudioSynthesis);

	PumpPendingMessages();

	int32 NumFramesToGenerate = NumFrames;
	if (!bRequireNumberSamples)
	{
		NumFramesToGenerate = FMath::Min(NumFrames, GetDesiredNumSamplesToRenderPerCallback() / NumChannels) ;
	}

	return OnGenerateAudio(OutAudio, NumFramesToGenerate, NumChannels);
}

void IAtomSoundGenerator::SynthCommand(TUniqueFunction<void()> Command)
{
	LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomAudioSynthesis);

	CommandQueue.Enqueue(MoveTemp(Command));
}

void IAtomSoundGenerator::PumpPendingMessages()
{
	TUniqueFunction<void()> Command;
	while (CommandQueue.Dequeue(Command))
	{
		Command();
	}
}
