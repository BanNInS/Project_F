
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraphUtilities.h"
#include "K2Node.h"
#include "SGraphPin.h"

// Forward Defintions
class UAtomGraphPinObject;
class SVerticalBox;

class SAtomGraphPinObject : public SGraphPin, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SAtomGraphPinObject) : _PropertyHolderObject(nullptr), _DisplayThumbnail(true) {}
	SLATE_ARGUMENT(UAtomGraphPinObject*, PropertyHolderObject)
	SLATE_ARGUMENT(bool, DisplayThumbnail)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("SAtomGraphPinObject");
	}
	//~ End FGCObject Interface

private:

	bool OnSelectionChanged(UObject* SoundObject);
	bool SetValue(const FAssetData& Sound);
	FAssetData GetValue() const;

	const FAssetData& GetAssetData() const;
	mutable FAssetData CachedAssetData;

	UObject* GetPropertyHolderObject();
	TObjectPtr<UAtomGraphPinObject> PropertyHolderObject = nullptr;
	bool DisplayThumbnail = true;

	TSharedPtr<SVerticalBox> Box;
};
