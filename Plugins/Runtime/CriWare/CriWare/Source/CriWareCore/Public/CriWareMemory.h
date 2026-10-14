

#pragma once

#include "CoreMinimal.h"
#include "CriWareApi.h"

enum class ECriWareMemoryType : uint8
{
	FileSystemMemory,
	AtomMemory,
	ManaMemory,
	ManaVp9Memory,

	StartPluginMemory = 100,
	EndPluginMemory = 150
};

/** Implementation of native memory allocation callbacks. */
namespace FCriWareAllocator_Native
{
	extern "C" CRIWARECORE_API void* CRIAPI AllocateMemory(void* ObjectPointer, CriUint32 Size);
	extern "C" CRIWARECORE_API void* CRIAPI ReallocateMemory(void* ObjectPointer, void* Pointer, CriUint32 Size);
	extern "C" CRIWARECORE_API void CRIAPI FreeMemory(void* ObjectPointer, void* Pointer);
}

/** Structure to maintain allocation functions and allow c++ classes method functions for CriWare libraries. */
struct CRIWARECORE_API FCriWareAllocator
{
	friend void* CRIAPI FCriWareAllocator_Native::AllocateMemory(void* ObjectPointer, CriUint32 Size);
	friend void* CRIAPI FCriWareAllocator_Native::ReallocateMemory(void* ObjectPointer, void* Pointer, CriUint32 Size);
	friend void CRIAPI FCriWareAllocator_Native::FreeMemory(void* ObjectPointer, void* Pointer);

	using FAllocator = TFunction<void* (void* ObjectPointer, unsigned int Size)>;
	using FReallocator = TFunction<void* (void* ObjectPointer, void* Pointer, unsigned int Size)>;
	using FDeallocator = TFunction<void (void* ObjectPointer, void* Pointer)>;

private:

	FAllocator AllocateCallback;
	FReallocator ReallocateCallback;
	FDeallocator FreeCallback;
	void* ObjectPointer;

public:

	FCriWareAllocator() : FCriWareAllocator(DefaultAlloc, DefaultRealloc, DefaultFree, nullptr) {}

	FCriWareAllocator(const FAllocator& AllocFunc, const FDeallocator& FreeFunc, void* InObjectPointer)
		: AllocateCallback(AllocFunc)
		, ReallocateCallback(nullptr)
		, FreeCallback(FreeFunc)
		, ObjectPointer(InObjectPointer)
	{
	}

	FCriWareAllocator(const FAllocator& AllocFunc, const FReallocator& ReallocFunc, const FDeallocator& FreeFunc, void* InObjectPointer)
		: AllocateCallback(AllocFunc)
		, ReallocateCallback(ReallocFunc)
		, FreeCallback(FreeFunc)
		, ObjectPointer(InObjectPointer)
	{
	}

public:

	static FORCEINLINE FCriWareAllocator& GetDefault() { return DefaultAllocator; }

protected:

	static FCriWareAllocator DefaultAllocator;
	
	static void* DefaultAlloc(void* ObjectPointer, unsigned int Size);
	static void* DefaultRealloc(void* ObjectPointer, void* Pointer, unsigned int Size);
	static void DefaultFree(void* ObjectPointer, void* Pointer);
};
