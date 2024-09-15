
#pragma once

#include <vector>
using std::vector;

#include "Utils.h"
#include "DSPParameters.h"
#include "Filters.h"
#include "FilteredParameter.h"

#define DEFAULT_SR 44100.0f

float msToCoefficient(float sampleRate, float length) {
	return expf(-1.0f / lengthToSamples(sampleRate, length));
}

class Compressor
{
	float sampleRate{ DEFAULT_SR };
	float blockSize{ 0.0f };
	int nChannels{ 1 };

	FilteredParameter threshold;
	FilteredParameter ratio;
	FilteredParameter attack;
	FilteredParameter release;
	FilteredParameter inputGain;
	FilteredParameter outputGain;

	bool bypass;

	float gainReduction{ 1.0f };

public:

    void prepare(float sr, float bs, float ch) {
		sampleRate = sr;
		blockSize = bs;
		nChannels = ch;

		threshold.prepare(sampleRate, 0.0f);
		ratio.prepare(sampleRate, 1.0f);
		attack.prepare(sampleRate, 0.0f);
		release.prepare(sampleRate, 0.0f);
		inputGain.prepare(sampleRate, 1.0f);
		outputGain.prepare(sampleRate, 1.0f);
	}

	void update(float _threshold, float _ratio, float _attack, float _release, float _in, float _out) {
		threshold.setValue(_threshold);
		ratio.setValue(_ratio);
		attack.setValue(msToCoefficient(sampleRate, _attack));
		release.setValue(msToCoefficient(sampleRate, _release));
		inputGain.setValue(dbToLinear(_in));
		outputGain.setValue(dbToLinear(_out));
	}

	float processSample(float sample) {
		auto inputSample = sample *= inputGain.next();
		auto sampleInDb = linearToDb(sample);

		auto currentThreshold = threshold.next();
		auto currentRatio = ratio.next();
		auto currentAtk = attack.next();
		auto currentRls = release.next();
				
		float target{ 1.0f };
		if (sampleInDb > currentThreshold) {
			auto excess = sampleInDb - currentThreshold;
			auto compressed = currentThreshold + excess / currentRatio;
			target = dbToLinear(compressed - sampleInDb);
		}

		if (target < gainReduction) {
			gainReduction = currentAtk * gainReduction + (1.0f - currentAtk) * target;
		}
		else if (target > gainReduction) {
			gainReduction = currentRls * gainReduction + (1.0f - currentRls) * target;
		}

		return inputSample * gainReduction * outputGain.next();
    }
};

class MultibandCompressor
{
	Compressor lowBand;
	Compressor midBand;
	Compressor highBand;

	LRFilter<float> lowMidFilter;
	LRFilter<float> midHighFilter;
	FilteredParameter lowMidCut;
	FilteredParameter midHighCut;

	float sampleRate{ DEFAULT_SR };
	float blockSize { 0.0f };
	int   nChannels { 1 };

	SmoothLogParameter lowEnabled;
	SmoothLogParameter midEnabled;
	SmoothLogParameter highEnabled;
	SmoothLogParameter allEnabled;

	FilteredParameter inputGain;
	FilteredParameter outputGain;

	float inputLow{ 1.0f };

public:

	void prepare(DSPParameters<float>& params) {
		sampleRate = params["sampleRate"];
		blockSize = params["blockSize"];
		nChannels = static_cast<int>(params["nChannels"]);
		
		lowEnabled. prepare(sampleRate, 1.0f - params["muteLow"]);
		midEnabled. prepare(sampleRate, 1.0f - params["muteMid"]);
		highEnabled.prepare(sampleRate, 1.0f - params["muteHigh"]);
		allEnabled. prepare(sampleRate, 1.0f - params["bypass"]);

		lowBand.prepare(sampleRate, blockSize, nChannels);
		lowBand.update(params["thresholdLow"], params["ratioLow"], 
					   params["attackLow"], params["releaseLow"],
					   params["inputLow"], params["outputLow"]);
		
		midBand.prepare(sampleRate, blockSize, nChannels);
		midBand.update(params["thresholdMid"], params["ratioMid"], 
					   params["attackMid"], params["releaseMid"], 
					   params["inputMid"], params["outputMid"]);
		
		highBand.prepare(sampleRate, blockSize, nChannels);
		highBand.update(params["thresholdHigh"], params["ratioHigh"], 
						params["attackHigh"], params["releaseHigh"],
					    params["inputHigh"], params["outputHigh"]);

		lowMidFilter.prepare(sampleRate, blockSize, nChannels);
		lowMidCut.prepare(sampleRate, params["lowMidCut"]);
		
		midHighFilter.prepare(sampleRate, blockSize, nChannels);
		midHighCut.prepare(sampleRate, params["midHighCut"]);

		inputGain.prepare(sampleRate, dbToLinear(params["inputAll"]));
		outputGain.prepare(sampleRate, dbToLinear(params["outputGainAll"]));

	}

	void update(DSPParameters<float>& params) {
		lowEnabled.setValue(1.0f - params["muteLow"]);
		midEnabled.setValue(1.0f - params["muteMid"]);
		highEnabled.setValue(1.0f - params["muteHigh"]);
		allEnabled.setValue(1.0f - params["bypass"]);

		lowMidCut.setValue(params["lowMidCut"]);
		midHighCut.setValue(params["midHighCut"]);

		lowBand.update(params["thresholdLow"], params["ratioLow"],
			params["attackLow"], params["releaseLow"],
			params["inputLow"], params["outputLow"]);

		midBand.update(params["thresholdMid"], params["ratioMid"],
			params["attackMid"], params["releaseMid"],
			params["inputMid"], params["outputMid"]);

		highBand.update(params["thresholdHigh"], params["ratioHigh"],
			params["attackHigh"], params["releaseHigh"],
			params["inputHigh"], params["outputHigh"]);

		inputGain.setValue(dbToLinear(params["inputAll"]));
		outputGain.setValue(dbToLinear(params["outputAll"]));
	
	}

	void processBlock(float** inputBuffer, int numChannels, int numSamples) {
		for (int ch = 0; ch < numChannels; ++ch) {
			for (int s = 0; s < numSamples; ++s) {
				auto sample = inputGain.next() * inputBuffer[ch][s];

				float lowBandFiltered, midBandFiltered, highBandFiltered;
				float lowBandCompressed, midBandCompressed, highBandCompressed;

				lowMidFilter.setFrequency(lowMidCut.next());
				midHighFilter.setFrequency(midHighCut.next());

				lowMidFilter.processSample(ch, sample, lowBandFiltered, midBandFiltered);
				midHighFilter.processSample(ch, midBandFiltered, midBandFiltered, highBandFiltered);

				lowBandCompressed = lowBand.processSample(lowBandFiltered) * lowEnabled.next();
				midBandCompressed = midBand.processSample(midBandFiltered) * midEnabled.next();
				highBandCompressed = highBand.processSample(highBandFiltered) * highEnabled.next();

				auto amplitude = allEnabled.next();

				inputBuffer[ch][s] = sample * (1.0f - amplitude) + 
					((lowBandCompressed + midBandCompressed + highBandCompressed) * outputGain.next() * amplitude);
			}
		}
	}
};

#undef DEFAULT_SR