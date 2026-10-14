
#pragma once

#include "Curves/RichCurve.h"
#include "RichCurveEditorModel.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "WaveTableCurveEditorViewStacked.h"

#include "AtomDefaults.h"

// Forward Declarations
class UCurveFloat;
class UAtomModulationParameter;
class UAtomModulationPatch;
struct FAtomControlModulationInput;

#define LOCTEXT_NAMESPACE "AtomModulation"

class SAtomModulationPatchEditorViewStacked : public WaveTable::Editor::SViewStacked
{
	virtual void FormatInputLabel(const WaveTable::Editor::FWaveTableCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat, FText& InOutLabel) const override;
	virtual void FormatOutputLabel(const WaveTable::Editor::FWaveTableCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat, FText& InOutLabel) const override;

protected:
	virtual FText FormatToolTipValue(const FCurveModel& CurveModel, double EvaluatedValue) const override;
	
	static constexpr int MaximumFractionalDigits = 3;
};

class FAtomModPatchCurveEditorModel : public WaveTable::Editor::FWaveTableCurveModel
{
public:
	static ECurveEditorViewID ModPatchViewId;

	FAtomModPatchCurveEditorModel(FRichCurve& InRichCurve, UObject* InOwner, EWaveTableCurveSource InSource);

	virtual void RefreshCurveDescriptorText(const FWaveTableTransform& InTransform, FText& OutShortDisplayName, FText& OutInputAxisName, FText& OutOutputAxisName) override;

	virtual ECurveEditorViewID GetViewId() const override { return ModPatchViewId; }
	virtual FColor GetCurveColor() const override { return Atom::DefaultModulationControlBusColor; }
	virtual bool GetPropertyEditorDisabled() const override { return GetIsBypassed(); }
	virtual FText GetPropertyEditorDisabledText() const override { return LOCTEXT("ModulationPatchCurveEditorView_Bypassed", "Bypassed"); }

	bool GetIsBypassed() const;

	UAtomModulationPatch* GetPatch();
	const UAtomModulationPatch* GetPatch() const;
};

#undef LOCTEXT_NAMESPACE
