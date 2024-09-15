
#pragma once

#include "Filters.h"

#define DEFAULT_FILTER_FREQ 0.5f
#define DEFAULT_SR          44100.0f

class FilteredParameter
{
    OnePoleFilter filter{ DEFAULT_FILTER_FREQ };
    float sampleRate{ DEFAULT_SR };
    float value{ 0.0f };
    float frequency{ DEFAULT_FILTER_FREQ };

public:

    FilteredParameter(float sr = DEFAULT_SR) : sampleRate(sr) {}

    void prepare(float sr, float v) {
        filter.setSampleRate(sr);
        filter.setFrequency(DEFAULT_FILTER_FREQ);
        value = v;
    }

    // Filter then return current value
    float next() {
        return filter.process(value);
    }

    // Just return current value
    float read() {
        return value;
    }


    void setValue(float v) {
        value = v;
    }
};

#define SILENCE 0.000001f

class SmoothLogParameter
{
    float currentGain, targetGain, multiplier;
    float attackTime, releaseTime, fadeSize{ 1.0f };
    float sampleRate{ DEFAULT_SR };

public:
    SmoothLogParameter(float atk = 150.0f, float rls = 150.0f) : attackTime(atk), releaseTime(rls), currentGain(SILENCE), targetGain(SILENCE), multiplier(1.0) {}

    void prepare(float sr, float v) {
        sampleRate = sr;
        currentGain = v;
        setValue(v);
    }

    void setValue(float v) {
        targetGain = v + SILENCE;
        if (targetGain < currentGain) fadeSize = lengthToSamples(releaseTime, sampleRate);
        else if (targetGain > currentGain ) fadeSize = lengthToSamples(attackTime, sampleRate);
        multiplier = std::pow(targetGain / currentGain, 1.0f / fadeSize);
    }

    float next() {
        if (std::abs(currentGain - targetGain) > 0.0001f) {
            currentGain *= multiplier;
            if ((multiplier > 1.0f && currentGain >= targetGain) || (multiplier < 1.0f && currentGain <= targetGain)) {
                currentGain = targetGain;
            }
        }
        return currentGain;
    }

    float read() {
        return currentGain;
    }
};


#undef DEFAULT_SR