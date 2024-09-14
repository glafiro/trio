/*
  ==============================================================================

    Multiband.h
    Created: 13 Sep 2024 1:14:32pm
    Author:  dglaf

  ==============================================================================
*/

#pragma once

#include "Utils.h"
#include "DSPParameters.h"

class Compressor
{
	float sampleRate{ 44100.0f };
	float blockSize{ 0.0f };
	int nChannels{ 1 };

    float threshold{ 1.0f };
    float ratio{ 1.0f };
    float attack{ 0.0f };
    float release{ 0.0f };

	bool bypass;

	float gainReduction{ 1.0f };

public:

    void prepare(DSPParameters<float>& params) {
		readParameters(params);
	}

	void update(DSPParameters<float>& params) {
		readParameters(params);
	}

	void readParameters(DSPParameters<float>& params) {
		sampleRate = params["sampleRate"];
		blockSize = params["blockSize"];
		nChannels = static_cast<int>(params["nChannels"]);

		ratio = params["ratioLow"];
		threshold = params["thresholdLow"];
		attack = msToCoefficient(params["attackLow"]);
		release = msToCoefficient(params["releaseLow"]);
		bypass = static_cast<bool>(params["bypassLow"]);
	}

	void processBlock(float* const* inputBuffer, int numChannels, int numSamples) {
		for (int ch = 0; ch < numChannels; ++ch) {
			for (auto s = 0; s < numSamples; ++s) {
				auto sample = inputBuffer[ch][s];

				if (bypass) {
					inputBuffer[ch][s] = sample;
					return;
				}

				// Compression
				auto sampleInDb = linearToDb(sample);
				
				float target{ 1.0f };
				if (sampleInDb > threshold) {
					auto excess = sampleInDb - threshold;
					auto compressed = threshold + excess / ratio;
					target = dbToLinear(compressed - sampleInDb);
				}

				if (target < gainReduction) {
					gainReduction = attack * gainReduction + (1.0f - attack) * target;
				}
				else if (target > gainReduction) {
					gainReduction = release * gainReduction + (1.0f - release) * target;
				}

				inputBuffer[ch][s] = sample * gainReduction;

			}
		}
	}

private:
	float msToCoefficient(float length) {
		return expf(-1.0f / lengthToSamples(sampleRate, length));
	}
};