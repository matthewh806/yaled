#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    chunkLengthMsParam = apvts.getRawParameterValue ("chunkLengthMs");
    tempoSyncParam = apvts.getRawParameterValue ("tempoSync");
    tempoSyncDivisionParam = apvts.getRawParameterValue ("tempoSyncDivision");
    mixParam = apvts.getRawParameterValue ("mix");
    feedbackParam = apvts.getRawParameterValue ("feedback");
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    // Steinberg's VST3 validator fails plugins whose single default program has no name
    return "Default";
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "chunkLengthMs", 1 },
        "Chunk Length",
        juce::NormalisableRange<float> (10.0f, maxChunkLengthMs, 0.0f, 0.3f),
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "tempoSync", 1 },
        "Tempo Sync",
        false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "tempoSyncDivision", 1 },
        "Tempo Sync Division",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16" },
        2)); // default: 1/4

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // 100% maps to the engine's maxFeedbackGain (just below unity), which is what keeps
    // the Feedback Path from running away; see DualBufferReverseEngine::maxFeedbackGain.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 },
        "Feedback",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

double PluginProcessor::getCurrentBpm() const
{
    constexpr double defaultBpm = 120.0;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
                return *bpm;
        }
    }

    return defaultBpm;
}

int PluginProcessor::currentChunkLengthInSamples() const
{
    const auto sampleRate = getSampleRate();

    if (tempoSyncParam->load() >= 0.5f)
    {
        const auto division = static_cast<NoteDivision> (static_cast<int> (tempoSyncDivisionParam->load()));
        return noteDivisionToSamples (getCurrentBpm(), sampleRate, division);
    }

    const auto ms = chunkLengthMsParam->load();
    return static_cast<int> (std::round (static_cast<double> (ms) / 1000.0 * sampleRate));
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // A tempo-synced length at a very slow tempo could exceed this; setChunkLength()
    // clamps to it rather than growing the buffers, which is an acceptable limit.
    const auto maxChunkLengthSamples = static_cast<int> (std::round (
        static_cast<double> (maxChunkLengthMs) / 1000.0 * sampleRate));
    const auto crossfadeLengthSamples = static_cast<int> (std::round (
        static_cast<double> (crossfadeMs) / 1000.0 * sampleRate));

    reverseEngine.prepare (getTotalNumOutputChannels(), maxChunkLengthSamples, crossfadeLengthSamples);
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto numSamples = buffer.getNumSamples();
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    reverseEngine.setChunkLength (currentChunkLengthInSamples());
    reverseEngine.setFeedback (feedbackParam->load() / 100.0f);
    reverseEngine.processBlock (buffer);

    const auto wetAmount = mixParam->load() / 100.0f;
    const auto dryAmount = 1.0f - wetAmount;
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* wet = buffer.getWritePointer (channel);
        auto* dry = dryBuffer.getReadPointer (channel);

        for (int n = 0; n < numSamples; ++n)
            wet[n] = dry[n] * dryAmount + wet[n] * wetAmount;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
