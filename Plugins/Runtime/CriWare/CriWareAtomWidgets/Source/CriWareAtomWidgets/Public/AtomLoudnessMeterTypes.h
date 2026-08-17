#pragma once

#include "AtomLoudnessMeterTypes.generated.h"

USTRUCT(BlueprintType)
struct FAtomLoudnessMeterInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLoudnessMeter)
	float MomentaryValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLoudnessMeter)
	float ShortTermValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLoudnessMeter)
	float IntegratedValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLoudnessMeter)
	FAtomLevelMeterChannelInfo TruePeakMeterChannelInfo;
};

inline bool operator==(const FAtomLoudnessMeterInfo& lhs, const FAtomLoudnessMeterInfo& rhs)
{
	return FMath::IsNearlyEqual(lhs.MomentaryValue, rhs.MomentaryValue) &&
		FMath::IsNearlyEqual(lhs.ShortTermValue, rhs.ShortTermValue) &&
		FMath::IsNearlyEqual(lhs.IntegratedValue, rhs.IntegratedValue) &&
		lhs.TruePeakMeterChannelInfo == rhs.TruePeakMeterChannelInfo;
}