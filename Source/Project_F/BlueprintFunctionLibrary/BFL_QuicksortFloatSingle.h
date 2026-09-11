#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_QuicksortFloatSingle.generated.h"

UCLASS()
class PROJECT_F_API UBFL_QuicksortFloatSingle : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ArraySort")
	static TArray<float> QuickSortFloatSingle(TArray<float> TargetArray);
};
