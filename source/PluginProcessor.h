#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/DualBufferReverseEngine.h"
#include "dsp/TempoSync.h"

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    juce::AudioProcessorValueTreeState apvts;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // The longest Chunk Length the "chunkLengthMs" parameter can request; also used
    // to size the reverse engine's buffers so tempo-synced lengths never exceed it.
    static constexpr float maxChunkLengthMs = 2000.0f;
    static constexpr float crossfadeMs = 15.0f;

    double getCurrentBpm() const;
    int currentChunkLengthInSamples() const;

    DualBufferReverseEngine reverseEngine;
    juce::AudioBuffer<float> dryBuffer;

    std::atomic<float>* chunkLengthMsParam = nullptr;
    std::atomic<float>* tempoSyncParam = nullptr;
    std::atomic<float>* tempoSyncDivisionParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
