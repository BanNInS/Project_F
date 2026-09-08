#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_QuicksortFloat.generated.h"

UCLASS()
class PROJECT_F_API UBFL_QuicksortFloat : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ArraySort")
	static void QuickSortFloat(
		const TArray<float>& ParentArray,
		const TArray<int32>& ChildArray,
		TArray<float>& SortedParentArray,
		TArray<int32>& SortedChildArray);
};