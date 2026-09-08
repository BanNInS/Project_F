
#pragma once

#include "AtomLevelMeterTypes.generated.h"

USTRUCT(BlueprintType)
struct FAtomLevelMeterChannelInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLevelMeter)
	float MeterValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLevelMeter)
	float PeakValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomLevelMeter)
	float ClippingValue = 0.0f;
};

inline bool operator==(const FAtomLevelMeterChannelInfo& lhs, const FAtomLevelMeterChannelInfo& rhs)
{
	return FMath::IsNearlyEqual(lhs.MeterValue, rhs.MeterValue) &&
		FMath::IsNearlyEqual(lhs.PeakValue, rhs.PeakValue) &&
		FMath::IsNearlyEqual(lhs.ClippingValue, rhs.ClippingValue);
}
