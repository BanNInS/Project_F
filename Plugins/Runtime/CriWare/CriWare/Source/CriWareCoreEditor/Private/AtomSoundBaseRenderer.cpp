
#include "AtomSoundBaseRenderer.h"

#include "UObject/Package.h"

#include "CriWare.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomCueSheetResource.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomActiveSound.h"

FByteBulkData& UAtomSoundBaseRenderer::Open(UAtomSoundBase* InAtomSoundBase, int InNumFrames)
{
	if (InAtomSoundBase == nullptr)
	{
		return RawSnapshot;
	}

	AtomSound = InAtomSoundBase;

	// postpone generation until no PIE/SIE
	if (GIsPlayInEditorWorld)
	{
		return RawSnapshot;
	}

	bIsRequestingSnapshot = false;

	// clear current snapshot
	if (RawSnapshot.GetBulkDataSize() > 0)
	{
		RawSnapshot.RemoveBulkData();
	}

	//UE_LOG(LogCriWareAtomDebug, Display, TEXT("Tries generates Atom Snapshot for %s (Generators in Flight: %d/5"), *InAtomSoundBase->GetFName().ToString(), NumSnapshotGeneratorsInFlight);

	// check if we can generate it now, or retry
	if (NumSnapshotGeneratorsInFlight >= MaxSimultaneousSnapshotGenerators)
	{
		bIsRequestingSnapshot = true;
		return RawSnapshot;
	}

	IAtomSoundResource* SoundResource = AtomSound->GetSoundResource();
	if (SoundResource && GCriWare) // is valid?
	{
		NumSnapshotGeneratorsInFlight++;

		auto ResourceManager = GCriWare->GetAtomResourceManager();
		auto RuntimeManager = GCriWare->GetAtomRuntimeManager();

		// create a new resouce handle, load if not yet
		FAtomLoadParams LoadParams;
		LoadParams.Source = SoundResource;
		LoadParams.bAsyncLoad = true;
		LoadParams.Scope = EAtomResourceScope::Shared;
		//LoadParams.RuntimeID = RuntimeManager->GetMainAtomRuntimeID();
		LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::NormalTaskPriority;
		LoadParams.OnLoadCompleted = [WeakThis = MakeWeakObjectPtr(this), NumFrames = InNumFrames](EAtomResourceLoadResult LoadResult)
		{
			if (WeakThis.IsValid() && (LoadResult == EAtomResourceLoadResult::Completed || LoadResult == EAtomResourceLoadResult::AlreadyLoaded))
			{
				auto* ThisRenderer = WeakThis.Get();
				UAtomSoundBase* ThisSound = ThisRenderer->AtomSound;
				FAtomResourceHandle& ResourceHandle = ThisRenderer->SnapshotResource;

				// If we hit this, something went wrong in RequestSoundResource.
				if (!ResourceHandle.IsValid())
				{
					UE_LOG(LogCriWareAtom, Display, TEXT("Failed to retrieve sound resource data from AtomSoundBase %s after successfully requesting it!"), *(WeakThis->GetName()));
					NumSnapshotGeneratorsInFlight--;
					return;
				}

				TSharedRef<FAtomWaveInfo, ESPMode::ThreadSafe> SoundWaveInfo = MakeShared<FAtomWaveInfo>();
				
				// Sound Cue use "first wave info" if accessible or exists (A cue can have no waves).
				if (UAtomSoundCue* ThisCue = Cast<UAtomSoundCue>(ThisSound))
				{
					if (FAtomCueSheetResource* CueSheetResource = ResourceHandle->GetCueSheetResource())
					{
						// force get wave info from the wave bank when streamed.
						CueSheetResource->GetWaveformInfo(ThisCue->CueInfo.ID, SoundWaveInfo.Get(), true, true);
					}
				}
				else
				{
					ThisSound->GetWaveInfo(ResourceHandle, SoundWaveInfo.Get());
				}

				// create output memory buffer
				struct FWaveformBuffer
				{
					int32 Total;
					int32 Count;
					int16* Pcm;
				};

				if (SoundWaveInfo->NumFrames <= 0 || SoundWaveInfo->NumChannels <= 0)
				{
					// the sound have wave info empty, nothing to generate
					ThisRenderer->SnapshotResource.Reset();
					NumSnapshotGeneratorsInFlight--;
					return;
				}

				// create a buffer to hold pcm data
				TSharedRef<FWaveformBuffer, ESPMode::ThreadSafe> WaveformBuffer = MakeShared<FWaveformBuffer>();
				WaveformBuffer->Total = SoundWaveInfo->NumFrames;
				WaveformBuffer->Count = 0;
				WaveformBuffer->Pcm = static_cast<int16*>(FMemory::Malloc(sizeof(int16) * SoundWaveInfo->NumFrames * SoundWaveInfo->NumChannels));
				if (WaveformBuffer->Pcm == nullptr)
				{
					// OUT: ERROR - out of memory!
					ThisRenderer->SnapshotResource.Reset();
					NumSnapshotGeneratorsInFlight--;
					return;
				}

				// setup the Pcm filter function 
				auto FilterDataFunction = [WaveformBuffer, SoundWaveInfo](UPTRINT, EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
				{
					FWaveformBuffer& DecodeBuffer = WaveformBuffer.Get();

					// clipping
					NumChannels = FMath::Min((int32)NumChannels, 8);
					NumFrames = FMath::Min((int32)NumFrames, (DecodeBuffer.Total - DecodeBuffer.Count));

					// set source data
					float* Source[8];
					for (int32 n = 0; n < NumChannels; n++)
					{
						Source[n] = (float*)Data[n];
					}

					// decode data to buffer
					int16* Dest = &DecodeBuffer.Pcm[DecodeBuffer.Count * NumChannels];
					for (int32 i = 0; i < NumFrames; i++)
					{
						for (int32 n = 0; n < NumChannels; n++)
						{
							int32 Value = Source[n][i] * 32768.0f;
							Value = FMath::Clamp(Value, (int32)MIN_int16, (int32)MAX_int16);
							Dest[i * NumChannels + n] = (int16)Value;
						}
					}

					// advance frame counter
					DecodeBuffer.Count += NumFrames;
					//UE_LOG(LogTemp, Warning, TEXT("DECODE (%i/%i)"), DecodeBuffer->Count, DecodeBuffer->Total);
				};

				// On Finished
				auto OnPlaybackFinished = [WeakThis, WaveformBuffer, SoundWaveInfo, NumDestFrames = (uint32)NumFrames](bool bWasEndReached)
				{
					if (TObjectPtr<UAtomSoundBaseRenderer> ThisRenderer = WeakThis.Get())
					{
						TObjectPtr<UAtomSoundBase> ThisSound = ThisRenderer->AtomSound;

						if (WaveformBuffer->Pcm == nullptr)
						{
							ThisRenderer->SnapshotResource.Reset();
							NumSnapshotGeneratorsInFlight--;
							return;
						}

						if (WaveformBuffer->Count == WaveformBuffer->Total)
						{
							// Sample count
							const uint32 NumChannels = SoundWaveInfo->NumChannels;
							const uint32 TotalFrameCount = WaveformBuffer->Count;
							const uint32 FramesPerDstFrame = TotalFrameCount / NumDestFrames;
							const float FrameScale = (float)TotalFrameCount / NumDestFrames;

							// Get the sample data of this file
							const int16* SamplePtr = WaveformBuffer->Pcm;

							ThisSound->RawSnapshot.Lock(LOCK_READ_WRITE);
							void* Ptr = ThisSound->RawSnapshot.Realloc(NumDestFrames * sizeof(int16)* NumChannels);
							int16* DestPtr = static_cast<int16*>(Ptr);

							// Render each channel separately so outer loop is the sound wave channel index.
							for (uint32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
							{
								// Reset the current frame count as we're starting from the beginning of the file to
								// scale the channel data
								uint32 CurrentFrameCount = 0;

								// Loop through each dest frames
								for (uint32 DestFrameIndex = 0; DestFrameIndex < NumDestFrames; ++DestFrameIndex)
								{
									// reset the sample sum and num samples in pixel for each pixel
									int64 SampleSum = 0;
									int32 NumSamplesInDstFrame = 0;

									if (FramesPerDstFrame > 0)
									{
										// resampling

										// Loop through all source frames in this dest frame, sum all audio data. Track total frames rendered to avoid writing past buffer boundary
										for (uint32 FrameIndex = 0; FrameIndex < FramesPerDstFrame && CurrentFrameCount < TotalFrameCount; ++FrameIndex)
										{
											// Get the sample value of the wave file
											const uint32 SampleIndex = CurrentFrameCount * NumChannels + ChannelIndex;
											check(SampleIndex < (TotalFrameCount * NumChannels));
											int16 SampleValue = SamplePtr[SampleIndex];

											// Sum the sample value with the running sum
											SampleSum += FMath::Abs(SampleValue);

											// Track the number of samples we're actually summing to get an accurate average
											++NumSamplesInDstFrame;

											// Increment the frame after processing channels
											++CurrentFrameCount;
										}
									}
									else
									{
										// interpolation to nearest

										// Get the sample value of the wave file
										const uint32 SampleIndex = (uint32)(CurrentFrameCount * FrameScale) * NumChannels + ChannelIndex;
										check(SampleIndex < (TotalFrameCount * NumChannels));
										int16 SampleValue = SamplePtr[SampleIndex];

										// copy the sample value
										SampleSum = FMath::Abs(SampleValue);

										// Only one sample is added per frame
										NumSamplesInDstFrame = 1;

										// Increment the frame after processing channels
										++CurrentFrameCount;
									}

									// If we actually added any audio data in this dest frame
									if (NumSamplesInDstFrame > 0)
									{
										const int16 AverageSampleValue = (int16)((float)SampleSum / NumSamplesInDstFrame);
										DestPtr[DestFrameIndex * NumChannels + ChannelIndex] = AverageSampleValue;
									}
								}
							}

							ThisSound->RawSnapshot.Unlock();
							ThisSound->RawSnapshotNumChannels = NumChannels;

							// snapshot was generated, re-generate thumbnail
						}
						else
						{
							// ask for regenerate in case of player stopped early by a global stop for PIE/SIE
							if (!bWasEndReached && GIsPlayInEditorWorld)
							{
								ThisRenderer->bIsRequestingSnapshot = true;
							}
						}

						ThisRenderer->SnapshotResource.Reset();
					}

					FMemory::Free(WaveformBuffer->Pcm);
					WaveformBuffer->Pcm = nullptr;
					WaveformBuffer->Count = 0;
					WaveformBuffer->Total = 0;

					NumSnapshotGeneratorsInFlight--;
				};

				// create a new active sound using a native AtomPlayer with the pcm filter set
				FAtomActiveSound NewActiveSound;
				NewActiveSound.SetSound(ThisSound);
				NewActiveSound.bForceUseAtomPlayer = true;
				NewActiveSound.SetVolume(0.0f);
				NewActiveSound.bIsUISound = true; // doesn't need game tick
				//NewActiveSound.bIsPreviewSound = true; // doesn't need to apply parameters
				NewActiveSound.PcmFilterFunction = FilterDataFunction; // actually using a pcm filter disables looping
				NewActiveSound.ThreadToCallOnSoundFinishedOn = ENamedThreads::GameThread;
				NewActiveSound.OnSoundFinished = OnPlaybackFinished;
#if !(NO_LOGGING || UE_BUILD_SHIPPING || UE_BUILD_TEST)
				NewActiveSound.bWarnedAboutOrphanedLooping = true; // avoid orphaned warning
#endif

				auto Runtime = GCriWare->GetActiveAtomRuntime();
				Runtime->AddNewActiveSound(NewActiveSound);
			}
		};

		SnapshotResource = ResourceManager->RequestSoundResource(LoadParams);
	}

	return RawSnapshot;
}

FByteBulkData& UAtomSoundBaseRenderer::Open(UAtomSoundBase* InAtomSoundBase)
{
	if (InAtomSoundBase == nullptr)
	{
		return RawPCMData;
	}

	AtomSound = InAtomSoundBase;

	// postpone generation until no PIE/SIE
	if (GIsPlayInEditorWorld)
	{
		return RawPCMData;
	}

	bIsRequestingPCMData = false;

	// clear current data
	{
		FScopeLock Lock(&InAtomSoundBase->RawPCMDataCriticalSection);
		if (InAtomSoundBase->RawPCMData)
		{
			FMemory::Free(InAtomSoundBase->RawPCMData);
			InAtomSoundBase->RawPCMData = nullptr;
		}
	}

	// check if we can generate it now, or retry
	if (NumPCMGeneratorsInFlight >= MaxSimultaneousPCMGenerators)
	{
		bIsRequestingPCMData = true;
		return RawPCMData;
	}

	IAtomSoundResource* SoundResource = AtomSound->GetSoundResource();
	if (SoundResource && GCriWare) // is valid
	{
		NumPCMGeneratorsInFlight++;

		auto ResourceManager = GCriWare->GetAtomResourceManager();
		auto RuntimeManager = GCriWare->GetAtomRuntimeManager();

		// create a new resouce handle, load if not yet
		FAtomLoadParams LoadParams;
		LoadParams.Source = SoundResource;
		LoadParams.bAsyncLoad = true;
		LoadParams.Scope = EAtomResourceScope::Shared;
		//LoadParams.RuntimeID = RuntimeManager->GetMainAtomRuntimeID();
		LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::NormalTaskPriority;
		LoadParams.OnLoadCompleted = [WeakThis = MakeWeakObjectPtr(this)](EAtomResourceLoadResult LoadResult)
		{
			if (WeakThis.IsValid() && (LoadResult == EAtomResourceLoadResult::Completed || LoadResult == EAtomResourceLoadResult::AlreadyLoaded))
			{
				UAtomSoundBaseRenderer* ThisRenderer = WeakThis.Get();
				UAtomSoundBase* ThisSound = ThisRenderer->AtomSound;;
				FAtomResourceHandle& ResourceHandle = ThisRenderer->RawPCMDataResource;

				// If we hit this, something went wrong in RequestSoundResource.
				if (!ResourceHandle.IsValid())
				{
					UE_LOG(LogCriWareAtom, Display, TEXT("Failed to retrieve sound resource data from AtomSoundBase %s after successfully requesting it!"), *(WeakThis->GetName()));
					NumPCMGeneratorsInFlight--;
					return;
				}

				auto SoundWaveInfo = MakeShared<FAtomWaveInfo>();
				// Sound Cue use "first wave info" if accessible or exists (A cue can have no waves).
				if (UAtomSoundCue* ThisCue = Cast<UAtomSoundCue>(ThisSound))
				{
					if (FAtomCueSheetResource* CueSheetResource = ResourceHandle->GetCueSheetResource())
					{
						// force get wave info from the wave bank when streamed.
						CueSheetResource->GetWaveformInfo(ThisCue->CueInfo.ID, SoundWaveInfo.Get(), true, true);
					}
				}
				else
				{
					ThisSound->GetWaveInfo(ResourceHandle, SoundWaveInfo.Get());
				}

				// create output memory buffer
				struct FWaveformBuffer
				{
					int32 Total;
					int32 Count;
					int16* Pcm;
				};

				if (SoundWaveInfo->NumFrames <= 0 || SoundWaveInfo->NumChannels <= 0)
				{
					// the sound have wave info empty, nothing to generate
					ThisRenderer->RawPCMDataResource.Reset();
					NumPCMGeneratorsInFlight--;
					return;
				}

				// create a buffer to hold pcm data
				auto WaveformBuffer = MakeShared<FWaveformBuffer>();
				WaveformBuffer->Total = SoundWaveInfo->NumFrames;
				WaveformBuffer->Count = 0;
				WaveformBuffer->Pcm = static_cast<int16*>(FMemory::Malloc(sizeof(int16) * SoundWaveInfo->NumFrames * SoundWaveInfo->NumChannels));
				if (WaveformBuffer->Pcm == nullptr)
				{
					// OUT: ERROR - out of memory!
					ThisRenderer->RawPCMDataResource.Reset();
					NumPCMGeneratorsInFlight--;
					return;
				}

				// setup the Pcm filter function 
				auto FilterDataFunction = [WaveformBuffer, SoundWaveInfo](UPTRINT, EAtomPcmBitDepth BitDepth, int32 NumChannels, int32 NumFrames, void* Data[])
				{
					FWaveformBuffer& DecodeBuffer = WaveformBuffer.Get();

					// clipping
					NumChannels = FMath::Min((int32)NumChannels, 8);
					NumFrames = FMath::Min((int32)NumFrames, (DecodeBuffer.Total - DecodeBuffer.Count));

					// set source data
					float* Source[8];
					for (int32 n = 0; n < NumChannels; n++)
					{
						Source[n] = (float*)Data[n];
					}

					// decode data to buffer
					int16* Dest = &DecodeBuffer.Pcm[DecodeBuffer.Count * NumChannels];
					for (int32 i = 0; i < NumFrames; i++)
					{
						for (int32 n = 0; n < NumChannels; n++)
						{
							int32 Value = Source[n][i] * 32768.0f;
							Value = FMath::Clamp(Value, (int32)MIN_int16, (int32)MAX_int16);
							Dest[i * NumChannels + n] = (int16)Value;
						}
					}

					// advance frame counter
					DecodeBuffer.Count += NumFrames;
					//UE_LOG(LogTemp, Warning, TEXT("DECODE (%i/%i)"), DecodeBuffer->Count, DecodeBuffer->Total);
				};

				// On Finished
				auto OnPlaybackFinished = [WeakThis, WaveformBuffer, SoundWaveInfo](bool bWasEndReached)
				{
					if (TObjectPtr<UAtomSoundBaseRenderer> ThisRenderer = WeakThis.Get())
					{
						TObjectPtr<UAtomSoundBase> ThisSound = ThisRenderer->AtomSound;

						if (WaveformBuffer->Pcm == nullptr)
						{
							ThisRenderer->RawPCMDataResource.Reset();
							NumSnapshotGeneratorsInFlight--;
							return;
						}

						if (WaveformBuffer->Count == WaveformBuffer->Total)
						{
							// save result
							FScopeLock Lock(&ThisSound->RawPCMDataCriticalSection);
							ThisSound->RawPCMData = (uint8*)WaveformBuffer->Pcm;
							ThisSound->RawPCMDataNumChannels = SoundWaveInfo->NumChannels;
							ThisSound->RawPCMDataSampleRate = SoundWaveInfo->SampleRate;
							ThisSound->RawPCMDataNumFrames = SoundWaveInfo->NumFrames;
						}
						else
						{
							FMemory::Free(WaveformBuffer->Pcm);
							WaveformBuffer->Pcm = nullptr;
							WaveformBuffer->Count = 0;
							WaveformBuffer->Total = 0;

							// ask for regenerate in case of player stopped early by a global stop for PIE/SIE
							if (!bWasEndReached && GIsPlayInEditorWorld)
							{
								ThisRenderer->bIsRequestingPCMData = true;
							}
						}

						ThisRenderer->RawPCMDataResource.Reset();
					}

					NumPCMGeneratorsInFlight--;
				};

				// create a new active sound using a native AtomPlayer with the pcm filter set
				FAtomActiveSound NewActiveSound;
				NewActiveSound.SetSound(ThisSound);
				NewActiveSound.bForceUseAtomPlayer = true;
				NewActiveSound.SetVolume(0.0f);
				NewActiveSound.bIsUISound = true; // doesn't need game tick
				NewActiveSound.bIsPreviewSound = true; // doesn't need to apply parameters
				NewActiveSound.PcmFilterFunction = FilterDataFunction; // actually using a pcm filter disable looping
				NewActiveSound.ThreadToCallOnSoundFinishedOn = ENamedThreads::GameThread;
				NewActiveSound.OnSoundFinished = OnPlaybackFinished;
#if !(NO_LOGGING || UE_BUILD_SHIPPING || UE_BUILD_TEST)
				NewActiveSound.bWarnedAboutOrphanedLooping = true; // avoid orphaned warning
#endif

				auto Runtime = GCriWare->GetActiveAtomRuntime();
				Runtime->AddNewActiveSound(NewActiveSound);
			}
		};

		RawPCMDataResource = ResourceManager->RequestSoundResource(LoadParams);
	}

	return RawPCMData;
}

void UAtomSoundBaseRenderer::Tick(float DeltaTime)
{
	if (bIsRequestingSnapshot)
	{
		// retry
		Open(AtomSound, 512);
	}

	if (bIsRequestingPCMData)
	{
		// retry
		Open(AtomSound);
	}
}

void UAtomSoundBaseRenderer::OnPlaybackEnded()
{
	
}

void UAtomSoundBaseRenderer::Close()
{

}
