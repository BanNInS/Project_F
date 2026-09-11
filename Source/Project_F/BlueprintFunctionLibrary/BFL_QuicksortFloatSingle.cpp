// Fill out your copyright notice in the Description page of Project Settings.


#include "BFL_QuicksortFloatSingle.h"
#include "Containers/Array.h"

TArray<float> UBFL_QuicksortFloatSingle::QuickSortFloatSingle(TArray<float> TargetArray)
{
	TargetArray.Sort();
	return TargetArray;
}

