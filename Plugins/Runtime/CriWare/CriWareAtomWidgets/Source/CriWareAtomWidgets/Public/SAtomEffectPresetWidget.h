#pragma once

#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/GCObject.h"

// Forward Declarations
class UAtomBusEffectPreset;

namespace AtomWidgets
{
	class SAtomRadialSlider;

	enum class EEffectPresetParameterType
	{
		Normalize = 0, // Range to [0,1]
		Linear, // Range to Range (As-is)
		Decibel, // Decibel Slow to Decibel/Linear
		Frequency, // Frequency Slow to Frequency/Linear
		Semitone, // Semitones to Linear
		Boolean, // Boolean to 0 or 1
		Integer, // Integer to Real
		Enum, // Enumeration to Index
		Hidden // Hidden
	};

	struct FEffectPresetParameterDesc
	{
	public:

		// parameter group name. Subsequents parameters will use same group. 
		FText Group = FText::GetEmpty();

		// The displayed name of the effect parameter.
		FName Name;
		
		// The type of parameter to use for convertions
		EEffectPresetParameterType Type = EEffectPresetParameterType::Normalize;

		// The unit to show after the value or the enumeration name list to use in the form "EnumName1|EnumName2|..." for Enum type.
		FString Unit = "";
		
		// The range of the slider and value to use.
		FVector2D Range = {0.0f, 1.0f};

		// curve Exponant of the slider for Linear type or Step for Interger type. 
		float Exponent = 1.0f;

		// Whether is the value need to be converted to linear space (for Decibel and Frequency types).
		bool bIsLinear = false;

		// The display scale of the widget to use.
		float Scale = 1.0f;	
	};

	struct FEffectPresetDesc
	{
		// Fullname of the effect.
		FName Name;
		
		// Array of effect parameter descriptors.
		TArray<FEffectPresetParameterDesc> ParameterDescs;
		
		// Maximum number of element to align in one widget line.
		int16 NumElementsPerLine;
	};

	class CRIWAREATOMWIDGETS_API SAtomEffectPresetWidget
		: public SCompoundWidget
		, public FGCObject
	{
	public:

		SLATE_BEGIN_ARGS(SAtomEffectPresetWidget)
			: _Preset(nullptr)
			{}

			SLATE_ARGUMENT(UAtomBusEffectPreset*, Preset)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void Update();

	private:

		//~ FGCObject Interface
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
		virtual FString GetReferencerName() const override
		{
			return TEXT("SAtomEffectPresetWidget");
		}

		TSharedRef<SWidget> GenerateComboBoxItem(TSharedPtr<FString> InOption);
		void OnSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type, int32 ParameterIndex);
		FText GetComboBoxContent(int32 ParameterIndex) const;
		void UpdateRadialSliderText(TSharedPtr<SAtomRadialSlider> AtomRadial, const FEffectPresetParameterDesc& ParameterDesc, float AtomValue);

		// the preset to use (referenced in FGCObject)
		TObjectPtr<UAtomBusEffectPreset> Preset;

		// the index of the preset in the descriptor list 
		int16 PresetIndex = INDEX_NONE;

		// widgets
		TAttribute<ECheckBoxState> bIsBypass;
		TSharedPtr<SCheckBox> BypassCheckBox;
		TArray<TSharedPtr<SWidget>> ParameterWidgets;
		TMap<int, const TArray<TSharedPtr<FString>>> ComboBoxSources;

	private:

		// Global list of all effect preset descriptors
		static TArray<FEffectPresetDesc> PresetDescs;
	};

} // namespace
