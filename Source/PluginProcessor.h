/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
using namespace juce;


#include <vector>
#include <array>
#include <unordered_map>
using std::vector;
using std::array;
using std::unordered_map;

#include "DSPParameters.h"
#include "Multiband.h"
#include "Utils.h"
#include "APVTSParameter.h"

enum ParameterNames
{
    THRESHOLD_LOW, THRESHOLD_MID, THRESHOLD_HIGH,
    RATIO_LOW, RATIO_MID, RATIO_HIGH,
    ATTACK_LOW, ATTACK_MID, ATTACK_HIGH,
    RELEASE_LOW, RELEASE_MID, RELEASE_HIGH,
    BYPASS_LOW, BYPASS_MID, BYPASS_HIGH,
    LOW_MID_CUT, MID_HIGH_CUT,
    PARAMETER_COUNT
};

static std::array<std::unique_ptr<IAPVTSParameter>, ParameterNames::PARAMETER_COUNT> apvtsParameters{
    std::make_unique<APVTSParameterFloat> ("thresholdLow",  "Threshold Low",  0.0f),
    std::make_unique<APVTSParameterFloat> ("thresholdMid",  "Threshold Mid",  0.0f),
    std::make_unique<APVTSParameterFloat> ("thresholdHigh", "Threshold High", 0.0f),
    std::make_unique<APVTSParameterChoice>("ratioLow",      "Ratio Low",      3.0f),
    std::make_unique<APVTSParameterChoice>("ratioMid",      "Ratio Mid",      3.0f),
    std::make_unique<APVTSParameterChoice>("ratioHigh",     "Ratio High",     3.0f),
    std::make_unique<APVTSParameterFloat> ("attackLow",     "Attack Low",     50.0f),
    std::make_unique<APVTSParameterFloat> ("attackMid",     "Attack Mid",     50.0f),
    std::make_unique<APVTSParameterFloat> ("attackHigh",    "Attack High",    50.0f),
    std::make_unique<APVTSParameterFloat> ("releaseLow",    "Release Low",    250.0f),
    std::make_unique<APVTSParameterFloat> ("releaseMid",    "Release Mid",    250.0f),
    std::make_unique<APVTSParameterFloat> ("releaseHigh",   "Release High",   250.0f),
    std::make_unique<APVTSParameterBool>  ("bypassLow",     "Bypass Low",     0.0f),
    std::make_unique<APVTSParameterBool>  ("bypassMid",     "Bypass Mid",     0.0f),
    std::make_unique<APVTSParameterBool>  ("bypassHigh",    "Bypass High",    0.0f),
    std::make_unique<APVTSParameterFloat> ("lowMidCut",     "Low/Mid Cut",    700.0f),
    std::make_unique<APVTSParameterFloat> ("midHighCut",    "Mid/High Cut",    5000.0f),
};

class MultibandCompressorAudioProcessor  : 
    public AudioProcessor,
    private ValueTree::Listener
{
public:
    //==============================================================================
    MultibandCompressorAudioProcessor();
    ~MultibandCompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState apvts;

private:
    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<bool> parametersChanged{ false };

    void valueTreePropertyChanged(ValueTree&, const Identifier&) override {
        parametersChanged.store(true);
    }

    void updateDSP();
    DSPParameters<float> compressorParameters;

    MultibandCompressor compressor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandCompressorAudioProcessor)
};
