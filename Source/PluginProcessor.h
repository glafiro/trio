/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
using namespace juce;

#include <vector>
using std::vector;

#include "DSPParameters.h"
#include "Multiband.h"
#include "Utils.h"


#define PLUGIN_VERSION 1

#define PARAMETER_ID(str) const juce::ParameterID str(#str, PLUGIN_VERSION);

PARAMETER_ID(threshold)
PARAMETER_ID(ratio)
PARAMETER_ID(attack)
PARAMETER_ID(release)

#define THRESHOLD   0.0f
#define RATIO       3.0f
#define ATTACK      50.0f
#define RELEASE     250.0f

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
    
    AudioParameterFloat* thresholdParam;
    AudioParameterChoice* ratioParam;
    AudioParameterFloat* attackParam;
    AudioParameterFloat* releaseParam;

    std::atomic<bool> parametersChanged{ false };

    void valueTreePropertyChanged(ValueTree&, const Identifier&) override {
        parametersChanged.store(true);
    }

    void updateDSP();
    DSPParameters<float> compressorParameters;

    MultibandCompressor compressor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandCompressorAudioProcessor)
};
