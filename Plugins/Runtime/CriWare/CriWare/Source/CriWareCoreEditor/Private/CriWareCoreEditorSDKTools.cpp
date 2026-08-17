// Copyright (c) 2025 CRI Middleware co., ltd. All Rights reserved.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformFileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/StringBuilder.h"

#include "CriWareCoreEditorPrivate.h"

struct FVerStr
{
	FString LibPath;
	FString LibName;
	int64   LibSize;

	inline bool operator<(const FVerStr& Other) const
	{
		if (LibPath != Other.LibPath)
		{
			return LibPath < Other.LibPath;
		}
		return LibName < Other.LibName;
	}
};

struct FVerStrItem
{
	FVerStrItem(const FString& VersionLine)
	{
		const int32 BuildPos = VersionLine.Find(TEXT("Build"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		const int32 VerPos = VersionLine.Find(TEXT("Ver"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, BuildPos);
		
		const FString Lib = VersionLine.Left(VerPos).TrimEnd();
		if (!Lib.Split(TEXT("/"), &Modulename, &Platform))
		{
			Modulename = Lib;
		}
		Version = VersionLine.Mid(VerPos, BuildPos - VerPos).TrimEnd();
		Date = VersionLine.Right(VersionLine.Len() - BuildPos).TrimEnd();
	}

	FString Modulename;
	FString Platform;
	FString Version;
	FString Date;
	FString Append;
};

// version checker
static int64 MemFind(const uint8* Mem, int64 MemSize, const uint8* Pattern, int64 PatternSize)
{
	for (int64 i = 0; i <= MemSize - PatternSize; ++i)
	{
		bool bFound = true;
		for (int64 j = 0; j < PatternSize; ++j)
		{
			if (Mem[i + j] != Pattern[j])
			{
				bFound = false;
				break;
			}
		}
		if (bFound)
		{
			return i;
		}
	}
	return -1;
}

static void GetCriWareLibVersionStr(const FString& LibPath, TArray<FVerStrItem>& OutVersions)
{
	if (IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*LibPath))
	{
		const int32 ChunkSize = 1024; // 1KB
		const int32 OverlapSize = 7;
		TArray<uint8> Block;
		Block.SetNumUninitialized(ChunkSize);

		int64 BytesRead = 0;
		int64 TotalFileSize = FileHandle->Size();
		bool bFound = false;

		TArray<uint8> OverlapBuffer;
		OverlapBuffer.SetNumUninitialized(0);

		while (BytesRead < TotalFileSize)
		{
			// Calculate the bytes to read for this iteration (in case the last chunk is smaller than ChunkSize)
			const int32 BytesToRead = FMath::Min((int64)ChunkSize, TotalFileSize - BytesRead);

			if (FileHandle->Read(Block.GetData(), BytesToRead))
			{
				// If the last chunk is smaller, resize the buffer appropriately for processing
				if (BytesToRead < ChunkSize)
				{
					Block.SetNum(BytesToRead);
				}

				TArray<uint8> SearchBlock;
				SearchBlock.Append(OverlapBuffer);
				SearchBlock.Append(Block);

				// Search for the version string pattern in the current block
				int64 FoundPos = 0;
				while (FoundPos >= 0)
				{
					bool bIsAppend = false;
					int64 CurFoundPos = MemFind(SearchBlock.GetData() + FoundPos, SearchBlock.Num() - FoundPos, (const uint8*)"Build:", 6);
					if (CurFoundPos < 0)
					{
						CurFoundPos = MemFind(SearchBlock.GetData() + FoundPos, SearchBlock.Num() - FoundPos, (const uint8*)"Append:", 7);
						if (CurFoundPos < 0)
						{
							break;
						}
						bIsAppend = true;
					}
					FoundPos += CurFoundPos;
					const int64 CurPos = FileHandle->Tell();
					const int64 LinePos = FMath::Max(CurPos - SearchBlock.Num() + FoundPos - 128, 0);

					FileHandle->Seek(LinePos);

					TArray<uint8> Line;
					Line.SetNumUninitialized(256);
					FileHandle->Read(Line.GetData(), 256);

					// check if it is a symbol from linux lib, discard it.
					if (Line[128 + 6] == '\\')
					{
						FileHandle->Seek(CurPos);
						FoundPos += 6; // Move past "Build:"
						break;
					}

					const ANSICHAR* LineStr = (ANSICHAR*)Line.GetData();
					int64 StartPos = 0;
					for (int i = 128; i >= 0; --i)
					{
						if (LineStr[i] == '\0' || LineStr[i] == '\n')
						{
							StartPos = i;
							break;
						}
					}

					FString VerStr = FString(ANSI_TO_TCHAR(LineStr + StartPos + 1)).TrimStartAndEnd();
					
					if (!bIsAppend)
					{
						if (VerStr.Len() > 6)
						{
							OutVersions.Add(VerStr);
						}
					}
					else
					{
						if (OutVersions.Num() > 0)
						{
							OutVersions.Last().Append = TEXT(" ") + VerStr.RightChop(8);
						}
					}

					FileHandle->Seek(CurPos);
					FoundPos += 6; // Move past "Build:"
				}

				OverlapBuffer.SetNumUninitialized(0);

				if (BytesToRead >= OverlapSize)
				{
					OverlapBuffer.Append(Block.GetData() + Block.Num() - OverlapSize, OverlapSize);
				}
				else
				{
					// If the chunk is smaller than the overlap needed, just store the whole chunk
					OverlapBuffer.Append(Block);
				}

				BytesRead += BytesToRead;
			}
			else
			{
				// Handle read error
				UE_LOG(LogCriWareCoreEditor, Error, TEXT("Error reading file chunk!"));
				break;
			}
		}

		delete FileHandle;
	}
	else
	{
		// Handle file open error
		UE_LOG(LogCriWareCoreEditor, Error, TEXT("Could not open file handle!"));
	}
}

static FString GetCriWareSDKLibPathForPlatform(const FString& PluginName, const FString& CriPlatformName)
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
	if (Plugin.IsValid())
	{
		FString PluginBaseDir = Plugin->GetBaseDir();
		FString Path = FPaths::Combine(PluginBaseDir, TEXT("Source/CriWare/SDK"), CriPlatformName, TEXT("libs"));

		if (IFileManager::Get().DirectoryExists(*Path))
		{
			return Path;
		}

		for (const FString& ExtensionDir : Plugin->GetExtensionBaseDirs())
		{
			Path = FPaths::Combine(ExtensionDir, TEXT("Source/CriWare/SDK"), CriPlatformName, TEXT("libs"));
			if (IFileManager::Get().DirectoryExists(*Path))
			{
				return Path;
			}
		}
	}
	return FString();
}

static const TCHAR* CriPlatfomLabels[][2] = {
	{ TEXT("Win64"), TEXT("pc") },
	{ TEXT("Mac"), TEXT("macosx") },
	{ TEXT("WinGDK"), TEXT("pc") },
	{ TEXT("XB1"), TEXT("xboxone_gdk") },
	{ TEXT("XSX"), TEXT("scarlett") },
};

static void GetCriWareSDKLibVersions(const FString& PluginName, const FString& PlatformName, TSortedMap<FVerStr, TArray<FVerStrItem>>& OutLibVersions)
{
	FString CriPlatfortmLabel = PlatformName;

	// Look if convertible cri platform label exists
	for (const auto& Pair : CriPlatfomLabels)
	{
		if (PlatformName.Equals(Pair[0], ESearchCase::IgnoreCase))
		{
			CriPlatfortmLabel = FString(Pair[1]);
		}
	}

	FString Path = GetCriWareSDKLibPathForPlatform(PluginName, CriPlatfortmLabel);

	// Get version for all cri lib files in Path directory
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *Path, TEXT("*.*"), true, false);
	for (const FString& File : Files)
	{
		FString FileName = FPaths::GetCleanFilename(File);
		if ((FileName.StartsWith(TEXT("cri_")) || FileName.StartsWith(TEXT("libcri_")) || FileName.StartsWith(TEXT("criafx_")) || FileName.StartsWith(TEXT("libcriafx_")))
			&& (FileName.EndsWith(TEXT(".lib")) || FileName.EndsWith(TEXT(".a")) || FileName.EndsWith(TEXT(".so")) || FileName.EndsWith(TEXT(".dylib")) || FileName.EndsWith(TEXT(".dll"))))
		{
			TArray<FVerStrItem> Versions;
			GetCriWareLibVersionStr(File, Versions);
			if (Versions.Num() > 0)
			{
				const int64 FileSize = IFileManager::Get().FileSize(*File);
				OutLibVersions.Add({ FPaths::GetPath(File), FileName, FileSize }, Versions);
			}
		}
	}
}

static void GetAllCriWareSDKLibVersions(const FString& PlatformName, TSortedMap<FVerStr, TArray<FVerStrItem>>& OutLibVersions)
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CriWare"));
	if (Plugin.IsValid())
	{
		FString PluginBaseDir = Plugin->GetBaseDir();

		TArray<FString> CriPluginPaths;
		IPluginManager::Get().FindPluginsUnderDirectory(FPaths::Combine(PluginBaseDir, TEXT("..")), CriPluginPaths);

		for (const FString& CriPluginPath : CriPluginPaths)
		{
			const FString CriPlugin = FPaths::GetBaseFilename(CriPluginPath);
			GetCriWareSDKLibVersions(CriPlugin, PlatformName, OutLibVersions);
		}
	}
}

static FString OutputLibVersionsAsDumpTable(const TSortedMap<FVerStr, TArray<FVerStrItem>>& LibVersions)
{
	int32 MaxLenModuleName = 10;
	int32 MaxLenPlatform = 6;
	int32 MaxLenVersion = 12;
	int32 MaxLenDate = 10;
	for (const auto& Pair : LibVersions)
	{
		for (const auto& Value : Pair.Value)
		{
			MaxLenModuleName = FMath::Max(MaxLenModuleName, Value.Modulename.Len());
			MaxLenPlatform = FMath::Max(MaxLenPlatform, Value.Platform.Len());
			MaxLenVersion = FMath::Max(MaxLenVersion, Value.Version.Len());
			MaxLenDate = FMath::Max(MaxLenDate, Value.Date.Len());
		}
	}

	FStringBuilderBase Builder;
	for (const auto& Pair : LibVersions)
	{
		Builder.Appendf(TEXT("[%s] %s bytes\n"), *Pair.Key.LibName, *FString::FormatAsNumber(Pair.Key.LibSize));
		for (const auto& Value : Pair.Value)
		{
			Builder.Appendf(TEXT("  %-*s  %-*s  %-*s  %-*s%s\n"),
				MaxLenModuleName, *Value.Modulename,
				MaxLenPlatform, *Value.Platform,
				MaxLenVersion, *Value.Version,
				MaxLenDate, *Value.Date,
				*Value.Append);	
		}
		Builder.Append(TEXT("\n"));
	}

	return Builder.ToString();
}

// Console Commands
static FAutoConsoleCommandWithArgsAndOutputDevice GListCriWareSDKLibVersionsCmd(
	TEXT("cri.ListSDKVersions"),
	TEXT("Dumps out a table containing SDK Libraries versions in selected CriWare plugin and platform.\n")
	TEXT("Usage: cri.ListSDKVersions <PluginName> <PlatformName...>"),
	FConsoleCommandWithArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, FOutputDevice& OutputDevice)
	{
		if (Args.Num() < 2)
		{
			OutputDevice.Logf(TEXT("Usage: cri.ListSDKVersions <PluginName> <PlatformName...>"));
			return;
		}

		const FString& PluginName = Args[0];
		
		TArray<FString> PlatformNames;
		for (int32 i = 1; i < Args.Num(); ++i)
		{
			PlatformNames.Add(Args[i]);
		}

		for (const FString& PlatformName : PlatformNames)
		{
			OutputDevice.Logf(TEXT("=== Plugin: %s, Platform: %s ==="), *PluginName, *PlatformName);
			TSortedMap<FVerStr, TArray<FVerStrItem>> VersionsList;
			GetCriWareSDKLibVersions(PluginName, PlatformName, VersionsList);
			if (VersionsList.Num() == 0)
			{
				OutputDevice.Logf(TEXT("No versions found for CriWare plugin '%s' with platform '%s'."), *PluginName, *PlatformName);
				continue;
			}
			FString FinalOutput = OutputLibVersionsAsDumpTable(VersionsList);
			OutputDevice.Log(FinalOutput);
		}
	}));

static FAutoConsoleCommandWithArgsAndOutputDevice GListAllPluginsCriWareSDKLibVersionsCmd(
	TEXT("cri.ListAllPluginsSDKVersions"),
	TEXT("Dumps out a table containing SDK Libraries versions in all CriWare plugin for selected platform.\n")
	TEXT("Usage: cri.ListSDKVersions <PlatformName...>"),
	FConsoleCommandWithArgsAndOutputDeviceDelegate::CreateLambda([](const TArray<FString>& Args, FOutputDevice& OutputDevice)
	{
		if (Args.Num() < 1)
		{
			OutputDevice.Logf(TEXT("Usage: cri.ListAllPluginsSDKVersions <PlatformName...>"));
			return;
		}

		TArray<FString> PlatformNames;
		for (const FString& Arg : Args)
		{
			PlatformNames.Add(Arg);
		}

		for (const FString& PlatformName : PlatformNames)
		{
			OutputDevice.Logf(TEXT("=== Platform: %s ==="), *PlatformName);
			TSortedMap<FVerStr, TArray<FVerStrItem>> VersionsList;
			GetAllCriWareSDKLibVersions(PlatformName, VersionsList);
			if (VersionsList.Num() == 0)
			{
				OutputDevice.Logf(TEXT("No versions found for any CriWare plugin with platform '%s'."), *PlatformName);
				continue;
			}
			FString FinalOutput = OutputLibVersionsAsDumpTable(VersionsList);
			OutputDevice.Log(FinalOutput);
		}
	}));
