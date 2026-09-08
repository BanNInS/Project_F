
#include "AtomEffectPresetAssetDefinition.h"

#include "AssetDefinitionRegistry.h"
#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "HAL/IConsoleManager.h"
#include "Settings/EditorLoadingSavingSettings.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/SimpleAssetEditor.h"

#include "CriWareWidgetSubsystem.h"
#include "Atom/AtomEffectPresetWidgetInterface.h"

#include "Toolkits/AtomEffectPresetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomEffectPresetActions"


void UAtomBusEffectPresetAssetDefinition::PostCDOContruct()
{
	Super::PostCDOContruct();

	// Register the audio editor asset type actions
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Look for any atom effect presets to register
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* ChildClass = *It;
		if (ChildClass->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		// Look for bus or source preset classes
		UClass* ParentClass = ChildClass->GetSuperClass();
		if (ParentClass && (ParentClass->IsChildOf(UAtomBusEffectPreset::StaticClass())) /*|| ParentClass->IsChildOf(UAtomEffectSourcePreset::StaticClass())*/)
		{
			UAtomEffectPreset* EffectPreset = ChildClass->GetDefaultObject<UAtomEffectPreset>();
			
			if (UAtomEffectPresetAssetDefinition* NewAssetDefinition = NewObject<UAtomEffectPresetAssetDefinition>())
			{
				NewAssetDefinition->Initialize(EffectPreset);
				UAssetDefinitionRegistry::Get()->RegisterAssetDefinition(NewAssetDefinition);
			}
		}
	}
}

bool UAtomEffectPresetAssetDefinition::CanRegisterStatically() const
{
	return Super::CanRegisterStatically() && EffectPreset.IsValid();
}

void UAtomEffectPresetAssetDefinition::Initialize(UAtomEffectPreset* InEffectPreset)
{
	EffectPreset = TStrongObjectPtr<UAtomEffectPreset>(InEffectPreset);
}

EAssetCommandResult UAtomEffectPresetAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	TArray<UAtomEffectPreset*> AssetPresets = OpenArgs.LoadObjects<UAtomEffectPreset>();
	TArray<UAtomEffectPreset*> CopyAssetPresets = AssetPresets;
	if (UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
	{
		if (UCriWareWidgetSubsystem* WidgetSubsystem = GEngine ? GEngine->GetEngineSubsystem<UCriWareWidgetSubsystem>() : nullptr)
		{
			for (UAtomEffectPreset* Preset : AssetPresets)
			{
				if (!Preset)
				{
					continue;
				}

				auto FilterFunction = [InPresetClass = Preset->GetClass()](UUserWidget* UserWidget)
				{
					TSubclassOf<UAtomEffectPreset> PresetClass = IAtomEffectPresetWidgetInterface::Execute_GetClass(UserWidget);
					while (PresetClass)
					{
						if (PresetClass == InPresetClass)
						{
							return true;
						}

						PresetClass = PresetClass->GetSuperClass();
					}

					return false;
				};

				TArray<UUserWidget*> UserWidgets = WidgetSubsystem->CreateUserWidgets(*World, UAtomEffectPresetWidgetInterface::StaticClass(), FilterFunction);
				if (!UserWidgets.IsEmpty())
				{
					TSharedRef<FAtomEffectPresetEditorToolkit> PresetEditor = MakeShared<FAtomEffectPresetEditorToolkit>();
					PresetEditor->Init(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Preset, UserWidgets);
					CopyAssetPresets.Remove(Preset);
				}
			}	
		}
	}

	if (!CopyAssetPresets.IsEmpty())
	{
		UAtomAssetDefinition_Base::OpenAssets(OpenArgs);
	}

	return EAssetCommandResult::Handled;
}

FText UAtomEffectPresetAssetDefinition::GetAssetDisplayName() const
{
	FText AssetActionName = EffectPreset->GetAssetActionName();
	if (AssetActionName.IsEmpty())
	{
		FString ClassName;
		EffectPreset->GetClass()->GetName(ClassName);
		ensureMsgf(false, TEXT("U%sGetAssetActionName not implemented. Please check that EFFECT_PRESET_METHODS(EffectClassName) is at the top of the declaration of %s."), *ClassName, *ClassName);
		FString DefaultName = ClassName + FString(TEXT(" (Error: EFFECT_PRESET_METHODS() Not Used in Class Declaration)"));
		return FText::FromString(DefaultName);
	}
	else
	{
		return EffectPreset->GetAssetActionName();
	}
}

TSoftClassPtr<UObject> UAtomEffectPresetAssetDefinition::GetAssetClass() const
{
	UClass* SupportedClass = EffectPreset->GetSupportedClass();
	if (SupportedClass == nullptr)
	{
		FString ClassName;
		EffectPreset->GetClass()->GetName(ClassName);
		ensureMsgf(false, TEXT("U%s::GetSupportedClass not implemented. Please check that EFFECT_PRESET_METHODS(EffectClassName) is at the top of the declaration of %s."), *ClassName, *ClassName);
		return EffectPreset->GetClass();
	}
	else
	{
		return SupportedClass;
	}
}

#undef LOCTEXT_NAMESPACE
