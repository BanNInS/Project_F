/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomFileIo.cpp
 *
 ****************************************************************************/

#include "Atom/AtomFileIo.h"

#include "CoreMinimal.h"
#include "IO/IoDispatcher.h"

#include <assert.h>

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomSoundBank.h"

//~ chunk handle (IoStore)
struct FCriWareChunk
{
	FIoChunkId ChunkID;
	FIoRequest Request;
	int64 FileSize = 0;
	int64 ReadSize = 0;
	int64 WriteSize = 0;
};

//~ Functions for the interface registration
namespace CriWareAtomFileIoDispatcher
{
	CriFsIoError CRIAPI Exists(const CriChar8* InFilename, CriBool* OutResult);
	CriFsIoError CRIAPI Remove(const CriChar8* InFilename);
	CriFsIoError CRIAPI Open(const CriChar8* InFilename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn* OutFile);
	CriFsIoError CRIAPI Close(CriFsFileHn InFile);
	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64* OutFileSize);
	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void* Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool* OutResult);
	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64* OutReadSize);
	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void* Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool* OutResult);
	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64* OutWriteSize);
	CriFsIoError CRIAPI CanParallelRead(CriBool* result);
};

//~ I/O processing interface
CriFsIoInterface GAtomCriFileIoToIoDispatcherInferface = {
	CriWareAtomFileIoDispatcher::Exists,
	CriWareAtomFileIoDispatcher::Remove,
	nullptr,
	CriWareAtomFileIoDispatcher::Open,
	CriWareAtomFileIoDispatcher::Close,
	CriWareAtomFileIoDispatcher::GetFileSize,
	CriWareAtomFileIoDispatcher::Read,
	CriWareAtomFileIoDispatcher::IsReadComplete,
	nullptr,
	CriWareAtomFileIoDispatcher::GetReadSize,
	CriWareAtomFileIoDispatcher::Write,
	CriWareAtomFileIoDispatcher::IsWriteComplete,
	nullptr,
	CriWareAtomFileIoDispatcher::GetWriteSize,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	CriWareAtomFileIoDispatcher::CanParallelRead
};

/* FileIo Interface functions
 ***************************************************************************/

namespace CriWareAtomFileIoDispatcher
{

	static bool CheckUsingIODispatcher(const FString& Filename)
	{
		return !Filename.IsEmpty() && Filename[0] == TEXT('@');
	}

	static FIoChunkId GetIoChunkId(const FString& Filename)
	{
		if (FAtomResourceManager* ResourceManager = GCriWare->GetAtomResourceManager())
		{
			uint32 ResourceID = FCString::Atoi(*Filename.RightChop(1));

			// TODO: USE A PROXY !!
			FAtomResourceHandle ResourceHandle = ResourceManager->GetResource(ResourceID);
			if (ResourceHandle.IsValid())
			{
				if (UAtomSoundBank* Source = Cast<UAtomSoundBank>(ResourceHandle.GetResource()->GetSoundResource()))
				{
					if (FByteBulkData* RawData = Source->GetRawData())
					{
						return RawData->CreateChunkId();
					}
				}
			}
		}

		return FIoChunkId::InvalidChunkId;
	}

	// Check whether the file exists
	CriFsIoError CRIAPI Exists(const CriChar8* InFilename, CriBool* OutResult)
	{
		assert(InFilename != nullptr);
		assert(OutResult != nullptr);

		const FString Filename(UTF8_TO_TCHAR(InFilename));

		if (CheckUsingIODispatcher(Filename))
		{
			// try get resource
			FIoChunkId ChunkID = GetIoChunkId(Filename);
			*OutResult = FIoDispatcher::Get().DoesChunkExist(ChunkID) ? CRI_TRUE : CRI_FALSE;
		}
		else
		{
			UE_LOG(LogCriWareCore, Error, TEXT("File %s do not use IoStore."), *Filename);
			return CRIFS_IO_ERROR_NG;
		}

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Remove(const CriChar8* InFilename)
	{
		assert(InFilename != nullptr);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot delete file %s from IoStore."), UTF8_TO_TCHAR(InFilename));
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI Open(const CriChar8* InFilename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn* OutFile)
	{
		assert(InFilename != nullptr);
		assert(OutFile != nullptr);

		*OutFile = nullptr;
		const FString Filename(UTF8_TO_TCHAR(InFilename));

		if (CheckUsingIODispatcher(Filename))
		{
			// try get resource
			FIoChunkId ChunkID = GetIoChunkId(Filename);
			if (!FIoDispatcher::Get().DoesChunkExist(ChunkID))
			{
				UE_LOG(LogCriWareCore, Warning, TEXT("File %s is not valid."), *Filename);
				return CRIFS_IO_ERROR_NG;
			}

			// Get file size
			TIoStatusOr<uint64> FileSizeStatus = FIoDispatcher::Get().GetSizeForChunk(ChunkID);
			if (!FileSizeStatus.IsOk())
			{
				UE_LOG(LogCriWareCore, Warning, TEXT("Could not find file %s in IoStore."), *Filename);
				return CRIFS_IO_ERROR_NG;
			}

			// Allocate a file handle area
			FCriWareChunk* Chunk = new FCriWareChunk();
			Chunk->ChunkID = ChunkID;
			Chunk->FileSize = (int64)FileSizeStatus.ValueOrDie();

			// Store the file chunk
			*OutFile = Chunk;

			return CRIFS_IO_ERROR_OK;
		}

		UE_LOG(LogCriWareCore, Error, TEXT("File %s do not use IoStore."), *Filename);
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI Close(CriFsFileHn InFile)
	{
		assert(InFile != nullptr);

		FCriWareChunk* Chunk = (FCriWareChunk*)InFile;

		// Deallocate the handle area
		delete Chunk;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64* OutFileSize)
	{
		assert(InFile != nullptr);
		assert(OutFileSize != nullptr);

		FCriWareChunk* Chunk = (FCriWareChunk*)InFile;

		// Store the file size
		*OutFileSize = Chunk->FileSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(ReadSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		FCriWareChunk* Chunk = (FCriWareChunk*)InFile;

		// Get read size
		ReadSize = FMath::Min(ReadSize, BufferSize);
		ReadSize = FMath::Min(ReadSize, (CriSint64)Chunk->FileSize - Offset);

		// Do nothing if no write required
		if (ReadSize <= 0)
		{
			Chunk->ReadSize = 0;
			return CRIFS_IO_ERROR_OK;
		}

		FIoReadOptions Options;
		Options.SetRange((uint64)Offset, (uint64)ReadSize);
		Options.SetTargetVa(Buffer);

		// request read
		FIoBatch IoBatch = FIoDispatcher::Get().NewBatch();
		Chunk->Request = IoBatch.Read(Chunk->ChunkID, Options, IoDispatcherPriority_Max);

		// issue request (async)
		IoBatch.Issue();

		/* 
		// issue request and wait for completion (sync)
		FEvent* Event = FPlatformProcess::GetSynchEventFromPool(); // obtain an event from the pool
		IoBatch.IssueAndTriggerEvent(Event);
		Event->Wait(); // sync
		FPlatformProcess::ReturnSynchEventToPool(Event); // return event to the pool

		// check error
		if (const FIoBuffer* IoBuffer = Chunk->Request.GetResult())
		{
			// get read size
			const uint8* CheckedBuffer = IoBuffer->Data(); // ptr
			Chunk->ReadSize = (int64)IoBuffer->DataSize(); // readsize
		}
		else
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to Read %d."), Chunk->Request.Status().GetErrorCode());
			Chunk->ReadSize = 0;
			return CRIFS_IO_ERROR_NG;
		}*/

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		FCriWareChunk* Chunk = (FCriWareChunk*)InFile;

		*OutResult = Chunk->Request.Status().IsCompleted() ? CRI_TRUE : CRI_FALSE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64* OutReadSize)
	{
		assert(InFile != nullptr);
		assert(OutReadSize != nullptr);

		FCriWareChunk* Chunk = (FCriWareChunk*)InFile;

		// check error
		if (!Chunk->Request.Status().IsOk())
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to Read %d."), Chunk->Request.Status().GetErrorCode());
			Chunk->ReadSize = -1; // signal error
		}
		else
		{
			// get read size
			const FIoBuffer* IoBuffer = Chunk->Request.GetResult();
			Chunk->ReadSize = (int64)IoBuffer->DataSize();
		}

		// Store the read size
		*OutReadSize = Chunk->ReadSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(WriteSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot write file to IoStore."));
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		// Always return TRUE as it is not available
		*OutResult = CRI_TRUE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64* OutWriteSize)
	{
		assert(InFile != nullptr);
		assert(OutWriteSize != nullptr);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot write file to IoStore."));
		return CRIFS_IO_ERROR_NG;
	}

	//~ Setup parallel read to unlimit file access in editor and in game with IoStore. 
	CriFsIoError CRIAPI CanParallelRead(CriBool* OutResult)
	{
		*OutResult = CRI_TRUE;

		return CRIFS_IO_ERROR_OK;
	}
} // namespace


//~ Async Reader (FBulkData streaming)
struct FCriWareAsyncFileHandle
{
	IAsyncReadFileHandle* FileHandle;
	IAsyncReadRequest* Request;
	int64 DataOffset = 0;
	int64 FileSize = 0;
	int64 ReadSize = 0;
	int64 WriteSize = 0;
};

//~ Functions for the interface registration
namespace CriWareAtomFileAsync
{
	CriFsIoError CRIAPI Exists(const CriChar8* InFilename, CriBool* OutResult);
	CriFsIoError CRIAPI Remove(const CriChar8* InFilename);
	CriFsIoError CRIAPI Open(const CriChar8* InFilename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn* OutFile);
	CriFsIoError CRIAPI Close(CriFsFileHn InFile);
	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64* OutFileSize);
	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void* Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool* OutResult);
	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64* OutReadSize);
	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void* Buffer, CriSint64 BufferSize);
	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool* OutResult);
	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64* OutWriteSize);
	CriFsIoError CRIAPI CanParallelRead(CriBool* result);
};

//~ I/O processing interface
CriFsIoInterface GAtomCriFileAsyncInferface = {
	CriWareAtomFileAsync::Exists,
	CriWareAtomFileAsync::Remove,
	nullptr,
	CriWareAtomFileAsync::Open,
	CriWareAtomFileAsync::Close,
	CriWareAtomFileAsync::GetFileSize,
	CriWareAtomFileAsync::Read,
	CriWareAtomFileAsync::IsReadComplete,
	nullptr,
	CriWareAtomFileAsync::GetReadSize,
	CriWareAtomFileAsync::Write,
	CriWareAtomFileAsync::IsWriteComplete,
	nullptr,
	CriWareAtomFileAsync::GetWriteSize,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	CriWareAtomFileAsync::CanParallelRead
};

namespace CriWareAtomFileAsync
{

	static bool CheckUsingIOAsync(const FString& Filename)
	{
		return !Filename.IsEmpty() && Filename[0] == TEXT('%');
	}

	static FByteBulkData* GetBulkData(const FString& Filename)
	{
		if (FAtomResourceManager* ResourceManager = GCriWare->GetAtomResourceManager())
		{
			uint32 ResourceID = FCString::Atoi(*Filename.RightChop(1));

			// TODO: USE A PROXY !!
			FAtomResourceHandle ResourceHandle = ResourceManager->GetResource(ResourceID);
			if (ResourceHandle.IsValid())
			{
				if (UAtomSoundBank* Source = Cast<UAtomSoundBank>(ResourceHandle.GetResource()->GetSoundResource()))
				{
					return &Source->RawData;
				}
			}
		}

		return nullptr;
	}

	// Check whether the file exists
	CriFsIoError CRIAPI Exists(const CriChar8* InFilename, CriBool* OutResult)
	{
		assert(InFilename != nullptr);
		assert(OutResult != nullptr);

		const FString Filename(UTF8_TO_TCHAR(InFilename));

		if (CheckUsingIOAsync(Filename))
		{
			// try get resource
			FByteBulkData* BulkData = GetBulkData(Filename);
			*OutResult = (BulkData && BulkData->DoesExist()) ? CRI_TRUE : CRI_FALSE;
		}
		else
		{
			UE_LOG(LogCriWareCore, Error, TEXT("File %s if not on file system."), *Filename);
			return CRIFS_IO_ERROR_NG;
		}

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Remove(const CriChar8* InFilename)
	{
		assert(InFilename != nullptr);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot delete file %s from bulk data."), UTF8_TO_TCHAR(InFilename));
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI Open(const CriChar8* InFilename, CriFsFileMode Mode, CriFsFileAccess Access, CriFsFileHn* OutFile)
	{
		assert(InFilename != nullptr);
		assert(OutFile != nullptr);

		*OutFile = nullptr;
		const FString Filename(UTF8_TO_TCHAR(InFilename));

		if (CheckUsingIOAsync(Filename))
		{
			// try get resource
			FByteBulkData* BulkData = GetBulkData(Filename);
			if (!BulkData)
			{
				UE_LOG(LogCriWareCore, Warning, TEXT("File %s is not valid."), *Filename);
				return CRIFS_IO_ERROR_NG;
			}

			IAsyncReadFileHandle* FileHandle = BulkData->OpenAsyncReadHandle();
			if (!FileHandle)
			{
				UE_LOG(LogCriWareCore, Warning, TEXT("Could not find open file %s from bulk data."), *Filename);
				return CRIFS_IO_ERROR_NG;
			}

			// Allocate a file handle area
			FCriWareAsyncFileHandle* Handle = new FCriWareAsyncFileHandle();
			Handle->FileHandle = FileHandle;
			Handle->DataOffset = BulkData->GetBulkDataOffsetInFile();
			Handle->FileSize = BulkData->GetBulkDataSize();

			// Store the file handle
			*OutFile = Handle;

			return CRIFS_IO_ERROR_OK;
		}

		UE_LOG(LogCriWareCore, Error, TEXT("File %s do not use IoStore."), *Filename);
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI Close(CriFsFileHn InFile)
	{
		assert(InFile != nullptr);

		FCriWareAsyncFileHandle* Handle = (FCriWareAsyncFileHandle*)InFile;

		if (Handle->Request)
		{
			delete Handle->Request;
			Handle->Request = nullptr;
		}

		// Close the file
		if (Handle->FileHandle != nullptr)
		{
			delete Handle->FileHandle;
			Handle->FileHandle = nullptr;
		}

		// Deallocate the handle area
		delete Handle;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetFileSize(CriFsFileHn InFile, CriSint64* OutFileSize)
	{
		assert(InFile != nullptr);
		assert(OutFileSize != nullptr);

		FCriWareAsyncFileHandle* Handle = (FCriWareAsyncFileHandle*)InFile;

		// Store the file size
		*OutFileSize = Handle->FileSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Read(CriFsFileHn InFile, CriSint64 Offset, CriSint64 ReadSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(ReadSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		FCriWareAsyncFileHandle* Handle = (FCriWareAsyncFileHandle*)InFile;

		// Get read size
		ReadSize = FMath::Min(ReadSize, BufferSize);
		ReadSize = FMath::Min(ReadSize, (CriSint64)Handle->FileSize - Offset);

		// Do nothing if no read required
		if (ReadSize <= 0)
		{
			Handle->ReadSize = 0;
			return CRIFS_IO_ERROR_OK;
		}

		Offset += Handle->DataOffset;

		// request read
		Handle->Request = Handle->FileHandle->ReadRequest((int64)Offset, (int64)ReadSize, EAsyncIOPriorityAndFlags::AIOP_MAX, nullptr, (uint8*)Buffer);
		Handle->ReadSize = ReadSize;

		/*
		// wait for completion (sync)
		bool Result = Handle->Request->WaitCompletion();

		// check error
		if (!Result)
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to Read."));
			Handle->ReadSize = 0;
			return CRIFS_IO_ERROR_NG;
		}
		*/

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI IsReadComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		FCriWareAsyncFileHandle* Handle = (FCriWareAsyncFileHandle*)InFile;
		if (Handle->Request)
		{
			*OutResult = Handle->Request->PollCompletion() ? CRI_TRUE : CRI_FALSE;
			return CRIFS_IO_ERROR_OK;
		}

		*OutResult = CRI_FALSE;
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI GetReadSize(CriFsFileHn InFile, CriSint64* OutReadSize)
	{
		assert(InFile != nullptr);
		assert(OutReadSize != nullptr);

		FCriWareAsyncFileHandle* Handle = (FCriWareAsyncFileHandle*)InFile;

		if (Handle->Request && Handle->Request->GetReadResults())
		{
			delete Handle->Request;
			Handle->Request = nullptr;
		}
		else
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to Read."));
			Handle->ReadSize = -1; // signal error
		}

		// Store the read size
		*OutReadSize = Handle->ReadSize;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI Write(CriFsFileHn InFile, CriSint64 Offset, CriSint64 WriteSize, void* Buffer, CriSint64 BufferSize)
	{
		assert(InFile != nullptr);
		assert(Offset >= 0);
		assert(WriteSize >= 0);
		assert(Buffer != nullptr);
		assert(BufferSize >= 0);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot write file to bulk data."));
		return CRIFS_IO_ERROR_NG;
	}

	CriFsIoError CRIAPI IsWriteComplete(CriFsFileHn InFile, CriBool* OutResult)
	{
		assert(InFile != nullptr);
		assert(OutResult != nullptr);

		// Always return TRUE as it is not available
		*OutResult = CRI_TRUE;

		return CRIFS_IO_ERROR_OK;
	}

	CriFsIoError CRIAPI GetWriteSize(CriFsFileHn InFile, CriSint64* OutWriteSize)
	{
		assert(InFile != nullptr);
		assert(OutWriteSize != nullptr);

		UE_LOG(LogCriWareCore, Error, TEXT("Cannot write file to bulk data."));
		return CRIFS_IO_ERROR_NG;
	}

	//~ Setup parallel read to unlimit file access in editor and in game with IoStore. 
	CriFsIoError CRIAPI CanParallelRead(CriBool* OutResult)
	{
		*OutResult = CRI_FALSE;

		return CRIFS_IO_ERROR_OK;
	}
} // namespace
