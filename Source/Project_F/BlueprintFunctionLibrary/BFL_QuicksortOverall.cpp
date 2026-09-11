#include "BFL_QuicksortOverall.h"
#include "Containers/Array.h"

void UBFL_QuicksortOverall::QuickSortOverall(
	const TArray<float>& ParentArray,
	const TArray<int32>& ChildArray,
	TArray<float>& SortedParentArray,
	TArray<int32>& SortedChildArray)
{
	SortedParentArray.Reset();
	SortedChildArray.Reset();

	if (ParentArray.Num() != ChildArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSortOverall: ParentArray and ChildArray must be the same length."));
		return;
	}

	TArray<int32> Indices;
	Indices.SetNum(ParentArray.Num());
	for (int32 i = 0; i < ParentArray.Num(); ++i)
	{
		Indices[i] = i;
	}

	Indices.Sort([&ParentArray](int32 L, int32 R)
		{
			return ParentArray[L] < ParentArray[R];
		});

	SortedParentArray.SetNum(ParentArray.Num());
	SortedChildArray.SetNum(ChildArray.Num());
	for (int32 i = 0; i < Indices.Num(); ++i)
	{
		SortedParentArray[i] = ParentArray[Indices[i]];
		SortedChildArray[i] = ChildArray[Indices[i]];
	}
}