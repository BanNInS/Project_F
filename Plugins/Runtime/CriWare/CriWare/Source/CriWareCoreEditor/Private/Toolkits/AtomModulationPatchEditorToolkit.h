
#pragma once

#include "WaveTableBankEditor.h"

class FAtomModulationPatchEditorToolkit : public WaveTable::Editor::FBankEditorBase
{
public:
	FAtomModulationPatchEditorToolkit() = default;
	virtual ~FAtomModulationPatchEditorToolkit() = default;

protected:
	virtual bool GetIsBypassed() const;
	virtual bool GetIsPropertyEditorDisabled() const override;
	virtual FWaveTableTransform* GetTransform(int32 InIndex) const override;
	virtual int32 GetNumTransforms() const override;

	virtual TUniquePtr<WaveTable::Editor::FWaveTableCurveModel> ConstructCurveModel(FRichCurve& InRichCurve, UObject* InParentObject, EWaveTableCurveSource InSource) override;
};
