
#include "AtomEnvelopeGraphNode.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "GraphEditorSettings.h"
#include "EditorCategoryUtils.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "KismetCompiler.h"
#include "ToolMenus.h"

#include "Atom/AtomEnvelope.h"

#define LOCTEXT_NAMESPACE "K2Node_Atom"

namespace
{
    static const FName AttackPinName(TEXT("Attack Time"));
    static const FName HoldPinName(TEXT("Hold Time"));
    static const FName DecayPinName(TEXT("Decay Time"));
    static const FName SustainPinName(TEXT("Sustain Level"));
    static const FName ReleasePinName(TEXT("Release Time"));

    static const FName AttackCurvePinName(TEXT("Attack Curve"));
    static const FName AttackCurveStrengthPinName(TEXT("Attack Curve Strength"));
    static const FName DecayCurvePinName(TEXT("Decay Curve"));
    static const FName DecayCurveStrengthPinName(TEXT("Decay Curve Strength"));
    static const FName ReleaseCurvePinName(TEXT("Release Curve"));
    static const FName ReleaseCurveStrengthPinName(TEXT("Release Curve Strength"));

    static const FName EnvelopePinName(TEXT("Atom Envelope"));
}

void UK2Node_MakeAtomEnvelope::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    Ar << bIsShapeVisible;
    Ar << ShapeColor;
}

void UK2Node_MakeAtomEnvelope::AllocateDefaultPins()
{
	UEnum* CurveEnum = StaticEnum<EAtomEnvelopeCurve>();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, AttackPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, HoldPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, DecayPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, SustainPinName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, ReleasePinName);

	FindPinChecked(SustainPinName)->DefaultValue = TEXT("1.0f");
	
	UEdGraphPin* AttackCurvePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, CurveEnum, AttackCurvePinName);
	UEdGraphPin* AttackCurveStrengthPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, AttackCurveStrengthPinName);
	UEdGraphPin* DecayCurvePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, CurveEnum, DecayCurvePinName);
	UEdGraphPin* DecayCurveStrengthPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, DecayCurveStrengthPinName);
	UEdGraphPin* ReleaseCurvePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Byte, CurveEnum, ReleaseCurvePinName);
	UEdGraphPin* ReleaseCurveStrengthPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Float, ReleaseCurveStrengthPinName);

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FAtomEnvelope::StaticStruct(), EnvelopePinName);

	AttackCurvePin->DefaultValue = TEXT("Linear");
	AttackCurveStrengthPin->DefaultValue = TEXT("1.0f");
	DecayCurvePin->DefaultValue = TEXT("Linear");
	DecayCurveStrengthPin->DefaultValue = TEXT("1.0f");
	ReleaseCurvePin->DefaultValue = TEXT("Linear");
	ReleaseCurveStrengthPin->DefaultValue = TEXT("1.0f");

	AttackCurvePin->bAdvancedView = true;
	AttackCurveStrengthPin->bAdvancedView = true;
	DecayCurvePin->bAdvancedView = true;
	DecayCurveStrengthPin->bAdvancedView = true;
	ReleaseCurvePin->bAdvancedView = true;
	ReleaseCurveStrengthPin->bAdvancedView = true;

	if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}
}

FText UK2Node_MakeAtomEnvelope::GetTooltipText() const
{
	// Returns text to be displayed in Tooltip.
	return LOCTEXT("MakeAtomEnvelope_Tooltip", "Adds a node that create an AtomEnvelope from its members and embedded with a shape editor.");
}

FLinearColor UK2Node_MakeAtomEnvelope::GetNodeTitleColor() const
{
	const UGraphEditorSettings* Settings = GetDefault<UGraphEditorSettings>();
	// The color of the title section is the same color as the struct type pin.
	return Settings->StructPinTypeColor;
}

FText UK2Node_MakeAtomEnvelope::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// Returns text to be displayed in the title section.
	return LOCTEXT("MakeAtomEnvelope", "Make AtomEnvelope (Shape Editor)");
}

FSlateIcon UK2Node_MakeAtomEnvelope::GetIconAndTint(FLinearColor& OutColor) const
{
	//OutColor = FColor(255, 127, 39);

	// Display the function icon in the title section.
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.MakeStruct_16x");
	return Icon;
}

bool UK2Node_MakeAtomEnvelope::IsNodePure() const
{
	return true;
}

void UK2Node_MakeAtomEnvelope::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_MakeAtomEnvelope::GetMenuCategory() const
{
	// Register the category "Atom".
	return LOCTEXT("Atom", "Utilities|Struct");
}

void UK2Node_MakeAtomEnvelope::GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	FToolMenuInsert InsertAfterNodeSection("EdGraphSchemaNodeActions", EToolMenuInsertType::After);
	FToolMenuSection& Section = Menu->AddSection("MakeAtomEnvelopeNode", LOCTEXT("MakeAtomEnvelopeNodeSection", "Atom Envelope"), InsertAfterNodeSection);

	const FUIAction ShowShapeAction = FUIAction(FExecuteAction::CreateLambda([this] { bIsShapeVisible = !bIsShapeVisible; }));

	// Add menu item "Hide/ShowEnvelopeShape".
	if (bIsShapeVisible)
	{
		Section.AddMenuEntry(
			"HideEnvelopeShape",
			LOCTEXT("HideEnvelopeShape", "Hide Envelope Shape"),
			LOCTEXT("HideEnvelopeShapeTooltip", "Hides the envelope shape graph."),
			FSlateIcon(),
			ShowShapeAction);
	}
	else
	{
		Section.AddMenuEntry(
			"ShowEnvelopeShape",
			LOCTEXT("ShowEnvelopeShape", "Show Envelope Shape"),
			LOCTEXT("ShowEnvelopeShapeTooltip", "Shows the envelope shape graph."),
			FSlateIcon(),
			ShowShapeAction);
	}
}

void UK2Node_MakeAtomEnvelope::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	// Spawn a "Make Struct" node to create the struct needed for editting the envelope.
	UK2Node_MakeStruct* MakeAtomEnvelopeStruct = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
	MakeAtomEnvelopeStruct->StructType = FAtomEnvelope::StaticStruct();
	MakeAtomEnvelopeStruct->AllocateDefaultPins();
	MakeAtomEnvelopeStruct->bMadeAfterOverridePinRemoval = true;
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeAtomEnvelopeStruct, this);

	UEdGraphPin** MakeStructOutPin = MakeAtomEnvelopeStruct->Pins.FindByPredicate([](UEdGraphPin* OtherPin) { return OtherPin->Direction == EGPD_Output; });
	check(MakeStructOutPin);

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(EnvelopePinName), *(*MakeStructOutPin));
	
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(AttackPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, AttackTime)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(HoldPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, HoldTime)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(DecayPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, DecayTime)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(SustainPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, SustainLevel)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(ReleasePinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, ReleaseTime)));

	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(AttackCurvePinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, AttackCurve)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(AttackCurveStrengthPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, AttackCurveStrength)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(DecayCurvePinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, DecayCurve)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(DecayCurveStrengthPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, DecayCurveStrength)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(ReleaseCurvePinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, ReleaseCurve)));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(ReleaseCurveStrengthPinName), *MakeAtomEnvelopeStruct->FindPinChecked(GET_MEMBER_NAME_CHECKED(FAtomEnvelope, ReleaseCurveStrength)));

	// This equals to the placement of "/" node.
	// UK2Node_CallFunction* CallDivideFunction = CompilerContext.SpawnIntermediateNode(this, SourceGraph);
	// CallDivideFunction->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Divide_IntInt)));
	// CallDivideFunction->AllocateDefaultPins();
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(QuotientPinName), *CallDivideFunction->GetReturnValuePin());
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(APinName), *CallDivideFunction->FindPinChecked(TEXT("A")));
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(BPinName), *CallDivideFunction->FindPinChecked(TEXT("B")));

	// This equals to the placement of "%" node.
	// UK2Node_CallFunction* CallPercentFunction = CompilerContext.SpawnIntermediateNode(this, SourceGraph);
	// CallPercentFunction->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Percent_IntInt)));
	// CallPercentFunction->AllocateDefaultPins();
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(RemainderPinName), *CallPercentFunction->GetReturnValuePin());
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(APinName), *CallPercentFunction->FindPinChecked(TEXT("A")));
	// CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(BPinName), *CallPercentFunction->FindPinChecked(TEXT("B")));
}

#undef LOCTEXT_NAMESPACE
