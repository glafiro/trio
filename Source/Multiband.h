
#pragma once

#include <vector>
using std::vector;

#include "Utils.h"
#include "DSPParameters.h"
#include "Filters.h"

#define DEFAULT_SR 44100.0f

float msToCoefficient(float sampleRate, float length) {
	return expf(-1.0f / lengthToSamples(sampleRate, length));
}

class Compressor
{
	float sampleRate{ DEFAULT_SR };
	float blockSize{ 0.0f };
	int nChannels{ 1 };

    float threshold{ 1.0f };
    float ratio{ 1.0f };
    float attack{ 0.0f };
    float release{ 0.0f };

	bool bypass;

	float gainReduction{ 1.0f };

	friend class MultibandCompressor;

public:

    void prepare(float sr, float bs, float ch) {
		sampleRate = sr;
		blockSize = bs;
		nChannels = ch;
	}

	void update(float _threshold, float _ratio, float _attack, float _release) {
		threshold = _threshold;
		ratio = _ratio;
		attack = msToCoefficient(sampleRate, _attack);
		release = msToCoefficient(sampleRate, _release);
	}

	float processSample(float sample) {
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

		return sample * gainReduction;
    }
};

class MultibandCompressor
{
	Compressor lowBand;
	Compressor midBand;
	Compressor highBand;

	LRFilter<float> lowMidFilter;
	LRFilter<float> midHighFilter;
	float lowMidCut{ 700.0f };
	float midHighCut{ 5000.0f };

	float sampleRate{ DEFAULT_SR };
	float blockSize { 0.0f };
	int   nChannels { 1 };

	bool bypassLow{ false };
	bool bypassMid{ false };
	bool bypassHigh{ false };

public:

	void prepare(DSPParameters<float>& params) {
		sampleRate = params["sampleRate"];
		blockSize = params["blockSize"];
		nChannels = static_cast<int>(params["nChannels"]);
		
		bypassLow = static_cast<bool>(params["bypassLow"]);

		lowBand.prepare(sampleRate, blockSize, nChannels);
		lowBand.update(params["thresholdLow"], params["ratioLow"], params["attackLow"], params["releaseLow"]);
		midBand.prepare(sampleRate, blockSize, nChannels);
		midBand.update(params["thresholdMid"], params["ratioMid"], params["attackMid"], params["releaseMid"]);
		highBand.prepare(sampleRate, blockSize, nChannels);
		highBand.update(params["thresholdHigh"], params["ratioHigh"], params["attackHigh"], params["releaseHigh"]);

		lowMidFilter.prepare(sampleRate, blockSize, nChannels);
		lowMidCut = params["lowMidCut"];
		lowMidFilter.setFrequency(lowMidCut);
		
		midHighFilter.prepare(sampleRate, blockSize, nChannels);
		midHighCut = params["midHighCut"];
		midHighFilter.setFrequency(midHighCut);

	}

	void update(DSPParameters<float>& params) {
		bypassLow  = static_cast<bool>(params["bypassLow"]);
		bypassMid  = static_cast<bool>(params["bypassMid"]);
		bypassHigh = static_cast<bool>(params["bypassHigh"]);

		lowMidCut = params["lowMidCut"];
		lowMidFilter.setFrequency(lowMidCut);

		midHighCut = params["midHighCut"];
		midHighFilter.setFrequency(midHighCut);
		
		lowBand.update(params["thresholdLow"], params["ratioLow"], params["attackLow"], params["releaseLow"]);
		midBand.update(params["thresholdMid"], params["ratioMid"], params["attackMid"], params["releaseMid"]);
		highBand.update(params["thresholdHigh"], params["ratioHigh"], params["attackHigh"], params["releaseHigh"]);
	}


	void processBlock(float** inputBuffer, int numChannels, int numSamples) {
		for (int ch = 0; ch < numChannels; ++ch) {
			for (int s = 0; s < numSamples; ++s) {

				auto sample = inputBuffer[ch][s]; 
				
				float lowBandFiltered, midBandFiltered, highBandFiltered;
				float lowBandCompressed, midBandCompressed, highBandCompressed;
				
				lowMidFilter.processSample(ch, sample, lowBandFiltered, midBandFiltered);
				midHighFilter.processSample(ch, midBandFiltered, midBandFiltered, highBandFiltered);
				
				lowBandCompressed  = bypassLow  ? 0.0f : lowBand.processSample(lowBandFiltered);
				midBandCompressed  = bypassMid  ? 0.0f : midBand.processSample(midBandFiltered);
				highBandCompressed = bypassHigh ? 0.0f : highBand.processSample(highBandFiltered);
				
				inputBuffer[ch][s] = lowBandCompressed + midBandCompressed + highBandCompressed;
			}
		}
	}
};

#undef DEFAULT_SR