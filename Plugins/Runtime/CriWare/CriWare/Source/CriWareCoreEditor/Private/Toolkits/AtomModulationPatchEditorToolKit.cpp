
#include "Toolkits/AtomModulationPatchEditorToolkit.h"

#include "Templates/UniquePtr.h"
#include "WaveTableCurveEditorViewStacked.h"
#include "WaveTableSettings.h"
#include "WaveTableSampler.h"

#include "Atom/Modulation/AtomModulationPatch.h"

#include "Widgets/SAtomModulationPatchCurveEditorViewStacked.h"

#define LOCTEXT_NAMESPACE "ModulationPatchEditor"

TUniquePtr<WaveTable::Editor::FWaveTableCurveModel> FAtomModulationPatchEditorToolkit::ConstructCurveModel(FRichCurve& InRichCurve, UObject* InParentObject, EWaveTableCurveSource InSource)
{
	using namespace WaveTable::Editor;
	return TUniquePtr<FWaveTableCurveModel>(new FAtomModPatchCurveEditorModel(InRichCurve, GetEditingObject(), InSource));
}

bool FAtomModulationPatchEditorToolkit::GetIsPropertyEditorDisabled() const
{
	return GetIsBypassed();
}

bool FAtomModulationPatchEditorToolkit::GetIsBypassed() const
{
	if (UAtomModulationPatch* Patch = Cast<UAtomModulationPatch>(GetEditingObject()))
	{
		return Patch->PatchSettings.bBypass;
	}

	return false;
}

int32 FAtomModulationPatchEditorToolkit::GetNumTransforms() const
{
	if (UAtomModulationPatch* Patch = Cast<UAtomModulationPatch>(GetEditingObject()))
	{
		return Patch->PatchSettings.Inputs.Num();
	}

	return 0;
}

FWaveTableTransform* FAtomModulationPatchEditorToolkit::GetTransform(int32 InTransformIndex) const
{
	if (UAtomModulationPatch* Patch = Cast<UAtomModulationPatch>(GetEditingObject()))
	{
		FAtomControlModulationPatch& Settings = Patch->PatchSettings;
		if (Settings.Inputs.IsValidIndex(InTransformIndex))
		{
			return &Settings.Inputs[InTransformIndex].Transform;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE
