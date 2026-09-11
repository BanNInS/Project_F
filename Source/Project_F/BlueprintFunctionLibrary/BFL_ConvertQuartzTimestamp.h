#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_ConvertQuartzTimestamp.generated.h"

UCLASS()
class PROJECT_F_API UBFL_ConvertQuartzTimestamp : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "QuartzUtility")
	static float ConvertQuartzTimestampToContinuousBeat(
		int InputNumBars,
		int InputBeat,
		float InputBeatFraction);

};
