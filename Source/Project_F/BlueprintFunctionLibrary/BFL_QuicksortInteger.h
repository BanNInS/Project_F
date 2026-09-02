// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BFL_QuicksortInteger.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_F_API UBFL_QuicksortInteger : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ArraySort")
	static TArray<int32> QuickSortInteger(TArray<int32> TargetArray);
	
};
