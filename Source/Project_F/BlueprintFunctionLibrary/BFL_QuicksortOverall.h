#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_QuicksortOverall.generated.h"

UCLASS()
class PROJECT_F_API UBFL_QuicksortOverall : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "ArraySort")
	static void QuickSortOverall(
		const TArray<float>& ParentArray,
		const TArray<int32>& ChildArray,
		TArray<float>& SortedParentArray,
		TArray<int32>& SortedChildArray);
};