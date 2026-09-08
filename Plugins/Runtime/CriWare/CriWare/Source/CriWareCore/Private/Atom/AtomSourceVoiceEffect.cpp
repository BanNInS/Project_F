	
#include "Atom/AtomSourceVoiceEffect.h"

#include "CriWareApi.h"
#include "Atom/Atom.h"
    
void FAtomSourceVoiceEffect::SetPitchShifter(const FAtomSourceEffectPitchShifterSettings& InPitchShifterSettings)
{
    Type = EAtomSourceVoiceEffectType::PitchShifter;
    Params.SetNumZeroed(3);
    Params[CRIATOMDSP_PITCHSHIFTER_PARAM_PITCH] = Atom::GetFrequencyMultiplier(InPitchShifterSettings.Pitch); // ratio
    Params[CRIATOMDSP_PITCHSHIFTER_PARAM_FORMANT] = Atom::GetFrequencyMultiplier(InPitchShifterSettings.Formant); // ratio
    Params[CRIATOMDSP_PITCHSHIFTER_PARAM_MODE] = (float)InPitchShifterSettings.Mode;
}

void FAtomSourceVoiceEffect::SetTimeStretch(const FAtomSourceEffectTimeStretchSettings& InTimeStretchSettings)
{
    Type = EAtomSourceVoiceEffectType::TimeStretch;
    Params.SetNumZeroed(3);
    Params[CRIATOMDSP_TIMESTRETCH_PARAM_RATIO] = InTimeStretchSettings.Ratio;
    Params[CRIATOMDSP_TIMESTRETCH_PARAM_FRAME_TIME] = (float)InTimeStretchSettings.FrameTime; // ms
    Params[CRIATOMDSP_TIMESTRETCH_PARAM_QUALITY] = InTimeStretchSettings.Quality;
}

void FAtomSourceVoiceEffect::SetPitchShifterPitch(float InSemitones)
{
    if (Type == EAtomSourceVoiceEffectType::PitchShifter)
    {
        Params[CRIATOMDSP_PITCHSHIFTER_PARAM_PITCH] = Atom::GetFrequencyMultiplier(InSemitones);
    }
}

void FAtomSourceVoiceEffect::SetPitchShifterFormant(float InSemitones)
{
    if (Type == EAtomSourceVoiceEffectType::PitchShifter)
    {
        Params[CRIATOMDSP_PITCHSHIFTER_PARAM_FORMANT] = Atom::GetFrequencyMultiplier(InSemitones);
    }
}

void FAtomSourceVoiceEffect::SetTimeStretchRatio(float InRatio)
{
    if (Type == EAtomSourceVoiceEffectType::TimeStretch)
    {
        Params[CRIATOMDSP_TIMESTRETCH_PARAM_RATIO] = InRatio;
    }
}

void FAtomSourceVoiceEffect::Reset()
{
    Type = EAtomSourceVoiceEffectType::Disabled;
    Params.Reset();
}

float FAtomSourceVoiceEffect::GetPitchShifterPitch() const
{
	return Type == EAtomSourceVoiceEffectType::PitchShifter && Params.IsValidIndex(CRIATOMDSP_PITCHSHIFTER_PARAM_PITCH) ? Params[CRIATOMDSP_PITCHSHIFTER_PARAM_PITCH] : 0.0f;
}

float FAtomSourceVoiceEffect::GetPitchShifterFormant() const
{
	return Type == EAtomSourceVoiceEffectType::PitchShifter && Params.IsValidIndex(CRIATOMDSP_PITCHSHIFTER_PARAM_FORMANT) ? Params[CRIATOMDSP_PITCHSHIFTER_PARAM_FORMANT] : 0.0f;
}

float FAtomSourceVoiceEffect::GetTimeStretchRatio() const
{
	return Type == EAtomSourceVoiceEffectType::TimeStretch && Params.IsValidIndex(CRIATOMDSP_TIMESTRETCH_PARAM_RATIO) ? Params[CRIATOMDSP_TIMESTRETCH_PARAM_RATIO] : 1.0f;
}
