
#pragma once

#include "Containers/Map.h"
#include "Templates/SharedPointer.h"

namespace Atom::Insights
{
	struct FTraceChannelHandle;

	class CRIWAREATOMINSIGHTS_API FTraceChannelManager : public TSharedFromThis<FTraceChannelManager>
	{
		FCriticalSection CritSect;
		TMap<FString, uint32> ActiveChannels;

		friend struct FTraceChannelHandle;

	public:

		FTraceChannelHandle CreateHandle(FString&& InName);
	};

	struct CRIWAREATOMINSIGHTS_API FTraceChannelHandle
	{
	private:

		FString ChannelName;
		TSharedRef<FTraceChannelManager> Manager;

		void Reset();
		void Init();

		FTraceChannelHandle(FString&& InName, TSharedRef<FTraceChannelManager> InManager);

	public:

		FTraceChannelHandle() = default;
		FTraceChannelHandle(FTraceChannelHandle&&);
		FTraceChannelHandle(const FTraceChannelHandle&);
		FTraceChannelHandle& operator=(const FTraceChannelHandle& Other);
		FTraceChannelHandle& operator=(FTraceChannelHandle&& Other);
		~FTraceChannelHandle();

		friend FORCEINLINE uint32 GetTypeHash(const FTraceChannelHandle& Handle)
		{
			return GetTypeHash(Handle.ChannelName);
		}

		bool operator==(const FTraceChannelHandle& Other) const;
		bool operator!=(const FTraceChannelHandle& Other) const;

		bool IsValid() const;

		friend class FTraceChannelManager;
	};
} // namespace
