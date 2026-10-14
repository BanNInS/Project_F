// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationGenerator.h"

#include "AtomModulationEnvelopeFollower.generated.h"

// Forward Declarations
class UAtomAudioBus;

USTRUCT(BlueprintType)
struct FAtomEnvelopeFollowerGeneratorParams
{
	GENERATED_BODY()

	/** If true, bypasses generator from being modulated by parameters, patches, or mixed (remains active and computed). */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite)
	bool bBypass = false;

	/** If true, inverts output */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (EditCondition = "!bBypass", DisplayAfter = "ReleaseTime"))
	bool bInvert = false;

	/** AudioBus to follow amplitude of and generate modulation control signal from. */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (EditCondition = "!bBypass"))
	TObjectPtr<UAtomAudioBus> AudioBus = nullptr;

	/** Gain to apply to amplitude signal. */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (EditCondition = "!bBypass", ClampMin = 0.0f))
	float Gain = 1.0f;

	/** Attack time of envelope response (in sec) */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (EditCondition = "!bBypass", ClampMin = 0.0f))
	float AttackTime = 0.010f;

	/** Release time of envelope response (in sec) */
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (EditCondition = "!bBypass", ClampMin = 0.0f))
	float ReleaseTime = 0.100f;
};

UCLASS(hidecategories = Object, BlueprintType, editinlinenew, meta = (DisplayName = "Envelope Follower Generator"))
class CRIWARECORE_API UAtomModulationGeneratorEnvelopeFollower
	: public UAtomModulationGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Modulation, BlueprintReadWrite, meta = (ShowOnlyInnerProperties))
	FAtomEnvelopeFollowerGeneratorParams Params;

	virtual AtomModulation::FGeneratorPtr CreateInstance() const override;
};
