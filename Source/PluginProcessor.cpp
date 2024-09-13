#include "PluginProcessor.h"
#include "PluginEditor.h"

MultibandCompressorAudioProcessor::MultibandCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    compressor()
#endif
{
    apvts.state.addListener(this);

    castParameter(apvts, threshold, thresholdParam);
    castParameter(apvts, ratio, ratioParam);
    castParameter(apvts, attack, attackParam);
    castParameter(apvts, release, releaseParam);
}

MultibandCompressorAudioProcessor::~MultibandCompressorAudioProcessor()
{
    apvts.state.removeListener(this);
}

const String MultibandCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MultibandCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MultibandCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MultibandCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MultibandCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MultibandCompressorAudioProcessor::getNumPrograms()
{
    return 1;   
}

int MultibandCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MultibandCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const String MultibandCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void MultibandCompressorAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void MultibandCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    int nChannels = getTotalNumInputChannels();

    compressorParameters.set("sampleRate", sampleRate);
    compressorParameters.set("blockSize", samplesPerBlock);
    compressorParameters.set("nChannels", nChannels);

    compressorParameters.set("threshold", THRESHOLD);
    compressorParameters.set("ratio", RATIO);
    compressorParameters.set("attack", ATTACK);
    compressorParameters.set("release", RELEASE);

    compressor.prepare(compressorParameters);

}

void MultibandCompressorAudioProcessor::releaseResources()
{

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MultibandCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MultibandCompressorAudioProcessor::updateDSP()
{

    compressorParameters.set("threshold", thresholdParam->get());
    compressorParameters.set("ratio", ratioParam->getCurrentChoiceName().getFloatValue());
    compressorParameters.set("attack", attackParam->get());
    compressorParameters.set("release", releaseParam->get());

    compressor.update(compressorParameters);
}

void MultibandCompressorAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool expected = true;
    
    if (isNonRealtime() || parametersChanged.compare_exchange_strong(expected, false)) {
        updateDSP();
    }

    compressor.processBlock(
        buffer.getArrayOfWritePointers(),
        buffer.getNumChannels(),
        buffer.getNumSamples()
    );

}

//==============================================================================
bool MultibandCompressorAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* MultibandCompressorAudioProcessor::createEditor()
{
    //return new MultibandCompressorAudioProcessorEditor (*this);
    return new GenericAudioProcessorEditor (*this);
}

void MultibandCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void MultibandCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        parametersChanged.store(true);
    }
}



juce::AudioProcessorValueTreeState::ParameterLayout MultibandCompressorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;


    layout.add(std::make_unique <juce::AudioParameterFloat>(
        threshold,
        "Threshold",
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f},
        THRESHOLD
    ));

    auto ratios = StringArray{ "1", "1.5", "2", "3", "4", "5", "6", "7", "8", "10", "15", "20", "50", "100" };

    layout.add(std::make_unique <juce::AudioParameterChoice>(
        ratio,
        "Ratio",
        ratios,
        3
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        attack,
        "Attack",
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f},
        ATTACK
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        release,
        "Release",
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f},
        RELEASE
    ));

    return layout;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultibandCompressorAudioProcessor();
}

