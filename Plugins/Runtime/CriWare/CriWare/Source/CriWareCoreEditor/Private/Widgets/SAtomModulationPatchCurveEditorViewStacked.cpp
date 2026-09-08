
#include "SAtomModulationPatchCurveEditorViewStacked.h"

#include "Curves/CurveFloat.h"
#include "CurveEditor.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"

#include "Atom/Modulation/AtomModulationDestination.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationParameter.h"
#include "Atom/Modulation/AtomModulationPatch.h"

#define LOCTEXT_NAMESPACE "AtomModulationPatchEditor"

namespace AtomPatchCurveViewUtils
{
	static const FText NormalizedAxisName = LOCTEXT("ModulationCurveDisplayTitle_Normalized", "Normalized");

	void FormatLabel(const UAtomModulationParameter& InParameter, const FNumberFormattingOptions& InNumFormatOptions, FText& InOutLabel)
	{
		const float NormalizedValue = FCString::Atof(*InOutLabel.ToString());
		const float UnitValue = InParameter.ConvertNormalizedToUnit(NormalizedValue);
		FText UnitLabel = FText::AsNumber(UnitValue, &InNumFormatOptions);
		InOutLabel = FText::Format(LOCTEXT("ModulationPatchCurveView_UnitFormat", "{0} ({1})"), UnitLabel, InOutLabel);
	}
} // namespace

void SAtomModulationPatchEditorViewStacked::FormatInputLabel(const WaveTable::Editor::FWaveTableCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat, FText& InOutLabel) const
{
	if (const UAtomModulationPatch* Patch = static_cast<const FAtomModPatchCurveEditorModel&>(EditorModel).GetPatch())
	{
		const int32 Index = EditorModel.GetCurveIndex();
		const FAtomControlModulationInput& Input = Patch->PatchSettings.Inputs[Index];
		if (const UAtomModulationControlBus* Bus = Input.Bus)
		{
			if (const UAtomModulationParameter* Parameter = Bus->Parameter)
			{
				AtomPatchCurveViewUtils::FormatLabel(*Parameter, InLabelFormat, InOutLabel);
			}
		}
	}
}

void SAtomModulationPatchEditorViewStacked::FormatOutputLabel(const WaveTable::Editor::FWaveTableCurveModel& EditorModel, const FNumberFormattingOptions& InLabelFormat, FText& InOutLabel) const
{
	if (const UAtomModulationPatch* Patch = static_cast<const FAtomModPatchCurveEditorModel&>(EditorModel).GetPatch())
	{
		if (const UAtomModulationParameter* Parameter = Patch->PatchSettings.OutputParameter)
		{
			AtomPatchCurveViewUtils::FormatLabel(*Parameter, InLabelFormat, InOutLabel);
		}
	}
}

FText SAtomModulationPatchEditorViewStacked::FormatToolTipValue(const FCurveModel& CurveModel, double EvaluatedValue) const
{
	FNumberFormattingOptions FormatOptions;
	FormatOptions.MaximumFractionalDigits = MaximumFractionalDigits;
	const FText NormalizedValue = FText::AsNumber(EvaluatedValue, &FormatOptions);
	const FAtomModPatchCurveEditorModel& PatchCurveModel = static_cast<const FAtomModPatchCurveEditorModel&>(CurveModel);
	const UAtomModulationPatch* Patch = PatchCurveModel.GetPatch();
	FText UnitValue;
	FText UnitLabel;
	if (const TObjectPtr<UAtomModulationParameter> Parameter = Patch->PatchSettings.OutputParameter)
	{
		const float Value = Parameter->ConvertNormalizedToUnit(EvaluatedValue);
		// Set the fractional digits based on the difference between the minimum and maximum value of the parameter.
		// Only add fractional digits so that the total number of digits is at least 4
		const float MinMaxDiff = Parameter->GetUnitMax() - Parameter->GetUnitMin();
		const int DiffDigits = FMath::Floor(FMath::LogX(10, FMath::Abs(MinMaxDiff)));
		FormatOptions.MaximumFractionalDigits = FMath::Clamp(MaximumFractionalDigits - DiffDigits, 0, MaximumFractionalDigits);
		UnitValue = FText::AsNumber(Value, &FormatOptions);
		UnitLabel = Parameter->Settings.UnitDisplayName;
	}
	return FText::Format(LOCTEXT("ModulationPatch_PointToolTipValue", "Value:  {0}{1} ({2})"), UnitValue, UnitLabel, NormalizedValue);
}

ECurveEditorViewID FAtomModPatchCurveEditorModel::ModPatchViewId = ECurveEditorViewID::Invalid;

FAtomModPatchCurveEditorModel::FAtomModPatchCurveEditorModel(FRichCurve& InRichCurve, UObject* InOwner, EWaveTableCurveSource InSource)
	: WaveTable::Editor::FWaveTableCurveModel(InRichCurve, InOwner, InSource)
{
}

bool FAtomModPatchCurveEditorModel::GetIsBypassed() const
{
	if (const UAtomModulationPatch* Patch = GetPatch())
	{
		return Patch->PatchSettings.bBypass;
	}

	return true;
}

UAtomModulationPatch* FAtomModPatchCurveEditorModel::GetPatch()
{
	if (ParentObject.IsValid())
	{
		return CastChecked<UAtomModulationPatch>(ParentObject);
	}

	return nullptr;
}

const UAtomModulationPatch* FAtomModPatchCurveEditorModel::GetPatch() const
{
	if (ParentObject.IsValid())
	{
		return CastChecked<const UAtomModulationPatch>(ParentObject);
	}

	return nullptr;
}

void FAtomModPatchCurveEditorModel::RefreshCurveDescriptorText(const FWaveTableTransform& InTransform, FText& OutShortDisplayName, FText& OutInputAxisName, FText& OutOutputAxisName)
{
	OutShortDisplayName = LOCTEXT("ModulationCurveDisplayTitle_BusUnset", "Bus (Unset)");
	OutInputAxisName = AtomPatchCurveViewUtils::NormalizedAxisName;
	OutOutputAxisName = AtomPatchCurveViewUtils::NormalizedAxisName;

	if (const UAtomModulationPatch* Patch = GetPatch())
	{
		const int32 Index = GetCurveIndex();

		const FAtomControlModulationInput& Input = Patch->PatchSettings.Inputs[Index];
		if (const UAtomModulationControlBus* Bus = Input.GetBus())
		{
			OutShortDisplayName = FText::FromString(Bus->GetName());

			if (const UAtomModulationParameter* Parameter = Bus->Parameter)
			{
				static const FText AxisNameFormat = LOCTEXT("ModulationCurveDisplayTitle_InputAxisNameFormat", "{0} ({1})");
				OutInputAxisName = FText::Format(AxisNameFormat, FText::FromString(Parameter->GetName()), Parameter->Settings.UnitDisplayName);
			}
		}

		if (UAtomModulationParameter* Parameter = Patch->PatchSettings.OutputParameter)
		{
			static const FText AxisNameFormat = LOCTEXT("ModulationCurveDisplayTitle_OutputAxisNameFormat", "{0} ({1})");
			OutOutputAxisName = FText::Format(AxisNameFormat, FText::FromString(Parameter->GetName()), Parameter->Settings.UnitDisplayName);
		}
	}
}
#undef LOCTEXT_NAMESPACE
