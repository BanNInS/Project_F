
#include "SAtomEffectPresetWidget.h"

#include "CoreMinimal.h"
#include "Components/ComboBoxString.h"
#include "Widgets/SNullWidget.h"

#include "Atom/Atom.h"
#include "Atom/AtomBlueprintLibrary.h"
#include "Atom/AtomBusEffectPreset.h"

#include "SAtomRadialSlider.h"

#define LOCTEXT_NAMESPACE "AtomEffectPresetWidget"

using Utility = UAtomUtilitiesBlueprintLibrary;

namespace AtomWidgets
{
	static FEffectPresetDesc GetBandpassFilterPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Bandpass");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
			{
				{ LOCTEXT("BandpassFilterGroupName", "Bandpass Filter"), "Low Cutoff", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} },
				{ LOCTEXT("BandpassFilterGroupName", "Bandpass Filter"), "High Cutoff", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} }
			});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetBinauralPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/HeadphoneVirtualSurround"); // (aka Binaural)
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
			{
				{ LOCTEXT("BinauralGroupName", "Binaural"), "Gain", EEffectPresetParameterType::Linear, "dB", {-12.0f, 12.0f}  },
				{ LOCTEXT("BinauralGroupName", "Binaural"), "Balance", EEffectPresetParameterType::Linear, "dB", {-12.0f, 12.0f} }
			});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetBiquadFilterPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Biquad");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
			{
				{ LOCTEXT("BiquadFilterGrouopName", "Biquad Filter"), "Filter Type", EEffectPresetParameterType::Enum, "LowPass|HighPass|Notch|LowShelf|HighShelf|Peaking" },
				{ LOCTEXT("BiquadFilterGrouopName", "Biquad Filter"), "Cutoff", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} },
				{ LOCTEXT("BiquadFilterGrouopName", "Biquad Filter"), "Q", EEffectPresetParameterType::Linear, "", {0.1f, 10.0f}, 5.0f  },
				{ LOCTEXT("BiquadFilterGrouopName", "Biquad Filter"), "Gain", EEffectPresetParameterType::Linear, "dB", {0.0f, 5.0f} },
			});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetBitCrusherPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/BitCrusher");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Drive", EEffectPresetParameterType::Linear, "dB", {0.0f, 48.0f} },
			{ FText::GetEmpty(), "Bit Depth", EEffectPresetParameterType::Integer, "bits", {1.0f, 24.0f} },
			{ FText::GetEmpty(), "Down Sampling", EEffectPresetParameterType::Linear, "", {1.0f, 100.0f} },
			{ FText::GetEmpty(), "Dry Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Gain", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 48.0f} }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetChorusPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Chorus");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Delay Time", EEffectPresetParameterType::Linear, "ms", {1.0f, 100.0f} },
			{ FText::GetEmpty(), "Depth", EEffectPresetParameterType::Linear, "", {0.0f, 1.0f}},
			{ FText::GetEmpty(), "Rate", EEffectPresetParameterType::Frequency, "Hz", {0.01f, 100.0f} },
			{ FText::GetEmpty(), "Feedback", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Dry Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wer Mix 1", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Mix 2", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Mix 3", EEffectPresetParameterType::Linear }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetCompressorPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Compressor");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
			{
				{ FText::GetEmpty(), "Threshold", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 0.0f}, 1.0f, true }, // presure value in Atom
				{ FText::GetEmpty(), "Ratio", EEffectPresetParameterType::Linear, "x", {1.0f, 10000.0f}, 5.0f },
				{ FText::GetEmpty(), "Attack Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 3000.0f} },
				{ FText::GetEmpty(), "Release Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 50000.0f} },
				{ FText::GetEmpty(), "Gain", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 48.0f}, 1.0f, true}, // presure
				{ FText::GetEmpty(), "Surround Link", EEffectPresetParameterType::Normalize, "%", {0.0f, 100.0f} },
				{ LOCTEXT("SplitEQGroupName", "Split EQ"), "Filter Type", EEffectPresetParameterType::Enum, "None|LowPass|HighPass|BandPass|Notch" },
				{ LOCTEXT("SplitEQGroupName", "Split EQ"), "Frequency", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} },
				{ LOCTEXT("SplitEQGroupName", "Split EQ"), "Q", EEffectPresetParameterType::Linear, "", {0.1f, 10.0f}, 5.0f},
			});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetDelayPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Delay");
		PresetDesc.NumElementsPerLine = 1;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Delay Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 1000.0f} }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetDistortionPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Distortion");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Drive", EEffectPresetParameterType::Linear, "dB", {0.0f, 48.0f} },
			{ FText::GetEmpty(), "Dry Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Output Gain", EEffectPresetParameterType::Linear, "dB", {-96.0f, 48.0f}, 1.0f, true } // presure value in Atom
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetEchoPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Echo");
		PresetDesc.NumElementsPerLine = 2;
		PresetDesc.ParameterDescs.Append(
		{
			{ LOCTEXT("EchoGroupName", "Echo"), "Delay Time", EEffectPresetParameterType::Linear, "ms", {1.0f, 1000.0f}},
			{ LOCTEXT("EchoGroupName", "Echo"), "Feedback", EEffectPresetParameterType::Linear, "", {-0.99f, 0.99f}},
			{ LOCTEXT("ConfigGroupName", "Config"), "Maximum Delay Time", EEffectPresetParameterType::Linear, "ms", {0.1f, 1000.0f} }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetFlangerPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Flanger");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Delay Time", EEffectPresetParameterType::Linear, "ms", {1.0f, 100.0f}},
			{ FText::GetEmpty(), "Depth", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Rate", EEffectPresetParameterType::Frequency, "Hz", {0.01f, 100.0f} },
			{ FText::GetEmpty(), "Feedback", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Dry Mix", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Mix", EEffectPresetParameterType::Linear }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetI3DL2ReverbPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/I3DL2Reverb");
		PresetDesc.NumElementsPerLine = 6;
		PresetDesc.ParameterDescs.Append(
		{
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Room", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f}, // milibels value in atom
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Room HF", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Decay Time", EEffectPresetParameterType::Linear, "ms", {0.1f, 20.0f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Decay HF Ratio", EEffectPresetParameterType::Linear, "x", {0.1f, 0.2f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Reflections", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 10.0f}, 100.0f }, // milibels
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Reflections Delay", EEffectPresetParameterType::Linear, "ms", {0.0, 0.3f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Reverb", EEffectPresetParameterType::Decibel, "dB", {-100.0, 20.0f}, 100.0f }, // milibels
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Reverb Delay", EEffectPresetParameterType::Linear, "ms", {0.0, 0.1f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Diffusion", EEffectPresetParameterType::Linear, "%", {0.0, 100.0f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Density", EEffectPresetParameterType::Linear, "%", {0.0, 100.0f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "HFReference", EEffectPresetParameterType::Frequency, "hZ", {20.0, 20000.0f} },
			{ LOCTEXT("InputGroupName", "Input"), "Front Input", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("InputGroupName", "Input"), "Rear Input", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("InputGroupName", "Input"), "Center Input", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("OutputGroupName", "Output"), "Front Output", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("OutputGroupName", "Output"), "Rear Output", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f }, // milibels
			{ LOCTEXT("OutputGroupName", "Output"), "Center Output", EEffectPresetParameterType::Decibel, "dB", {-100.0f, 0.0f}, 100.0f } // milibels
		});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetI3DL2Reverb2PresetDesc()
	{
		FEffectPresetDesc PresetDesc = GetI3DL2ReverbPresetDesc();
		PresetDesc.Name = TEXT("CRIWARE/I3DL2Reverb2");

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetIRReverbPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/IRReverb");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Dry Gain", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Wet Gain", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Reverb Gain", EEffectPresetParameterType::Linear },
			{ FText::GetEmpty(), "Input Separation Ratio", EEffectPresetParameterType::Linear },
			{ LOCTEXT("InputGroupName", "Input"), "Front Input Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("InputGroupName", "Input"), "Center Input Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("InputGroupName", "Input"), "Surround Input Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("InputGroupName", "Input"), "Surround Back Input Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("OutputGroupName", "Output"), "Front Output Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("OutputGroupName", "Output"), "Center Output Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("OutputGroupName", "Output"), "Surround Output Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("OutputGroupName", "Output"), "Surround Back Output Gain", EEffectPresetParameterType::Linear },
			{ LOCTEXT("ConfigGroupName", "Config"), "IRIndex", EEffectPresetParameterType::Integer, "Index", {0, 128} }
		});

		return MoveTemp(PresetDesc);
	}

	//
	static FEffectPresetDesc GetLimiterPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Limiter");
		PresetDesc.NumElementsPerLine = 3;
		PresetDesc.ParameterDescs.Append(
			{
				{ FText::GetEmpty(), "Threshold", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 0.0f}, 1.0f, true }, // presure value in Atom
				{ FText::GetEmpty(), "Attack Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 3000.0f} },
				{ FText::GetEmpty(), "Release Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 50000.0f} },
				{ FText::GetEmpty(), "Gain", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 48.0f}, 1.0f, true }, // presure
				{ FText::GetEmpty(), "Surround Link", EEffectPresetParameterType::Normalize, "%", {0.0f, 100.0f} },
				{ FText::GetEmpty(), "Type", EEffectPresetParameterType::Enum, "Peak|RMS" }
			});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetMatrixPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = "CRIWARE/Matrix";
		PresetDesc.NumElementsPerLine = 8;
		PresetDesc.ParameterDescs.Append(
			{
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFLGroupName", "Front Left"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFRGroupName", "Front Right"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixFCGroupName", "Front Center"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixLFEGroupName", "LFE"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSLGroupName", "Side Left"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixSRGroupName", "Side Right"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRLGroupName", "Rear Left"), "Rear Right", EEffectPresetParameterType::Linear },

				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Front Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Front Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Front Center", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "LFE", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Side Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Side Right", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Rear Left", EEffectPresetParameterType::Linear },
				{ LOCTEXT("MatrixRRGroupName", "Rear Right"), "Rear Right", EEffectPresetParameterType::Linear }
			});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetMultiTapPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/MultiTapDelay");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ LOCTEXT("MultiTapDelayTap1GroupName", "Tap 1"), "Delay Time", EEffectPresetParameterType::Linear, TEXT("ms"), {1.0f, 10000.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap1GroupName", "Tap 1"), "Level", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap1GroupName", "Tap 1"), "Azimuth", EEffectPresetParameterType::Linear, TEXT("属"), {-180.0f, 180.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap1GroupName", "Tap 1"), "Feedback", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },

			{ LOCTEXT("MultiTapDelayTap2GroupName", "Tap 2"), "Delay Time", EEffectPresetParameterType::Linear, TEXT("ms"), {1.0f, 10000.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap2GroupName", "Tap 2"), "Level", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap2GroupName", "Tap 2"), "Azimuth", EEffectPresetParameterType::Linear, TEXT("属"), {-180.0f, 180.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap2GroupName", "Tap 2"), "Feedback", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },

			{ LOCTEXT("MultiTapDelayTap3GroupName", "Tap 3"), "Delay Time", EEffectPresetParameterType::Linear, TEXT("ms"), {1.0f, 10000.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap3GroupName", "Tap 3"), "Level", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap3GroupName", "Tap 3"), "Azimuth", EEffectPresetParameterType::Linear, TEXT("属"), {-180.0f, 180.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap3GroupName", "Tap 3"), "Feedback", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },

			{ LOCTEXT("MultiTapDelayTap4GroupName", "Tap 4"), "Delay Time", EEffectPresetParameterType::Linear, TEXT("ms"), {1.0f, 10000.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap4GroupName", "Tap 4"), "Level", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap4GroupName", "Tap 4"), "Azimuth", EEffectPresetParameterType::Linear, TEXT("属"), {-180.0f, 180.0f}, 1.0f, true, 0.5f },
			{ LOCTEXT("MultiTapDelayTap4GroupName", "Tap 4"), "Feedback", EEffectPresetParameterType::Linear, TEXT(""), {0.0f, 1.0f}, 1.0f, true, 0.5f }
		});
		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetPhaserPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Phaser");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
		{
			{ LOCTEXT("PhaserGroupName", "Phaser"), "Stages", EEffectPresetParameterType::Integer, "stages", {4, 12}, 2 },
			{ LOCTEXT("PhaserGroupName", "Phaser"), "Depth", EEffectPresetParameterType::Linear },
			{ LOCTEXT("PhaserGroupName", "Phaser"), "Rate", EEffectPresetParameterType::Frequency, "Hz", {0.01f, 100.0f} },
			{ LOCTEXT("PhaserGroupName", "Phaser"), "Feedback", EEffectPresetParameterType::Linear },
			{ LOCTEXT("PhaserGroupName", "Phaser"), "WetMix", EEffectPresetParameterType::Linear },
			{ LOCTEXT("PhaserGroupName", "Phaser"), "DryMix", EEffectPresetParameterType::Linear }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetPitchShifterPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/PitchShifter");
		PresetDesc.NumElementsPerLine = 3;
		PresetDesc.ParameterDescs.Append(
		{
			{ FText::GetEmpty(), "Pitch Shift", EEffectPresetParameterType::Semitone, "Semitones", { -24.0f, 24.0f } },
			{ FText::GetEmpty(), "Formant Shift", EEffectPresetParameterType::Semitone, "Semitones", { -24.0f, 24.0f } },
			{ FText::GetEmpty(), "Shift Mode", EEffectPresetParameterType::Enum, "Music|Vocal|Sound Effect|Speech" },
			{ FText::GetEmpty(), "Data Pitch Frequency", EEffectPresetParameterType::Hidden, "Hz", { 24.0f, 24000.0f } }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetReverbPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/Reverb");
		PresetDesc.NumElementsPerLine = 3;
		PresetDesc.ParameterDescs.Append(
		{
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Reverb Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 1.0f}},
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Room Size", EEffectPresetParameterType::Linear, "meters", {0.0f, 1.0f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Pre DelayTime", EEffectPresetParameterType::Linear, "ms", {0.0f, 1.0f}},
			{ LOCTEXT("ReverbGroupName", "Reverb"), "Low Cutoff", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} },
			{ LOCTEXT("ReverbGroupName", "Reverb"), "High Cutoff", EEffectPresetParameterType::Frequency, "Hz", {24.0f, 24000.0f} },
			{ LOCTEXT("ConfigGroupName", "Config"), "Maximum Room Size", EEffectPresetParameterType::Linear, "meters", {0.0f, 1.0f} },
			{ LOCTEXT("ConfigGroupName", "Config"), "Maximum PreDelay Time", EEffectPresetParameterType::Linear, "ms", {0.0f, 1.0f} }
		});

		return MoveTemp(PresetDesc);
	}

	static FEffectPresetDesc GetSurrounderWithEQPresetDesc()
	{
		FEffectPresetDesc PresetDesc{};
		PresetDesc.Name = TEXT("CRIWARE/SurrounderWithEqualizer");
		PresetDesc.NumElementsPerLine = 4;
		PresetDesc.ParameterDescs.Append(
			{
				{ FText::GetEmpty(), "Surrounder Mode", EEffectPresetParameterType::Enum, "Straight|Cross|Matrix" },
				{ FText::GetEmpty(), "DelayTime", EEffectPresetParameterType::Linear, "ms", {0.0f, 500.0f} },
				{ FText::GetEmpty(), "Gain", EEffectPresetParameterType::Decibel, "dB", {-96.0f, 48.0f}, 1.0f, true } // presure value in Atom
			});
		return MoveTemp(PresetDesc);
	}
	
	TArray<FEffectPresetDesc> SAtomEffectPresetWidget::PresetDescs = {
		GetBandpassFilterPresetDesc(),
		GetBinauralPresetDesc(),
		GetBiquadFilterPresetDesc(),
		GetBitCrusherPresetDesc(),
		GetChorusPresetDesc(),
		GetCompressorPresetDesc(),
		GetDelayPresetDesc(),
		GetDistortionPresetDesc(),
		GetEchoPresetDesc(),
		GetFlangerPresetDesc(),
		GetI3DL2ReverbPresetDesc(),
		GetI3DL2Reverb2PresetDesc(),
		GetIRReverbPresetDesc(),
		GetLimiterPresetDesc(),
		GetMatrixPresetDesc(),
		GetMultiTapPresetDesc(),
		GetPhaserPresetDesc(),
		GetPitchShifterPresetDesc(),
		GetReverbPresetDesc(),
		GetSurrounderWithEQPresetDesc()
	};

	void SAtomEffectPresetWidget::Construct(const FArguments& InArgs)
	{
		Preset = InArgs._Preset;
		if (Preset == nullptr)
		{
			return;
		}

		FName PresetName = Preset->GetEffectName();

		for (int Index = 0; Index < PresetDescs.Num(); ++Index)
		{
			if (PresetName == PresetDescs[Index].Name)
			{
				// we found the used preset index
				PresetIndex = Index;
				break;
			}
		}

		check(PresetDescs.IsValidIndex(PresetIndex));
		if (!PresetDescs.IsValidIndex(PresetIndex))
		{
			return;
		}

		const FEffectPresetDesc& PresetDesc = PresetDescs[PresetIndex];

		TSharedPtr<SVerticalBox> MainVerticalBox;
		ChildSlot
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SAssignNew(MainVerticalBox, SVerticalBox)
			];

		// by-pass
		MainVerticalBox->AddSlot()
			.AutoHeight()
			.Padding(10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(4.0f)
				[
					SAssignNew(BypassCheckBox, SCheckBox)
					.IsChecked(bIsBypass.CreateLambda([this]()
					{
						return Preset->IsBypassed() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					}))
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						Preset->SetBypass(NewState == ECheckBoxState::Checked);
						Preset->ApplyToDefautlsSettings(); // Copy to permanent Settings
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("BypassEffect", "Bypass Effect"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
				]
			];

		// widgets
		TSharedPtr<SScrollBox> WidgetsVerticalBox;
		TSharedPtr<SVerticalBox> GroupVerticalBox;
		TSharedPtr<SHorizontalBox> LineHorizontalBox;
		FString PreviousGroup = TEXT("_Nothing_");

		MainVerticalBox->AddSlot()
			[
				SAssignNew(WidgetsVerticalBox, SScrollBox)
			];

		for (int32 ParameterIndex = 0, CurrentLineIndex = 0; ParameterIndex < Preset->GetNumParameters(); ++ParameterIndex, ++CurrentLineIndex)
		{
			if (!PresetDesc.ParameterDescs.IsValidIndex(ParameterIndex))
			{
				break;
			}

			const FEffectPresetParameterDesc& ParameterDesc = PresetDesc.ParameterDescs[ParameterIndex];

			// group border
			if (PreviousGroup != ParameterDesc.Group.ToString())
			{
				WidgetsVerticalBox->AddSlot()
					.Padding(4.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
						.Padding(4.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(20.0f, 0.0f, 0.0f, 4.0f)
							[
								SNew(STextBlock)
								.Text(ParameterDesc.Group)
								.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SAssignNew(GroupVerticalBox, SVerticalBox)
							]
						]
					];

				PreviousGroup = ParameterDesc.Group.ToString();

				// reset line index
				CurrentLineIndex = 0;
			}

			if (CurrentLineIndex == PresetDesc.NumElementsPerLine)
			{
				CurrentLineIndex = 0;
			}

			// Create a new line if needed
			if (CurrentLineIndex == 0)
			{
				GroupVerticalBox->AddSlot()
					.AutoHeight()
					[
						SAssignNew(LineHorizontalBox, SHorizontalBox)
					];
			}

			auto GetEnumValues = [](const FString& EnumList) 
			{
				// Split enumlist into different strings
				TArray<TSharedPtr<FString>> OutStrings;
				FString Left, Right = EnumList;
				while (Right.Split(TEXT("|"), &Left, &Right))
				{
					OutStrings.Add(MakeShareable(new FString(Left)));
				}
				OutStrings.Add(MakeShareable(new FString(Right)));
				return MoveTemp(OutStrings);
			};

			if (ParameterDesc.Type == EEffectPresetParameterType::Enum)
			{
				ComboBoxSources.Add(ParameterIndex, GetEnumValues(ParameterDesc.Unit));
				auto& InitialSelection = ComboBoxSources[ParameterIndex][0];

				// todo a MyEnumSelector with text and list selection
				TSharedPtr<SComboBox<TSharedPtr<FString>>> NewComboBox;

				LineHorizontalBox->AddSlot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.FillHeight(0.2f)
						.Padding(4.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromName(ParameterDesc.Name))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
						]
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Fill)
						.FillHeight(0.8f)
						[
							SAssignNew(NewComboBox, SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&ComboBoxSources[ParameterIndex])
							.OnGenerateWidget(this, &SAtomEffectPresetWidget::GenerateComboBoxItem)
							.OnSelectionChanged(this, &SAtomEffectPresetWidget::OnSelectionChanged, ParameterIndex)
							.InitiallySelectedItem(InitialSelection)
							.Content()
							[
								SNew(STextBlock)
								.Text(this, &SAtomEffectPresetWidget::GetComboBoxContent, ParameterIndex)
							]
						]
					];

				ParameterWidgets.Add(NewComboBox);
			}
			else if (ParameterDesc.Type == EEffectPresetParameterType::Hidden)
			{
				LineHorizontalBox->AddSlot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNullWidget::NullWidget
					];

				ParameterWidgets.Add(SNullWidget::NullWidget);
			}
			else
			{
				// for decibel and frequency we use normalized range for eazy convertions
				FVector2D SliderRange = (
					ParameterDesc.Type == EEffectPresetParameterType::Decibel 
					|| ParameterDesc.Type == EEffectPresetParameterType::Frequency) ? 
					FVector2D{0.0f, 1.0f} : ParameterDesc.Range;
					
				auto NewRadialSlider = SNew(SAtomRadialSlider)
					.Name(FText::FromName(ParameterDesc.Name))
					.Value(0.0f)
					.Range(SliderRange) // Linear Curve Range of the slider
					.OnValueChanged_Lambda([this, ParameterIndex, ParameterDesc](float InSliderValue)
					{
						float AtomValue = 0.0f; // value as if from atom
						switch (ParameterDesc.Type)
						{
						case EEffectPresetParameterType::Normalize: // Range -> [0, 1]
						{
							AtomValue = FMath::GetMappedRangeValueClamped(ParameterDesc.Range, { 0.0, 1.0f }, InSliderValue);
						}
							break;
						case EEffectPresetParameterType::Linear: // Range expo -> Range
						{
							AtomValue = InSliderValue;
						}
							break;
						case EEffectPresetParameterType::Decibel: // Slider Log Linear -> Decibels/Linear
						{
							float Decibels = Utility::ConvertLinearToDecibelsSlower(InSliderValue, ParameterDesc.Range);
							AtomValue = ParameterDesc.bIsLinear ? Atom::ConvertToLinear(Decibels) : Decibels;
							AtomValue *= ParameterDesc.Exponent; // as factor
						}
							break;
						case EEffectPresetParameterType::Frequency: // Slider Log Linear Frenquency -> Frequency/Linear
						{
							float Frequency = Atom::GetLogFrequencyClamped(InSliderValue, { 0.0, 1.0f }, ParameterDesc.Range);
							AtomValue = Frequency;
						}
							break;
						case EEffectPresetParameterType::Semitone:
						{
							AtomValue = Atom::GetFrequencyMultiplier(InSliderValue);
						}
							break;
						case EEffectPresetParameterType::Integer:
						{
							int32 Value = FMath::RoundToInt(InSliderValue);
							Value -= Value % (int32)ParameterDesc.Exponent; // step
							AtomValue = (float)Value;
							// apply back the stepped value to the radial
							TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
							AtomRadial->SetValue(AtomValue);
						}
							break;
						default:
							break;
						}

						Preset->SetParameterValue(ParameterIndex, AtomValue); // generic setup as from Atom
						Preset->ApplyToDefautlsSettings(); // Copy to permanent Settings
					
						TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
						UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
					});

				LineHorizontalBox->AddSlot()
					.HAlign(HAlign_Fill)
					[
						NewRadialSlider
					];

				ParameterWidgets.Add(NewRadialSlider);
			}
		}
	}

	void SAtomEffectPresetWidget::Update()
	{
		if (Preset == nullptr)
		{
			return;
		}

		const FEffectPresetDesc& PresetDesc = PresetDescs[PresetIndex];

		// bypass
		//bIsBypass.Set(Preset->IsBypassed() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

		// widgets
		auto GetEnumValue = [this](int32 EnumIndex, int32 ParameterIndex)
		{
			const TArray<TSharedPtr<FString, ESPMode::ThreadSafe>>& Sources = ComboBoxSources[ParameterIndex];
			return Sources.IsValidIndex(EnumIndex) ? Sources[EnumIndex] : nullptr;
		};

		for (int32 ParameterIndex = 0; ParameterIndex < Preset->GetNumParameters(); ++ParameterIndex)
		{
			if (!PresetDesc.ParameterDescs.IsValidIndex(ParameterIndex))
			{
				break;
			}

			const FEffectPresetParameterDesc& ParameterDesc = PresetDesc.ParameterDescs[ParameterIndex];
			const float AtomValue = Preset->GetParameterValue(ParameterIndex);
			
			// Convert AtomValue from Preset to value for slider
			switch (ParameterDesc.Type)
			{
			case EEffectPresetParameterType::Normalize: // [0,1] -> Range
			{
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				const float SliderValue = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, ParameterDesc.Range, AtomValue);
				AtomRadial->SetValue(SliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Linear: // Range -> Range
			{
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				const float SliderValue = AtomValue;
				AtomRadial->SetValue(SliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Decibel: // Range -> Slider Linear
			{
				const float Value = AtomValue / ParameterDesc.Exponent; // as Factor
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				float Decibels = ParameterDesc.bIsLinear ? Atom::ConvertToDecibels(Value) : Value;
				float LinearSliderValue = Utility::ConvertDecibelsToLinearSlower(Decibels, ParameterDesc.Range);
				AtomRadial->SetValue(LinearSliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Frequency: // Range -> Slider Linear
			{
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				const float LinearSliderValue = Atom::GetLinearFrequencyClamped(AtomValue, { 0.0f, 1.0f }, ParameterDesc.Range);
				AtomRadial->SetValue(LinearSliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Semitone:
			{
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				const float SliderValue = Atom::GetSemitones(AtomValue);
				AtomRadial->SetValue(SliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Integer:
			{
				TSharedPtr<SAtomRadialSlider> AtomRadial = StaticCastSharedPtr<SAtomRadialSlider>(ParameterWidgets[ParameterIndex]);
				const float SliderValue = AtomValue;
				AtomRadial->SetValue(SliderValue);
				UpdateRadialSliderText(AtomRadial, ParameterDesc, AtomValue);
			}
			break;
			case EEffectPresetParameterType::Enum:
			{
				TSharedPtr<SComboBox<TSharedPtr<FString>>> MyComboBox = StaticCastSharedPtr<SComboBox<TSharedPtr<FString>>>(ParameterWidgets[ParameterIndex]);
				const int32 EnumIndex = (const int32)AtomValue;
				MyComboBox->SetSelectedItem(GetEnumValue(EnumIndex, ParameterIndex));
			}
			break;
			case EEffectPresetParameterType::Hidden:
			default:
				break;
			}
		}
	}

	void SAtomEffectPresetWidget::AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(Preset);
	}

	void SAtomEffectPresetWidget::UpdateRadialSliderText(TSharedPtr<SAtomRadialSlider> AtomRadial, const FEffectPresetParameterDesc& ParameterDesc, float AtomValue)
	{
		switch(ParameterDesc.Type)
		{
		case EEffectPresetParameterType::Normalize:
		{
			const float UeValue = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, ParameterDesc.Range, AtomValue);
			AtomRadial->SetTextValue(FText::FromString(FString::Printf(TEXT("%.3f %s"), UeValue, *ParameterDesc.Unit)));
		}
		break;
		case EEffectPresetParameterType::Semitone:
		{
			const float UeValue = Atom::GetSemitones(AtomValue);
			AtomRadial->SetTextValue(FText::FromString(FString::Printf(TEXT("%.3f %s"), UeValue, *ParameterDesc.Unit)));
		}
		break;
		case EEffectPresetParameterType::Decibel:
		{
			AtomValue /= ParameterDesc.Exponent; // as Factor
			const float UeValue = ParameterDesc.bIsLinear ? Atom::ConvertToDecibels(AtomValue) : AtomValue;
			AtomRadial->SetTextValue(FText::FromString(FString::Printf(TEXT("%.3f %s"), UeValue, *ParameterDesc.Unit)));
		}
		break;
		case EEffectPresetParameterType::Integer:
		{
			const int32 UeValue = (int32)AtomValue;
			AtomRadial->SetTextValue(FText::FromString(FString::Printf(TEXT("%d %s"), UeValue, *ParameterDesc.Unit)));
		}
		break;
		default:
		{
			const float UeValue = AtomValue;
			AtomRadial->SetTextValue(FText::FromString(FString::Printf(TEXT("%.3f %s"), UeValue, *ParameterDesc.Unit)));
		}
		}
	}

	TSharedRef<SWidget> SAtomEffectPresetWidget::GenerateComboBoxItem(TSharedPtr<FString> InOption)
	{
		if (InOption.IsValid())
		{
			return SNew(STextBlock).Text(FText::FromString(*InOption));
		}

		return SNew(STextBlock).Text(LOCTEXT("Invalid", "Invalid"));
	}

	void SAtomEffectPresetWidget::OnSelectionChanged(const TSharedPtr<FString> NewValue, ESelectInfo::Type, int32 ParameterIndex)
	{
		if (ComboBoxSources.Contains(ParameterIndex))
		{
			auto& Sources = ComboBoxSources[ParameterIndex];
			const int32 EnumIndex = Sources.IndexOfByKey(NewValue);
			float Value = (float)EnumIndex;
			Preset->SetParameterValue(ParameterIndex, Value);
			Preset->ApplyToDefautlsSettings();
		}
	}

	FText SAtomEffectPresetWidget::GetComboBoxContent(int32 ParameterIndex) const
	{
		if (ParameterWidgets.IsValidIndex(ParameterIndex))
		{
			if (auto ComboBox = StaticCastSharedPtr<SComboBox<TSharedPtr<FString>>>(ParameterWidgets[ParameterIndex]); ComboBox.IsValid())
			{
				if (auto Selected = ComboBox->GetSelectedItem().Get())
				{
					return FText::FromString(*Selected);
				}
			}
		}

		return LOCTEXT("NONE", "NONE");
	}
} // namespace

#undef LOCTEXT_NAMESPACE
