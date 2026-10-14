/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundSimple.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "AtomSoundBase.h"
#include "AtomSoundWave.h"

#include "AtomSoundSimple.generated.h"

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomSoundVariation
{
	GENERATED_USTRUCT_BODY()

	/** The Atom sound wave asset to use for this variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundVariation")
	TObjectPtr<UAtomSoundWave> SoundWave;

	/** The probability weight to use for this variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float ProbabilityWeight;

	/** The volume range to use for this variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	FVector2D VolumeRange;

	/** The pitch range to use for this variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	FVector2D PitchRange;

	FAtomSoundVariation()
		: SoundWave(nullptr)
		, ProbabilityWeight(1.0f)
		, VolumeRange(1.0f, 1.0f)
		, PitchRange(1.0f, 1.0f)
	{
	}
};

/** Class which contains a simple list of Atom sound wave variations. */
UCLASS(ClassGroup = Atom, meta = (BlueprintSpawnableComponent))
class CRIWARECORE_API UAtomSoundSimple
	: public UAtomSoundBase
{
	GENERATED_BODY()

public:

	UAtomSoundSimple(const FObjectInitializer& ObjectInitializer);

	/** List of variations for the simple sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variations")
	TArray<FAtomSoundVariation> Variations;

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	//~ Begin USoundBase Interface.
	virtual bool IsPlayable() const override;
	virtual void Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& PlaybackInstances) override;
	virtual float GetMaxDistance() const override;
	virtual float GetDuration() const override;
#if WITH_EDITOR
	virtual bool CanVisualizeAsset() const override { return false; }
#endif
	//~ End USoundBase Interface.

protected:

	void CacheValues();
	void ChooseSoundWave();

	// The current chosen Atom sound wave.
	UPROPERTY(Transient)
	TObjectPtr<UAtomSoundWave> SoundWave;
};
