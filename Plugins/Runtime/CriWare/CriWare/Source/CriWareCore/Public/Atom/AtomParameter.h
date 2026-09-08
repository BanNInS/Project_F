#pragma once

#include "Containers/Array.h"
#include "UObject/Interface.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"

#include "AtomParameter.generated.h"

/*
 * Cue parameters (Selector ...)
 *****************************************************************************/

 /** Struct that define an Atom selector. */
USTRUCT(BlueprintType, meta = (ToolTip = "Describe an Atom selector with its labels."))
struct CRIWARECORE_API FAtomSelector
{
	GENERATED_BODY()

public:

	FAtomSelector() {}
	FAtomSelector(FName InName, const TArray<FName>& InLabels) : Name(InName), Labels(InLabels) {}

	FORCEINLINE FName GetName() const { return Name; }
	FORCEINLINE int GetNumLabels() const { return Labels.Num(); }
	FORCEINLINE FName GetLabel(int Index) const { return Labels.IsValidIndex(Index) ? Labels[Index] : NAME_None; }
	FORCEINLINE int FindLabel(FName Label) const { return Labels.Find(Label); }
	FORCEINLINE bool ContainLabel(FName Label) const { return Labels.Contains(Label); }

protected:

	/** Selector Name. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The selector name."))
	FName Name;

	/** Selector Labels */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atom", meta = (ToolTip = "The list of labels contained in this selector."))
	TArray<FName> Labels;
};

/** Struct that define an Atom selector label parameter. */
USTRUCT(BlueprintType, meta = (ToolTip = "Structure to setup a selector label as parameter."))
struct CRIWARECORE_API FAtomSelectorParam
{
	GENERATED_BODY()

public:

	FAtomSelectorParam();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selector)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Selector)
	FName Label;
};

