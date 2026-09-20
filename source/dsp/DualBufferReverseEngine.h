#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Dual-Buffer Engine (see docs/adr/0001-dual-buffer-reverse-engine.md and
// CONTEXT.md): two buffers alternate roles every chunkLength samples, one
// playing its captured audio back in reverse while the other silently fills
// with fresh input. A fade-in/fade-out envelope at each buffer's playback
// boundaries (Chunk Boundary Crossfade) avoids audible clicks at the handoff.
class DualBufferReverseEngine
{
public:
    // The highest gain the Feedback Path can reach. Kept just below unity: at exactly 1.0
    // repeats never decay and continued input keeps building the level up.
    static constexpr float maxFeedbackGain = 0.98f;

    void prepare (int numChannels, int maxChunkLengthSamples, int crossfadeLengthSamples);
    void setChunkLength (int chunkLengthSamples);
    void setFeedback (float amount);
    void processBlock (juce::AudioBuffer<float>& buffer);

private:
    juce::AudioBuffer<float> bufferA, bufferB;
    juce::AudioBuffer<float>* captureBuffer = &bufferA;
    juce::AudioBuffer<float>* playBuffer = &bufferB;

    int maxChunkLength = 0;
    int chunkLength = 0;
    int crossfadeLength = 0;
    int position = 0;
    int pendingChunkLength = -1; // -1 means no pending change
    float feedbackGain = 0.0f;

    float gainAt (int pos) const;
};
