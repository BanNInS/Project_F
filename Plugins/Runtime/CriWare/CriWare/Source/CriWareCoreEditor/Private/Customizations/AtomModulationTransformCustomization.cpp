
#include "AtomModulationTransformCustomization.h"

#include "Containers/Array.h"
#include "UObject/NoExportTypes.h"

#include "Atom/Modulation/AtomModulationPatch.h"

TSet<EWaveTableCurve> FAtomModulationTransformCustomization::GetSupportedCurves() const
{
	TSet<EWaveTableCurve> Curves = WaveTable::Editor::FTransformLayoutCustomizationBase::GetSupportedCurves();
	Curves.Remove(EWaveTableCurve::File);
	return Curves;
}

FWaveTableTransform* FAtomModulationTransformCustomization::GetTransform() const
{
	if (ensure(WaveTableOptionsHandle.IsValid()))
	{
		TArray<UObject*> OuterObjects;
		WaveTableOptionsHandle->GetOuterObjects(OuterObjects);
		if (OuterObjects.Num() == 1)
		{
			if (UAtomModulationPatch* Patch = Cast<UAtomModulationPatch>(OuterObjects.Last()))
			{
				const int32 InputIndex = GetOwningArrayIndex();
				if (InputIndex != INDEX_NONE)
				{
					FAtomControlModulationPatch& PatchSettings = Patch->PatchSettings;
					if (InputIndex < PatchSettings.Inputs.Num())
					{
						return &PatchSettings.Inputs[InputIndex].Transform;
					}
				}
			}
		}
	}

	return nullptr;
}

bool FAtomModulationTransformCustomization::IsBipolar() const
{
	return false;
}
