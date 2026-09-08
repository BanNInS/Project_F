#include "BFL_QuicksortFloat.h"
#include "Containers/Array.h"

void UBFL_QuicksortFloat::QuickSortFloat(
	const TArray<float>& ParentArray,
	const TArray<int32>& ChildArray,
	TArray<float>& SortedParentArray,
	TArray<int32>& SortedChildArray)
{
	// 出力を初期化(呼び出し側の使い回しに備えて空にしておく)
	SortedParentArray.Reset();
	SortedChildArray.Reset();

	if (ParentArray.Num() != ChildArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("QuickSortFloat: ParentArray and ChildArray must be the same length."));
		return;
	}

	// インデックス配列を作成
	TArray<int32> Indices;
	Indices.SetNum(ParentArray.Num());
	for (int32 i = 0; i < ParentArray.Num(); ++i)
	{
		Indices[i] = i;
	}

	// ParentArrayの値を基準にインデックスをソート
	Indices.Sort([&ParentArray](int32 L, int32 R)
		{
			return ParentArray[L] < ParentArray[R];
		});

	// ソート結果を出力用配列に格納
	SortedParentArray.SetNum(ParentArray.Num());
	SortedChildArray.SetNum(ChildArray.Num());
	for (int32 i = 0; i < Indices.Num(); ++i)
	{
		SortedParentArray[i] = ParentArray[Indices[i]];
		SortedChildArray[i] = ChildArray[Indices[i]];
	}
}