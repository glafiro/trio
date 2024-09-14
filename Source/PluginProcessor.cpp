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
    for (auto& param : parameters) {
        param->castParameter(apvts);
    }
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

    for (auto& param : parameters) {
        compressorParameters.set(param->id.getParamID().toStdString(), param->getDefault());
    }

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

    for (auto& param : parameters) {
        compressorParameters.set(param->id.getParamID().toStdString(), param->get());
    }

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
        parameters[ParameterNames::THRESHOLD_LOW]->id,
        parameters[ParameterNames::THRESHOLD_LOW]->displayValue,
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f },
        parameters[ParameterNames::THRESHOLD_LOW]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::THRESHOLD_MID]->id,
        parameters[ParameterNames::THRESHOLD_MID]->displayValue,
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f},
        parameters[ParameterNames::THRESHOLD_MID]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::THRESHOLD_HIGH]->id,
        parameters[ParameterNames::THRESHOLD_HIGH]->displayValue,
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f},
        parameters[ParameterNames::THRESHOLD_HIGH]->getDefault()
    ));

    auto ratios = StringArray{ "1", "1.5", "2", "3", "4", "5", "6", "7", "8", "10", "15", "20", "50", "100" };

    layout.add(std::make_unique <juce::AudioParameterChoice>(
        parameters[ParameterNames::RATIO_LOW]->id,
        parameters[ParameterNames::THRESHOLD_LOW]->displayValue,
        ratios,
        3
    ));
    
    layout.add(std::make_unique <juce::AudioParameterChoice>(
        parameters[ParameterNames::RATIO_MID]->id,
        parameters[ParameterNames::THRESHOLD_MID]->displayValue,
        ratios,
        3
    ));
    
    layout.add(std::make_unique <juce::AudioParameterChoice>(
        parameters[ParameterNames::RATIO_HIGH]->id,
        parameters[ParameterNames::THRESHOLD_HIGH]->displayValue,
        ratios,
        3
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::ATTACK_LOW]->id,
        parameters[ParameterNames::ATTACK_LOW]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::ATTACK_LOW]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::ATTACK_MID]->id,
        parameters[ParameterNames::ATTACK_MID]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::ATTACK_MID]->getDefault()
    ));


    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::ATTACK_HIGH]->id,
        parameters[ParameterNames::ATTACK_HIGH]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::ATTACK_HIGH]->getDefault()
    ));


    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::RELEASE_LOW]->id,
        parameters[ParameterNames::RELEASE_LOW]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::RELEASE_LOW]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::RELEASE_MID]->id,
        parameters[ParameterNames::RELEASE_MID]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::RELEASE_MID]->getDefault()
    ));


    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::RELEASE_HIGH]->id,
        parameters[ParameterNames::RELEASE_HIGH]->displayValue,
        juce::NormalisableRange<float>{ 5.0f, 5000.0f, 1.0f },
        parameters[ParameterNames::RELEASE_HIGH]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterBool>(
        parameters[ParameterNames::BYPASS_LOW]->id,
        parameters[ParameterNames::BYPASS_LOW]->displayValue,
        parameters[ParameterNames::BYPASS_LOW]->getDefault()
    ));
    
    layout.add(std::make_unique <juce::AudioParameterBool>(
        parameters[ParameterNames::BYPASS_MID]->id,
        parameters[ParameterNames::BYPASS_MID]->displayValue,
        parameters[ParameterNames::BYPASS_MID]->getDefault()
    ));
    
    layout.add(std::make_unique <juce::AudioParameterBool>(
        parameters[ParameterNames::BYPASS_HIGH]->id,
        parameters[ParameterNames::BYPASS_HIGH]->displayValue,
        parameters[ParameterNames::BYPASS_HIGH]->getDefault()
    ));

    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::LOW_MID_CUT]->id,
        parameters[ParameterNames::LOW_MID_CUT]->displayValue,
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f },
        parameters[ParameterNames::LOW_MID_CUT]->getDefault()
    ));
    
    layout.add(std::make_unique <juce::AudioParameterFloat>(
        parameters[ParameterNames::MID_HIGH_CUT]->id,
        parameters[ParameterNames::MID_HIGH_CUT]->displayValue,
        juce::NormalisableRange<float>{ -60.0f, 12.0f, 1.0f },
        parameters[ParameterNames::MID_HIGH_CUT]->getDefault()
    ));


    return layout;
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultibandCompressorAudioProcessor();
}

