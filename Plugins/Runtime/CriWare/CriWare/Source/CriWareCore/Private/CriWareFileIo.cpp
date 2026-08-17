/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2015-2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : File I/O Interface
 * File     : CriWareFileIo.cpp
 *
 ****************************************************************************/

#include "CriWareFileIo.h"

#include <string.h>
#include <assert.h>
#include "CoreMinimal.h"
#include "HAL/FileManager.h"

#include "CriWareCorePrivate.h"
#include "Atom/AtomFileIo.h"

#define CRI_XPT_DISABLE_UNPREFIXED_TYPE
#if !defined(CRIWARE_UE_LE)	/* <cri_delete_if_LE> */
#include <cri_xpt.h>
#include <cri_file_system.h>
#else	/* </cri_delete_if_LE> */
#include <cri_le_xpt.h>
#include <cri_le_file_system.h>
#endif

 //~ File handle
struct FCriWareFile
{
	FArchive* FileHandle{ nullptr };
	int64 FileSize{ 0 };
	int64 ReadSize{ 0 };
	int64 WriteSize{ 0 };
};

//~ Functions for the interface registration
namespace CriWareFile
{
	CriFsIoError CRIAPI Exists(const CriChar8 *Filename, CriBool *OutResult);
	CriFsIoError CRIAPI Remove(const CriChar8 *Filename);
	CriFsIoError CRIAPI Open(const CriChar8 *Filename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn *OutFile);
	CriFsIoError CRIAPI Close(CriFsFileHn InFile);
	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64 *OutFileSize);
	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void *Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool *OutResult);
	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64 *OutReadSize);
	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void *Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool *OutResult);
	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64 *OutWriteSize);
	CriFsIoError CRIAPI GetNativeFileHandle(CriFsFileHn InFile, void **OutNativeFileHn);
	CriFsIoError CRIAPI CanParallelRead(CriBool* OutResult);
};

//~ I/O processing interface
static CriFsIoInterface GCriFileIoInterface = {
	CriWareFile::Exists,
	CriWareFile::Remove,
	nullptr,
	CriWareFile::Open,
	CriWareFile::Close,
	CriWareFile::GetFileSize,
	CriWareFile::Read,
	CriWareFile::IsReadComplete,
	nullptr,
	CriWareFile::GetReadSize,
	CriWareFile::Write,
	CriWareFile::IsWriteComplete,
	nullptr,
	CriWareFile::GetWriteSize,
	nullptr,
	nullptr,
	CriWareFile::GetNativeFileHandle,
	nullptr,
	CriWareFile::CanParallelRead
};

/* I/O interface selection function
 ***************************************************************************/
CriError CRIAPI CriWareFileIo::SelectIo(const CriChar8 *Filename, CriFsDeviceId *DeviceId, CriFsIoInterfacePtr *IoInterface)
{
	assert(Filename != nullptr);
	assert(DeviceId != nullptr);
	assert(IoInterface != nullptr);

    // The default device ID is specified
    // Note: One device is used in this sample
	*DeviceId = CRIFS_DEFAULT_DEVICE;

	// check if using IoStore for Atom
	FString FilenameStr = UTF8_TO_TCHAR(Filename);
	if (FilenameStr.Len() > 0)
	{
		if (FilenameStr[0] == TEXT('@'))
		{
			// GAtomCriFileIoToIoDispatcherInferface is used as I/O interface
			*IoInterface = &GAtomCriFileIoToIoDispatcherInferface;
		}
		else if (FilenameStr[0] == TEXT('%'))
		{
			// GAtomCriFileAsyncInferface is used as I/O interface
			*IoInterface = &GAtomCriFileAsyncInferface;
		}
		else
		{
			// GCriFileIoInterface is used as I/O interface
			*IoInterface = &GCriFileIoInterface;
		}

		return CRIERR_OK;
	}

	return CRIERR_INVALID_PARAMETER;
}

/* File Interface functions
 ***************************************************************************/

namespace CriWareFile
{
	// Check whether the file exists
	CriFsIoError CRIAPI Exists(const CriChar8* Filename, CriBool* OutResult)
	{
		assert(Filename != nullptr);
		assert(OutResult != nullptr);

		// Check whether the file exists
		*OutResult = IFileManager::Get().FileExists(UTF8_TO_TCHAR(Filename)) ? CRI_TRUE : CRI_FALSE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Remove(const CriChar8* Filename)
	{
		assert(Filename != nullptr);

		// Remove the file
		if (IFileManager::Get().Delete(UTF8_TO_TCHAR(Filename)))
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to DeleteFile %s."), UTF8_TO_TCHAR(Filename));
			return CRIFS_IO_ERROR_NG;
		}

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Open(const CriChar8* Filename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn* OutFile)
	{
		assert(Filename != nullptr);
		assert(OutFile != nullptr);

		*OutFile = nullptr;

		// Get file size
		// Note: When using a file server, this copy the file locally.
		const int64 FileSize = IFileManager::Get().FileSize(UTF8_TO_TCHAR(Filename));
		if (FileSize < 0)
		{
			UE_LOG(LogCriWareCore, Warning, TEXT("Could not find file %s."), UTF8_TO_TCHAR(Filename));
		}

		// Check if append mode
		bool bAppend = Mode == CRIFS_FILE_MODE_APPEND;

		// Check read availability
		bool bAllowRead = Access != CRIFS_FILE_ACCESS_WRITE;

		// Allocate a file handle area
		FCriWareFile* File = new FCriWareFile();
		if (File == nullptr)
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to allocate FCriWareFile %s."), UTF8_TO_TCHAR(Filename));
			return CRIFS_IO_ERROR_NG;
		}

		// Open a file
		if (Access == CRIFS_FILE_ACCESS_READ)
		{
			File->FileHandle = IFileManager::Get().CreateFileReader(UTF8_TO_TCHAR(Filename));
		}
		else
		{
			File->FileHandle = IFileManager::Get().CreateFileWriter(UTF8_TO_TCHAR(Filename));
		}

		if (File->FileHandle == nullptr)
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to Open %s."), UTF8_TO_TCHAR(Filename));
			delete File;
			return CRIFS_IO_ERROR_NG;
		}

		// Get the file size
		File->FileSize = File->FileHandle->TotalSize();

		// Store the file handle
		*OutFile = File;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Close(CriFsFileHn InFile)
	{
		assert(InFile != nullptr);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Close the file
		if (File->FileHandle != nullptr)
		{
			delete File->FileHandle;
			File->FileHandle = nullptr;
		}

		// Deallocate the handle area
		delete File;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64* OutFileSize)
	{
		assert(InFile != nullptr);
		assert(OutFileSize != nullptr);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Store the file size
		*OutFileSize = File->FileSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(ReadSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Get read size
		ReadSize = FMath::Min(ReadSize, BufferSize);
		ReadSize = FMath::Min(ReadSize, (CriSint64)File->FileSize - Offset);

		// Do nothing if no read required
		if (ReadSize <= 0)
		{
			File->ReadSize = 0;
			return CRIFS_IO_ERROR_OK;
		}

		// Move to a position for starting file read
		File->FileHandle->Seek(Offset);

		// Read data
		File->FileHandle->Serialize(Buffer, ReadSize);

		// check error
		if (File->FileHandle->GetError())
		{
			File->ReadSize = 0;
			return CRIFS_IO_ERROR_NG;
		}

		// Store the read size
		File->ReadSize = ReadSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		// Always return TRUE as it is implemented as synchronus I/O
		*OutResult = CRI_TRUE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64* OutReadSize)
	{
		assert(InFile != nullptr);
		assert(OutReadSize != nullptr);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Store the file size
		*OutReadSize = File->ReadSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(WriteSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		// Initialize variables
		FCriWareFile* File = (FCriWareFile*)InFile;

		// Get write size
		WriteSize = FMath::Min(WriteSize, BufferSize);

		// Do nothing if no write required
		if (WriteSize <= 0)
		{
			File->WriteSize = 0;
			return CRIFS_IO_ERROR_OK;
		}

		// Move to a position for starting file write
		File->FileHandle->Seek(Offset);

		// Write data
		File->FileHandle->Serialize(Buffer, WriteSize);

		// check error
		if (File->FileHandle->GetError())
		{
			File->WriteSize = 0;
			return CRIFS_IO_ERROR_NG;
		}

		// Store the write size
		File->WriteSize = WriteSize;

		// Correct file size
		int64 SeekPos = Offset + WriteSize;
		if (File->FileSize < SeekPos)
		{
			File->FileSize = SeekPos;
		}

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		// Always return TRUE as it is implemented as synchronus I/O
		*OutResult = CRI_TRUE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64* OutWriteSize)
	{
		assert(InFile != nullptr);
		assert(OutWriteSize != nullptr);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Store the file size
		*OutWriteSize = File->WriteSize;

		return CRIFS_IO_ERROR_OK;
	}

	//~ Get a native file handle
	CriFsIoError CRIAPI GetNativeFileHandle(CriFsFileHn InFile, void** OutNativeFileHn)
	{
		assert(InFile != nullptr);
		assert(OutNativeFileHn);

		FCriWareFile* File = (FCriWareFile*)InFile;

		// Store the file pointer
		*OutNativeFileHn = (void*)File->FileHandle;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI CanParallelRead(CriBool* OutResult)
	{
		*OutResult = CRI_FALSE; // default

		return CRIFS_IO_ERROR_OK;
	}
} // namespace
