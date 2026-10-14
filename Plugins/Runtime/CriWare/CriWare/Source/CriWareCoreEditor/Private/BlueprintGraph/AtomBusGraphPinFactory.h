
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraphUtilities.h"
#include "K2Node.h"
#include "EdGraphSchema_K2.h"

#include "Atom/AtomBus.h" 
#include "AtomBusGraphPinObject.h"
#include "SAtomGraphPinObject.h"

class FAtomBusGraphPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		/* Check if pin is struct, and then check if that pin is of struct type we want customize. */
		if ((InPin->PinType.PinCategory == K2Schema->PC_Object || InPin->PinType.PinCategory == K2Schema->PC_SoftObject)
			&& InPin->PinType.PinSubCategoryObject == UAtomBus::StaticClass())
		{
			//and return our customized pin widget ;).
			return SNew(SAtomGraphPinObject, InPin)
				.PropertyHolderObject(NewObject<UAtomBusGraphPinObject>())
				.DisplayThumbnail(false);
		}
		return nullptr;
	}
};
