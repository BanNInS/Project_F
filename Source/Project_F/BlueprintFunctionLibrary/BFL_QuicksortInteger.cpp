// Fill out your copyright notice in the Description page of Project Settings.


#include "BFL_QuicksortInteger.h"
#include "Containers/Array.h"

TArray<int32> UBFL_QuicksortInteger::QuickSortInteger(TArray<int32> TargetArray)
{
	TargetArray.Sort();
	return TargetArray;
}

