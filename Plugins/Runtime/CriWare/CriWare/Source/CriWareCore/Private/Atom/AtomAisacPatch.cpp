
#include "Atom/AtomAisacPatch.h"

#include "Atom/AtomConfig.h"

UAtomConfig* UAtomAisacPatch::GetConfig() const
{
	return Cast<UAtomConfig>(GetOuter());
}

void FAtomAisacParameter::Merge(TArray<FAtomAisacParameter>&& InParams, TArray<FAtomAisacParameter>& OutParams)
{
	if (InParams.IsEmpty())
	{
		return;
	}

	if (OutParams.IsEmpty())
	{
		OutParams = MoveTemp(InParams);
		return;
	}

	auto SortParamsPredicate = [](const FAtomAisacParameter& A, const FAtomAisacParameter& B) { return A.Control.ID < B.Control.ID; };
	OutParams.Sort(SortParamsPredicate);

	for (FAtomAisacParameter& NewParam : InParams)
	{
		const int32 ExistingElementIndex = Algo::LowerBound(OutParams, NewParam, SortParamsPredicate);
		if (OutParams.IsValidIndex(ExistingElementIndex))
		{
			FAtomAisacParameter& ExistingParam = OutParams[ExistingElementIndex];
			if (ExistingParam.Control == NewParam.Control)
			{
				ExistingParam.Value = NewParam.Value;
				continue;
			}
		}

		OutParams.Emplace(MoveTemp(NewParam));
	}
}