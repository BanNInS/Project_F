
#include "Atom/Mixer/AtomMixerChannelMaps.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Mixer/AtomMixer.h"

namespace Atom
{
	namespace ChannelMapPrivate
	{
		// Tables based on Ac-3 down-mixing
		// Rows: output speaker configuration
		// Cols: input channels

		static constexpr float ToMonoMatrix[ChannelMapMaxNumChannels * 1] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
			0.707f,			0.707f,		1.0f,		0.0f,			0.5f,		0.5f,		0.5f,		0.5f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
		};

		static constexpr float VorbisToMonoMatrix[ChannelMapVorbisNumChannels * 1] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			0.707f,			1.0f,		0.707f,		0.5f,			0.5f,		0.0f,		// FrontLeft
		};

		static constexpr float ToStereoMatrix[ChannelMapMaxNumChannels * 2] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
			1.0f,			0.0f,		0.707f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
		};

		static constexpr float VorbisToStereoMatrix[ChannelMapVorbisNumChannels * 2] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.707f,		0.0f,		0.707f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.707f,		1.0f,		0.0f,			0.707f,		0.0f,		// FrontRight
		};

		static constexpr float ToTriMatrix[ChannelMapMaxNumChannels * 3] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
			1.0f,			0.0f,		0.0f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// Center
		};

		static constexpr float VorbisToTriMatrix[ChannelMapVorbisNumChannels * 3] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.707f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.707f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
		};

		static constexpr float ToQuadMatrix[ChannelMapMaxNumChannels * 4] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
			1.0f,			0.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisToQuadMatrix[ChannelMapVorbisNumChannels * 4] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.707f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.707f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
		};

		static constexpr float To5Matrix[ChannelMapMaxNumChannels * 5] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// Center
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo5Matrix[ChannelMapVorbisNumChannels * 5] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
		};

		static constexpr float To5Point1Matrix[ChannelMapMaxNumChannels * 6] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// Center
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo5Point1Matrix[ChannelMapVorbisNumChannels * 6] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency	
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
		};

		static constexpr float ToHexMatrix[ChannelMapMaxNumChannels * 7] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
			1.0f,			0.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LFE
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisToHexMatrix[ChannelMapVorbisNumChannels * 7] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency	
			1.0f,			0.707f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.707f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LFE
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
		};

#if PLATFORM_MICROSOFT

		static constexpr float To7Point1Matrix[ChannelMapMaxNumChannels * 8] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
		};

#else //PLATFORM_MICROSOFT

		// NOTE: the BackLeft/BackRight and SideLeft/SideRight are reversed than they should be since our 7.1 importer code has it backward
		static constexpr float To7Point1Matrix[ChannelMapMaxNumChannels * 8] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

#endif//PLATFORM_MICROSOFT

		static constexpr float VorbisTo7Point1Matrix[ChannelMapVorbisNumChannels * 8] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
		};

		// 9
		static constexpr float To9Matrix[ChannelMapMaxNumChannels * 9] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo9Matrix[ChannelMapVorbisNumChannels * 9] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To7Point1Point2Matrix[ChannelMapMaxNumChannels * 10] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo7Point1Point2Matrix[ChannelMapVorbisNumChannels * 10] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To11Matrix[ChannelMapMaxNumChannels * 11] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo11Matrix[ChannelMapVorbisNumChannels * 11] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To7Point1Point4Matrix[ChannelMapMaxNumChannels * 12] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo7Point1Point4Matrix[ChannelMapVorbisNumChannels * 12] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To13Matrix[ChannelMapMaxNumChannels * 13] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo13Matrix[ChannelMapVorbisNumChannels * 13] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To14Matrix[ChannelMapMaxNumChannels * 14] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo14Matrix[ChannelMapVorbisNumChannels * 14] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To15Matrix[ChannelMapMaxNumChannels * 15] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
		};

		static constexpr float VorbisTo15Matrix[ChannelMapVorbisNumChannels * 15] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static constexpr float To7Point1Point4Point4Matrix[ChannelMapMaxNumChannels * 16] =
		{
			// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontLeft
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontRight
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// FrontCenter
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,	// SideRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,	0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,	// SideRight
		};

		static constexpr float VorbisTo7Point1Point4Point4Matrix[ChannelMapVorbisNumChannels * 16] =
		{
			// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
			1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
			0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
			0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
			0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
			0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
			0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		};

		static float const * const OutputChannelMaps[ChannelMapMaxNumChannels] =
		{
			ToMonoMatrix,
			ToStereoMatrix,
			ToTriMatrix,	// Experimental
			ToQuadMatrix,
			To5Matrix,		// Experimental
			To5Point1Matrix,
			ToHexMatrix,	// Experimental
			To7Point1Matrix,
			// added
			To9Matrix, // Experimental
			To7Point1Point2Matrix,
			To11Matrix, // Experimental
			To7Point1Point4Matrix,
			To13Matrix, // Experimental
			To14Matrix, // Experimental
			To15Matrix, // Experimental
			To7Point1Point4Point4Matrix
		};

		// 5.1 Vorbis files have a different channel order than normal
		static float const * const VorbisChannelMaps[ChannelMapMaxNumChannels] =
		{
			VorbisToMonoMatrix,
			VorbisToStereoMatrix,
			VorbisToTriMatrix,	// Experimental
			VorbisToQuadMatrix,
			VorbisTo5Matrix,		// Experimental
			VorbisTo5Point1Matrix,
			VorbisToHexMatrix,	// Experimental
			VorbisTo7Point1Matrix,
			// added
			VorbisTo9Matrix, // Experimental
			VorbisTo7Point1Point2Matrix,
			VorbisTo11Matrix, // Experimental
			VorbisTo7Point1Point4Matrix,
			VorbisTo13Matrix, // Experimental
			VorbisTo14Matrix, // Experimental
			VorbisTo15Matrix, // Experimental
			VorbisTo7Point1Point4Point4Matrix
		};

		// Create a channel map for a generic multichannel input to any supported
		// multichannel output.
		//
		// @param InNumInputChannels - The number of channels in the input content.
		// @param InNumOutputChannels - The number of channels in the output audio.
		// @param OutChannelMap - An array of channel gains in OutputMajorOrder (see EChannelMapOrder for details on format).
		//
		// @return true on success, false on failure. 
		bool Create2DGenericInputChannelMap(int32 InNumInputChannels, int32 InNumOutputChannels, TArray<float>& OutChannelMap)
		{
			const int32 OutputChannelMapIndex = InNumOutputChannels - 1;
			check(OutputChannelMapIndex >= 0);
			check(OutputChannelMapIndex < ChannelMapMaxNumChannels);

			const float* RESTRICT Matrix = OutputChannelMaps[OutputChannelMapIndex];
			check(Matrix != nullptr);

			for (int32 InputChannel = 0; InputChannel < InNumInputChannels; ++InputChannel)
			{
				for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
				{
					const int32 Index = OutputChannel * ChannelMapMaxNumChannels + InputChannel;
					OutChannelMap.Add(Matrix[Index]);
				}
			}

			return true;
		}

		// Create a channel map for a mono input to any supported multichannel output.
		//
		// @param InNumOutputChannels - The number of channels in the output audio.
		// @param InMonoChannelUpmixMethod - Method for upmixing mono to front-left and front-right (not used if bInIsCenterChannelOnly is true and output format has center channel, or if output is Mono or Quad).
		// @param bInIsCenterChannelOnly - If true, and if the output channel count corresponds to format containing a center channel, the mono input will be mapped directly to the center channel.
		// @param OutChannelMap - An array of channel gains in OutputMajorOrder (see EChannelMapOrder for details on format).
		//
		// @return true on success, false on failure. 
		bool Create2DMonoInputChannelMap(int32 InNumOutputChannels, EChannelMapMonoUpmixMethod InMonoChannelUpmixMethod, bool bInIsCenterChannelOnly, TArray<float>& OutChannelMap)
		{
			const int32 OutputChannelMapIndex = InNumOutputChannels - 1;
			check(OutputChannelMapIndex >= 0);
			check(OutputChannelMapIndex < ChannelMapMaxNumChannels);

			// Mono-in mono-out channel map
			if (InNumOutputChannels == 1)
			{
				OutChannelMap.Add(1.0f);
			}
			else
			{
				// If we have more than stereo output (means we have center channel, which is always the 3rd index)
				// Then we need to only apply 1.0 to the center channel, 0.0 for everything else
				if ((InNumOutputChannels == 3 || InNumOutputChannels > 4) && bInIsCenterChannelOnly)
				{
					for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
					{
						// Center channel is always 3rd index
						if (OutputChannel == 2)
						{
							OutChannelMap.Add(1.0f);
						}
						else
						{
							OutChannelMap.Add(0.0f);
						}
					}
				}
				else
				{
					// Mapping out to more than 2 channels, mono inputs should be equally spread to left and right
					switch (InMonoChannelUpmixMethod)
					{
						case EChannelMapMonoUpmixMethod::Linear:
							OutChannelMap.Add(0.5f);
							OutChannelMap.Add(0.5f);
							break;

						case EChannelMapMonoUpmixMethod::EqualPower:
							OutChannelMap.Add(0.707f);
							OutChannelMap.Add(0.707f);
							break;

						case EChannelMapMonoUpmixMethod::FullVolume:
							OutChannelMap.Add(1.0f);
							OutChannelMap.Add(1.0f);
							break;
					}

					const float* RESTRICT Matrix = OutputChannelMaps[OutputChannelMapIndex];
					check(Matrix != nullptr);

					for (int32 OutputChannel = 2; OutputChannel < InNumOutputChannels; ++OutputChannel)
					{
						const int32 Index = OutputChannel * ChannelMapMaxNumChannels;
						OutChannelMap.Add(Matrix[Index]);
					}
				}
			}

			return true;
		}

		// Create a channel map for a quad input to any supported multichannel output.
		//
		// @param InNumOutputChannels - The number of channels in the output audio.
		// @param OutChannelMap - An array of channel gains in OutputMajorOrder (see EChannelMapOrder for details on format).
		//
		// @return true on success, false on failure. 
		bool Create2DQuadInputChannelMap(int32 InNumOutputChannels, TArray<float>& OutChannelMap)
		{
			const int32 OutputChannelMapIndex = InNumOutputChannels - 1;

			check(OutputChannelMapIndex >= 0);
			check(OutputChannelMapIndex < ChannelMapMaxNumChannels);

			const float* RESTRICT Matrix = OutputChannelMaps[OutputChannelMapIndex];
			check(Matrix != nullptr);

			// Quad has a special-case to map input channels 0 1 2 3 to 0 1 4 5: 
			const int32 FrontLeftChannel = 0;
			for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
			{
				const int32 Index = OutputChannel * ChannelMapMaxNumChannels + FrontLeftChannel;
				OutChannelMap.Add(Matrix[Index]);
			}

			const int32 FrontRightChannel = 1;
			for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
			{
				const int32 Index = OutputChannel * ChannelMapMaxNumChannels + FrontRightChannel;
				OutChannelMap.Add(Matrix[Index]);
			}

			const int32 SurroundLeftChannel = 4;
			for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
			{
				const int32 Index = OutputChannel * ChannelMapMaxNumChannels + SurroundLeftChannel;
				OutChannelMap.Add(Matrix[Index]);
			}

			const int32 SurroundRightChannel = 5;
			for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
			{
				const int32 Index = OutputChannel * ChannelMapMaxNumChannels + SurroundRightChannel;
				OutChannelMap.Add(Matrix[Index]);
			}

			return true;
		}

		// Returns true if channel counts are supported by the hardcoded channel maps.
		bool EnsureValidChannelCounts(int32 InNumInputChannels, int32 InNumOutputChannels)
		{
			const bool bIsValidNumOutputChannels = (InNumOutputChannels > 0) && (InNumOutputChannels <= ChannelMapMaxNumChannels);
			if (ensureMsgf(bIsValidNumOutputChannels, TEXT("Invalid number of output channels: %d"), InNumOutputChannels))
			{
				const bool bIsValidNumInputChannels = (InNumInputChannels > 0) && (InNumInputChannels <= ChannelMapMaxNumChannels);
				if (ensureMsgf(bIsValidNumInputChannels, TEXT("Invalid number of input channels: %d"), InNumInputChannels))
				{
					return true;
				}
			}

			return false;
		}

		// Transpose a matrix from OutputMajorOrder to InputMajorOrder. See EChannelMapOrder
		// for details on the formats. 
		void TransposeOutputMajorOrderToInputMajorOrder(int32 InNumInputChannels, int32 InNumOutputChannels, TArray<float>& OutChannelMap)
		{
			check(OutChannelMap.Num() == (InNumInputChannels * InNumOutputChannels));

			// Transpose matrix to get input major order. 
			TArray<float> CopyOfMap = OutChannelMap;
			for (int32 InputChannelIndex = 0; InputChannelIndex < InNumInputChannels; InputChannelIndex++)
			{
				int32 OriginalIndex = InputChannelIndex * InNumOutputChannels;
				int32 NewIndex = InputChannelIndex;
				for (int32 OutputChannelIndex = 0; OutputChannelIndex < InNumOutputChannels; OutputChannelIndex++)
				{
					OutChannelMap[NewIndex] = CopyOfMap[OriginalIndex];
					OriginalIndex++;
					NewIndex += InNumInputChannels;
				}
			}
		}
	}

	bool Create2DChannelMap(const FChannelMapParams& InParams, TArray<float>& OutChannelMap)
	{
		using namespace ChannelMapPrivate;

		if (!EnsureValidChannelCounts(InParams.NumInputChannels, InParams.NumOutputChannels))
		{
			return false;
		}

		bool bSuccess = false;

		// Mono and quad have special considerations for upmixing.
		if (InParams.NumInputChannels == 1)
		{
			bSuccess = Create2DMonoInputChannelMap(InParams.NumOutputChannels, InParams.MonoUpmixMethod, InParams.bIsCenterChannelOnly, OutChannelMap);
		}
		else if (InParams.NumInputChannels == 4)
		{
			bSuccess = Create2DQuadInputChannelMap(InParams.NumOutputChannels, OutChannelMap);
		}
		else
		{
			bSuccess = Create2DGenericInputChannelMap(InParams.NumInputChannels, InParams.NumOutputChannels, OutChannelMap);
		}

		// By default, functions create channel map in "OutputMajorOrder". Check
		// if it needs to be transposed. 
		if (bSuccess && (InParams.Order == EChannelMapOrder::InputMajorOrder))
		{
			TransposeOutputMajorOrderToInputMajorOrder(InParams.NumInputChannels, InParams.NumOutputChannels, OutChannelMap);
		}

		return bSuccess;
	}

	bool CreateVorbis2DChannelMap(int32 InNumOutputChannels, EChannelMapOrder InOrder, TArray<float>& OutVorbisChannelMap)
	{
		using namespace ChannelMapPrivate;

		if (!ensureMsgf((InNumOutputChannels > 0) && (InNumOutputChannels <= ChannelMapMaxNumChannels), TEXT("Invalid number of output channels: %d"), InNumOutputChannels))
		{
			return false;
		}

		// Get the matrix for the channel map index
		const int32 OutputChannelMapIndex = InNumOutputChannels - 1;
		const float* VorbisMatrix = VorbisChannelMaps[OutputChannelMapIndex];
		check(nullptr != VorbisMatrix);

		// Build it by looping over the vorbis ordered 5.1 input channels
		for (int32 InputChannel = 0; InputChannel < ChannelMapVorbisNumChannels; ++InputChannel)
		{
			for (int32 OutputChannel = 0; OutputChannel < InNumOutputChannels; ++OutputChannel)
			{
				const int32 Index = OutputChannel * ChannelMapVorbisNumChannels + InputChannel;
				OutVorbisChannelMap.Add(VorbisMatrix[Index]);
			}
		}

		// By default, channel map is in "OutputMajorOrder". Check if it needs to be transposed. 
		if (InOrder == EChannelMapOrder::InputMajorOrder)
		{
			TransposeOutputMajorOrderToInputMajorOrder(ChannelMapVorbisNumChannels, InNumOutputChannels, OutVorbisChannelMap);
		}

		return true;
	}
} // namespace

namespace Atom
{
	static FCriticalSection ChannelMapCacheLock;
	// Make a channel map cache
	static TArray<TArray<float>> ChannelMapCache;
	static TArray<TArray<float>> VorbisChannelMapCache;

	static int32 GetChannelMapCacheID(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly)
	{
		if (ensure(NumSourceChannels > 0) &&
			ensure(NumOutputChannels > 0) &&
			ensure(NumSourceChannels <= ATOM_MIXER_MAX_OUTPUT_CHANNELS) &&
			ensure(NumOutputChannels <= ATOM_MIXER_MAX_OUTPUT_CHANNELS))
		{
			int32 Index = (NumSourceChannels - 1) + ATOM_MIXER_MAX_OUTPUT_CHANNELS * (NumOutputChannels - 1);
			if (bIsCenterChannelOnly)
			{
				Index += ATOM_MIXER_MAX_OUTPUT_CHANNELS * ATOM_MIXER_MAX_OUTPUT_CHANNELS;
			}
			return Index;
		}
		return 0;
	}
} // namespace

void FAtomRuntime::CacheChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly)
{
	if (NumSourceChannels <= 0 ||
		NumOutputChannels <= 0 ||
		NumSourceChannels > ATOM_MIXER_MAX_OUTPUT_CHANNELS ||
		NumOutputChannels > ATOM_MIXER_MAX_OUTPUT_CHANNELS
		)
	{
		return;
	}
	// Generate the unique cache ID for the channel count configuration
	const int32 CacheID = Atom::GetChannelMapCacheID(NumSourceChannels, NumOutputChannels, bIsCenterChannelOnly);

	// Setup parameters for generating channel maps. 
	Atom::FChannelMapParams Params;
	Params.NumInputChannels = NumSourceChannels;
	Params.NumOutputChannels = NumOutputChannels;
	Params.Order = Atom::EChannelMapOrder::OutputMajorOrder; // Downmix code expects OutputMajorOrder
	Params.bIsCenterChannelOnly = bIsCenterChannelOnly;

	switch (MonoChannelUpmixMethod)
	{
	case EAtomMonoChannelUpmixMethod::Linear:
		Params.MonoUpmixMethod = Atom::EChannelMapMonoUpmixMethod::Linear;
		break;

	case EAtomMonoChannelUpmixMethod::EqualPower:
		Params.MonoUpmixMethod = Atom::EChannelMapMonoUpmixMethod::EqualPower;
		break;

	case EAtomMonoChannelUpmixMethod::FullVolume:
		Params.MonoUpmixMethod = Atom::EChannelMapMonoUpmixMethod::FullVolume;
		break;

	default:
		Params.MonoUpmixMethod = Atom::EChannelMapMonoUpmixMethod::EqualPower;
		checkNoEntry();
	}

	FScopeLock Lock(&Atom::ChannelMapCacheLock);
	bool bSuccess = Create2DChannelMap(Params, Atom::ChannelMapCache[CacheID]);
	check(bSuccess);
}

void FAtomRuntime::InitializeChannelMaps()
{
	FScopeLock Lock(&Atom::ChannelMapCacheLock);
	// If we haven't yet created the static channel map cache
	if (!Atom::ChannelMapCache.Num())
	{
		// Make a matrix big enough for every possible configuration, double it to account for center channel only 
		Atom::ChannelMapCache.AddZeroed(ATOM_MIXER_MAX_OUTPUT_CHANNELS * ATOM_MIXER_MAX_OUTPUT_CHANNELS * 2);

		// Create a vorbis channel map cache
		Atom::VorbisChannelMapCache.AddZeroed(ATOM_MIXER_MAX_OUTPUT_CHANNELS);

		// Loop through all input to output channel map configurations and cache them
		for (int32 OutputChannelCount = 1; OutputChannelCount < ATOM_MIXER_MAX_OUTPUT_CHANNELS + 1; ++OutputChannelCount)
		{
			for (int32 InputChannelCount = 1; InputChannelCount < ATOM_MIXER_MAX_OUTPUT_CHANNELS + 1; ++InputChannelCount)
			{
				// Cache non-vorbis channel maps
				CacheChannelMap(InputChannelCount, OutputChannelCount, true /* bIsCenterChannelOnly */);
				CacheChannelMap(InputChannelCount, OutputChannelCount, false /* bIsCenterChannelOnly */);
			}
			// Cache vorbis channel maps. 
			bool bSuccess = Atom::CreateVorbis2DChannelMap(OutputChannelCount, Atom::EChannelMapOrder::OutputMajorOrder, Atom::VorbisChannelMapCache[OutputChannelCount - 1]);
			check(bSuccess);
		}
	}
}

void FAtomRuntime::InitializeChannelAzimuthMap(const int32 NumChannels)
{
	// Initialize and cache 2D channel maps
	InitializeChannelMaps();

	// Now setup the hard-coded values
	if (NumChannels == 2)
	{
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontLeft] = { Atom::EMixerSpeaker::FrontLeft, 270 };
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontRight] = { Atom::EMixerSpeaker::FrontRight, 90 };
	}
	else
	{
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontLeft] = { Atom::EMixerSpeaker::FrontLeft, 330 };
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontRight] = { Atom::EMixerSpeaker::FrontRight, 30 };
	}

	if (bAllowCenterChannel3DPanning)
	{
		// Allow center channel for azimuth computations
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontCenter] = { Atom::EMixerSpeaker::FrontCenter, 0 };
	}
	else
	{
		// Ignore front center for azimuth computations. 
		DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::FrontCenter] = { Atom::EMixerSpeaker::FrontCenter, INDEX_NONE };
	}

	// Always ignore low frequency channel for azimuth computations. 
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::LowFrequency] = { Atom::EMixerSpeaker::LowFrequency, INDEX_NONE };

	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::SurroundBackLeft] = { Atom::EMixerSpeaker::SurroundBackLeft, 210 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::SurroundBackRight] = { Atom::EMixerSpeaker::SurroundBackRight, 150 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::SurroundLeft] = { Atom::EMixerSpeaker::SurroundLeft, 250 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::SurroundRight] = { Atom::EMixerSpeaker::SurroundRight, 110 };

	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::TopFrontLeft] = { Atom::EMixerSpeaker::TopFrontLeft, 330, 45 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::TopFrontRight] = { Atom::EMixerSpeaker::TopFrontRight, 30, 45 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::TopRearLeft] = { Atom::EMixerSpeaker::TopRearLeft, 250, 135 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::TopRearRight] = { Atom::EMixerSpeaker::TopRearRight, 110, 135 };

	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::BottomFrontLeft] = { Atom::EMixerSpeaker::BottomFrontLeft, 330, -45 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::BottomFrontRight] = { Atom::EMixerSpeaker::BottomFrontRight, 30, -45 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::BottomRearLeft] = { Atom::EMixerSpeaker::BottomRearLeft, 250, -135 };
	DefaultChannelAzimuthPositions[(int32)Atom::EMixerSpeaker::BottomRearRight] = { Atom::EMixerSpeaker::BottomRearRight, 110, -135 };

	// Check any engine ini overrides for these default positions
	if (NumChannels != 2)
	{
		int32 AzimuthPositionOverride = 0;
		for (int32 ChannelOverrideIndex = 0; ChannelOverrideIndex < Atom::MaxSupportedSpeakers; ++ChannelOverrideIndex)
		{
			Atom::EMixerSpeaker MixerChannelType = (Atom::EMixerSpeaker)ChannelOverrideIndex;

			// Don't allow overriding the center channel if its not allowed to spatialize.
			if (MixerChannelType != Atom::EMixerSpeaker::FrontCenter || bAllowCenterChannel3DPanning)
			{
				const TCHAR* ChannelName = Atom::ToString(MixerChannelType);
				if (GConfig->GetInt(TEXT("AtomChannelAzimuthMap"), ChannelName, AzimuthPositionOverride, GEngineIni))
				{
					if (AzimuthPositionOverride >= 0 && AzimuthPositionOverride < 360)
					{
						// Make sure no channels have this azimuth angle first, otherwise we'll get some bad math later
						bool bIsUnique = true;
						for (int32 ExistingChannelIndex = 0; ExistingChannelIndex < Atom::MaxSupportedSpeakers; ++ExistingChannelIndex)
						{
							if (DefaultChannelAzimuthPositions[ExistingChannelIndex].Azimuth == AzimuthPositionOverride)
							{
								bIsUnique = false;

								// If the override is setting the same value as our default, don't print a warning
								if (ExistingChannelIndex != ChannelOverrideIndex)
								{
									const TCHAR* ExistingChannelName = Atom::ToString((Atom::EMixerSpeaker)ExistingChannelIndex);
									UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Azimuth value '%d' for audio mixer channel '%s' is already used by '%s'. Azimuth values must be unique."),
										AzimuthPositionOverride, ChannelName, ExistingChannelName);
								}
								break;
							}
						}

						if (bIsUnique)
						{
							DefaultChannelAzimuthPositions[(int32)MixerChannelType].Azimuth = AzimuthPositionOverride;
						}
					}
					else
					{
						UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Azimuth value, %d, for audio mixer channel %s out of range. Must be [0, 360)."), AzimuthPositionOverride, ChannelName);
					}
				}
			}
		}
	}

	// Sort the current mapping by azimuth
	struct FCompareByAzimuth
	{
		FORCEINLINE bool operator()(const Atom::FChannelPositionInfo& A, const Atom::FChannelPositionInfo& B) const
		{
			return A.Azimuth < B.Azimuth;
		}
	};

	// Build a array of azimuth positions of only the current audio device's output channels
	RuntimeChannelAzimuthPositions.Reset();

	// Setup the default channel azimuth positions
	TArray<Atom::FChannelPositionInfo> DevicePositions;
	for (Atom::EMixerSpeaker Channel : PlatformInfo.OutputChannelArray)
	{
		// Only track non-LFE and non-Center channel azimuths for use with 3d channel mappings
		if (Channel != Atom::EMixerSpeaker::LowFrequency && DefaultChannelAzimuthPositions[(int32)Channel].Azimuth >= 0)
		{
			RuntimeChannelAzimuthPositions.Add(DefaultChannelAzimuthPositions[(int32)Channel]);
		}
	}
	RuntimeChannelAzimuthPositions.Sort(FCompareByAzimuth());
}

const TArray<Atom::EMixerSpeaker>& FAtomRuntime::GetChannelArray() const
{
	return PlatformInfo.OutputChannelArray;
}

const Atom::FChannelPositionInfo* FAtomRuntime::GetDefaultChannelPositions() const
{
	return DefaultChannelAzimuthPositions;
}

void FAtomRuntime::Get2DChannelMap(bool bIsVorbis, const int32 NumSourceChannels, const bool bIsCenterChannelOnly, Atom::FAlignedFloatBuffer& OutChannelMap) const
{
	Get2DChannelMap(bIsVorbis, NumSourceChannels, GetRuntimeNumOutputChannels(), bIsCenterChannelOnly, OutChannelMap);
}

void FAtomRuntime::Get2DChannelMap(bool bIsVorbis, const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly, Atom::FAlignedFloatBuffer& OutChannelMap)
{
	if (NumSourceChannels <= 0 ||
		NumOutputChannels <= 0 ||
		NumSourceChannels > ATOM_MIXER_MAX_OUTPUT_CHANNELS ||
		NumOutputChannels > ATOM_MIXER_MAX_OUTPUT_CHANNELS
		)
	{
		// Return a zero'd channel map buffer in the case of an unsupported channel configuration
		OutChannelMap.AddZeroed(ATOM_MIXER_MAX_OUTPUT_CHANNELS * ATOM_MIXER_MAX_OUTPUT_CHANNELS);

#if !NO_LOGGING			
		// Anti-Spam warning.
		static uint64 TimeOfLastLogMsgInCycles = 0;
		constexpr double MinTimeBetweenWarningsInMs = 5000.f; // 5 Secs.
		double ElapsedTimeInMs = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - TimeOfLastLogMsgInCycles);

		if (ElapsedTimeInMs > MinTimeBetweenWarningsInMs)
		{
			TimeOfLastLogMsgInCycles = FPlatformTime::Cycles64();
			UE_LOG(LogCriWareAtomMixer, Warning, TEXT("Unsupported source channel (%d) count or output channels (%d)"), NumSourceChannels, NumOutputChannels);
		}
#endif //!NO_LOGGING

		// Bail.
		return;
	}

	// 5.1 Vorbis files have a non-standard channel order so pick a channel map from the 5.1 vorbis channel maps based on the output channels
	if (bIsVorbis && NumSourceChannels == 6)
	{
		FScopeLock Lock(&Atom::ChannelMapCacheLock);
		OutChannelMap = Atom::VorbisChannelMapCache[NumOutputChannels - 1];
	}
	else
	{
		const int32 CacheID = Atom::GetChannelMapCacheID(NumSourceChannels, NumOutputChannels, bIsCenterChannelOnly);
		FScopeLock Lock(&Atom::ChannelMapCacheLock);
		OutChannelMap = Atom::ChannelMapCache[CacheID];
	}
}
