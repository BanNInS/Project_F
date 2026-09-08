
#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFileManager.h"
#include "Logging/LogMacros.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationControlBusMix.h"
#include "Atom/Modulation/AtomModulationValue.h"

namespace AtomModulation
{
	class FProfileSerializer
	{
		static bool StageValueToFloat(const FConfigSection& InSection, const FName& InMember, float& OutValue)
		{
			if (const FConfigValue* Value = InSection.Find(InMember))
			{
				OutValue = FCString::Atof(*Value->GetValue());
				return true;
			}

			return false;
		}

		static FString GetConfigDir()
		{
			return FPaths::GeneratedConfigDir() + TEXT("AtomModulation");
		}

		static bool GetConfigDirectory(const FString& InDir, bool bCreateDirIfMissing)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (!bCreateDirIfMissing && PlatformFile.DirectoryExists(*InDir))
			{
				return false;
			}

			TArray<FString> DirArray;
			InDir.ParseIntoArray(DirArray, TEXT("/"));

			FString SubPath = GetConfigDir();

			if (!PlatformFile.CreateDirectory(*SubPath))
			{
				return false;
			}

			for (FString& Dir : DirArray)
			{
				SubPath /= Dir;
				if (!PlatformFile.CreateDirectory(*SubPath))
				{
					return false;
				}
			}

			return true;
		}

		static bool GetMixProfileConfigPath(const FString& Name, const FString& Path, const int32 InProfileIndex, bool bInCreateDirIfMissing, FString& OutMixProfileConfig)
		{
			FString ConfigDir = FPaths::GetPath(Path);
			FString ConfigPath = GetConfigDir() + ConfigDir / Name;

			ConfigPath += InProfileIndex > 0
				? FString::Printf(TEXT("%i.ini"), InProfileIndex)
				: FString(TEXT(".ini"));

			if (!GConfig)
			{
				UE_LOG(LogAtomModulation, Error,
					TEXT("Failed to find mix profile '%s' config file. Config cache not initialized."),
					*Path, *ConfigPath);
				return false;
			}

			if (!GetConfigDirectory(ConfigDir, bInCreateDirIfMissing))
			{
				UE_LOG(LogAtomModulation, Error, TEXT("Failed to %s mix profile '%s' config directory."),
					bInCreateDirIfMissing ? TEXT("create") : TEXT("find"),
					*Path,
					*ConfigDir);
				return false;
			}

			OutMixProfileConfig = ConfigPath;
			return true;
		}

	public:
		static bool Serialize(const UAtomModulationControlBusMix& InBusMix, const int32 InProfileIndex, const FString* InMixPathOverride = nullptr)
		{
			check(IsInGameThread());

			const FString Name = InMixPathOverride ? FPaths::GetBaseFilename(*InMixPathOverride) : InBusMix.GetName();
			const FString Path = InMixPathOverride ? *InMixPathOverride : InBusMix.GetPathName();

			FString ConfigPath;
			if (!GetMixProfileConfigPath(Name, Path, InProfileIndex, true /* bCreateDir */, ConfigPath))
			{
				return false;
			}

			// this will create and try to read in if not already loaded
			FConfigFile* ConfigFile = GConfig->Find(ConfigPath);
			if (!ConfigFile)
			{
				ConfigFile = &(GConfig->Add(ConfigPath, FConfigFile()));
			}

			ConfigFile->Reset();

			for (const FAtomModulationControlBusMixStage& Stage : InBusMix.MixStages)
			{
				if (Stage.Bus)
				{
					FString SectionName = Stage.Bus->GetPathName();
					ConfigFile->SetFloat(*SectionName, *GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, AttackTime).ToString(), Stage.Value.AttackTime);
					ConfigFile->SetFloat(*SectionName, *GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, ReleaseTime).ToString(), Stage.Value.ReleaseTime);
					ConfigFile->SetFloat(*SectionName, *GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, TargetValue).ToString(), Stage.Value.TargetValue);
				}
			}
			ConfigFile->Dirty = true;

			UE_LOG(LogAtomModulation, Display, TEXT("Serialized mix to '%s'"), *Path);
			GConfig->Flush(false, ConfigPath);

			return true;
		}

		static bool Deserialize(int32 InProfileIndex, UAtomModulationControlBusMix& InOutBusMix, const FString* InMixPathOverride = nullptr)
		{
			check(IsInGameThread());

			const FString Name = InMixPathOverride ? FPaths::GetBaseFilename(*InMixPathOverride) : InOutBusMix.GetName();
			const FString Path = InMixPathOverride ? *InMixPathOverride : InOutBusMix.GetPathName();

			FString ConfigPath;
			if (!GetMixProfileConfigPath(Name, Path, InProfileIndex, false /* bCreateDir */, ConfigPath))
			{
				return false;
			}

			FConfigFile* ConfigFile = GConfig->Find(ConfigPath);
			if (!ConfigFile)
			{
				UE_LOG(LogAtomModulation, Warning, TEXT("Config '%s' not found. Mix '%s' not loaded from config profile."), *ConfigPath , *Name);
				return false;
			}

			bool bMarkDirty = false;
			TSet<FString> SectionsProcessed;
			TArray<FAtomModulationControlBusMixStage>& Stages = InOutBusMix.MixStages;
			for (int32 i = Stages.Num() - 1; i >= 0; --i)
			{
				FAtomModulationControlBusMixStage& Stage = Stages[i];
				if (Stage.Bus)
				{
					FString PathName = Stage.Bus->GetPathName();
					if (const FConfigSection* ConfigSection = ConfigFile->FindSection(PathName))
					{
						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, AttackTime), Stage.Value.AttackTime);
						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, ReleaseTime), Stage.Value.ReleaseTime);
						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, TargetValue), Stage.Value.TargetValue);

#if WITH_EDITORONLY_DATA
						if (UAtomModulationParameter* Parameter = Stage.Bus->Parameter)
						{
							const float UnitValue = Parameter->ConvertNormalizedToUnit(Stage.Value.TargetValue);
							Stage.Value.TargetUnitValue = UnitValue;
						}
						else
						{
							Stage.Value.TargetUnitValue = Stage.Value.TargetValue;
						}
#endif // WITH_EDITORONLY_DATA

						SectionsProcessed.Emplace(MoveTemp(PathName));
						bMarkDirty = true;
					}
					else
					{
						Stages.RemoveAt(i);
					}
				}
				else
				{
					Stages.RemoveAt(i);
				}
			}

			TArray<FString> ConfigSectionNames;
			ConfigFile->GetKeys(ConfigSectionNames);
			for (FString& SectionName : ConfigSectionNames)
			{
				if (!SectionsProcessed.Contains(SectionName))
				{
					const FConfigSection* ConfigSection = ConfigFile->FindSection(SectionName);
					UObject* BusObj = FSoftObjectPath(SectionName).TryLoad();
					if (UAtomModulationControlBus* Bus = Cast<UAtomModulationControlBus>(BusObj))
					{
						FAtomModulationControlBusMixStage Stage;
						Stage.Bus = Bus;

						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, AttackTime),  Stage.Value.AttackTime);
						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, ReleaseTime), Stage.Value.ReleaseTime);
						StageValueToFloat(*ConfigSection, GET_MEMBER_NAME_CHECKED(FAtomModulationMixValue, TargetValue), Stage.Value.TargetValue);

						Stages.Emplace(MoveTemp(Stage));

						bMarkDirty = true;
					}
					else
					{
						UE_LOG(LogAtomModulation, Warning,
							TEXT("Bus missing or invalid type at '%s'. Profile '%s' stage not added/updated for mix '%s'"),
							*SectionName,
							*ConfigPath,
							*Path);
					}
				}
			}

			if (bMarkDirty)
			{
				InOutBusMix.MarkPackageDirty();
			}

			UE_LOG(LogAtomModulation, Display, TEXT("Deserialized mix from '%s'"), *ConfigPath);
			return true;
		}
	};
} // namespace
