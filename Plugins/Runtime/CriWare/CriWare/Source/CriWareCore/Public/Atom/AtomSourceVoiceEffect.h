#pragma once

#include "UObject/Object.h"

#include "Atom/Effects/AtomBusEffectPitchShifter.h"

#include "AtomSourceVoiceEffect.generated.h"

// Forward Definitions
namespace Atom
{
	class FAtomExPlayback;
	class FMixerSourceVoice;
}

/** */
USTRUCT(BlueprintType)
struct FAtomSourceEffectPitchShifterSettings
{
	GENERATED_BODY()

public:

	/** Specify the shift amount in semitones of the pitch shifter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", meta = (ClampMin = "-24", ClampMax = "24", UIMin = "-24", UIMax = "24"))
	float Pitch = 0.0f;

	/** Specifies the shift amount in semitones of the formant (harmonic frequencies that occur in the human voice) of the pitch shifter when using Vocal or Speach modes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", meta = (ClampMin = "-24", ClampMax = "24", UIMin = "-24", UIMax = "24"))
	float Formant = 0.0f;

	/** Specifies the operation mode of the pitch shifter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter")
	EAtomPitchShifterMode Mode = EAtomPitchShifterMode::Music;
};

/** */
USTRUCT(BlueprintType)
struct FAtomSourceEffectTimeStretchSettings
{
	GENERATED_BODY()

public:

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Stretch", meta = (ClampMin = "0.5", ClampMax = "2.0", UIMin = "0.5", UIMax = "2.0", ForceUnits = x))
	float Ratio = 1.0f;

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Stretch", meta = (ClampMin = "10", ClampMax = "60", UIMin = "10", UIMax = "60", ForceUnits = ms))
	int32 FrameTime = 20;

	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Stretch", meta = (ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10"))
	int32 Quality = 4.0f;
};

/** */
UENUM(BlueprintType)
enum class EAtomSourceVoiceEffectType : uint8
{
    Disabled = 0,
    PitchShifter,
    TimeStretch
};

/** Describes the voice effect attached to Atom mixer voices. */
struct FAtomSourceVoiceEffect
{
public:

	void SetPitchShifter(const FAtomSourceEffectPitchShifterSettings& InPitchShifterSettings);
	void SetTimeStretch(const FAtomSourceEffectTimeStretchSettings& InTimeStretchSettings);
	void SetPitchShifterPitch(float InPitch);
	void SetPitchShifterFormant(float InFormant);
	void SetTimeStretchRatio(float InRatio);

	void Reset();

	inline EAtomSourceVoiceEffectType GetType() const { return Type; }
	float GetPitchShifterPitch() const;
	float GetPitchShifterFormant() const;
	float GetTimeStretchRatio() const;

private:

	/** Voice effect to use. */
	EAtomSourceVoiceEffectType Type = EAtomSourceVoiceEffectType::Disabled;

	/** DSP parameters of the effect. */
	TArray<float> Params;

	friend Atom::FAtomExPlayback;
	friend Atom::FMixerSourceVoice;
};
